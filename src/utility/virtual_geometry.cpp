#include "virtual_geometry.h"
#include "my_mesh_optimizer.h"
#include "utils.h"
#include <unordered_set>
#include <functional>
#include <numeric>
#include <algorithm>

namespace
{
	glm::vec4 _MergeBounds(const glm::vec4& inSphere1, const glm::vec4& inSphere2)
	{
		glm::vec4 result{};

		// center
		result.x = (inSphere1.x + inSphere2.x) * 0.5f;
		result.y = (inSphere1.y + inSphere2.y) * 0.5f;
		result.z = (inSphere1.z + inSphere2.z) * 0.5f;

		// radius
		result.w =
			std::max(inSphere1.w, inSphere2.w)
			+ glm::distance(glm::vec3(result.x, result.y, result.z), glm::vec3(inSphere1.x, inSphere1.y, inSphere1.z));

		return result;
	}
}

const std::vector<Vertex>& VirtualGeometry::_GetCompleteVertices(uint32_t inLod) const
{
	return m_pBaseMesh->verts;
}

void VirtualGeometry::_AddMyMeshlet(
	uint32_t inLod, 
	float inError, 
	const std::vector<Meshlet>& inNewMeshlets, 
	const std::vector<uint32_t>& inChildren, 
	uint32_t* outFirstIndexPtr, 
	uint32_t* outNumAddedPtr)
{
	auto& vectorToAdd = m_meshlets[inLod];
	uint32_t firstIndex = vectorToAdd.size();
	uint32_t numAdded = inNewMeshlets.size();
	float tinyClusterMaxError = 0.0f;	// maximum of all childrens' cluster error
	float clusterError = 0.0f;			// all children have the same cluster error
	MeshOptimizer optimizer{};
	
	if (outFirstIndexPtr != nullptr)
	{
		*outFirstIndexPtr = firstIndex;
	}
	if (outNumAddedPtr != nullptr)
	{
		*outNumAddedPtr = numAdded;
	}

	// find maximum error of tiny clusters
	if (inLod > 0)
	{
		uint32_t finerLOD = inLod - 1;
		for (size_t i = 0; i < inChildren.size(); ++i)
		{
			const MyMeshlet& parentMeshlet = m_meshlets[finerLOD][inChildren[i]];
			tinyClusterMaxError = std::max(tinyClusterMaxError, parentMeshlet.clusterError);
		}
	}
	clusterError = inError + tinyClusterMaxError;

	// fill in new myMeshlets
	for (uint32_t i = 0; i < numAdded; ++i)
	{
		MyMeshlet myMeshlet{};
		std::vector<uint32_t> indices;

		inNewMeshlets[i].GetIndices(indices);
		auto bounds = optimizer.ComputeBounds(_GetCompleteVertices(inLod), indices);
		
		myMeshlet.firstLove = firstIndex;
		myMeshlet.loverCount = numAdded;
		myMeshlet.lod = inLod;
		myMeshlet.meshlet = inNewMeshlets[i];
		myMeshlet.children = inChildren;
		myMeshlet.boundingSphere = glm::vec4(bounds.center, bounds.radius);
		myMeshlet.clusterError = clusterError;
		myMeshlet.groupError = FLT_MAX; // to be filled later by its parent(Larger clusters)

		vectorToAdd.push_back(myMeshlet);
	}

	// update children(Tiny clusters)
	if (inLod > 0)
	{
		uint32_t finerLOD = inLod - 1;
		for (size_t i = 0; i < inChildren.size(); ++i)
		{
			MyMeshlet& childMeshlet = m_meshlets[finerLOD][inChildren[i]];
			childMeshlet.parent = firstIndex;
			childMeshlet.groupError = clusterError; // all children have the same group error
		}
	}
}

void VirtualGeometry::_PrepareMETIS(
	uint32_t inLod, 
	std::vector<idx_t>& outXadj, 
	std::vector<idx_t>& outAdjncy, 
	std::vector<idx_t>& outAdjwgt) const
{
	const uint32_t n = m_meshlets[inLod].size();
	outXadj.clear();
	outAdjncy.clear();
	outAdjwgt.clear();
	outXadj.resize(n + 1);
	outAdjncy.reserve(8 * n);
	outAdjwgt.reserve(8 * n);

	outXadj.push_back(static_cast<idx_t>(outAdjncy.size()));
	for (uint32_t i = 0; i < n; ++i)
	{
		const auto& curMeshlet = m_meshlets[inLod][i];
		for (const auto& p : curMeshlet.adjacentWeight)
		{
			outAdjncy.push_back(static_cast<idx_t>(p.first));		// index of the adjacent meshlet
			outAdjwgt.push_back(static_cast<idx_t>(p.second));	// number of shared edges
		}
		outXadj.push_back(static_cast<idx_t>(outAdjncy.size()));
	}
}

void VirtualGeometry::_FindMeshletsBorder(uint32_t inLod)
{
	MeshOptimizer optimizer{};
	std::vector<uint32_t> remapIndex(_GetCompleteVertices(inLod).size()); // remap the index based on position
	
	optimizer.GeneratePositionRemap(_GetCompleteVertices(inLod), remapIndex);

	// for each meshlet of this LOD find its border
	for (size_t i = 0; i < m_meshlets[inLod].size(); ++i)
	{
		std::unordered_map<std::pair<uint32_t, uint32_t>, uint32_t, common_utils::PairHash> edgeSeen;
		MyMeshlet& myMeshlet = m_meshlets[inLod][i];
		const Meshlet& meshlet = myMeshlet.meshlet;
		 
		// iterate all triangle of current meshlet
		for (uint32_t j = 0; j < meshlet.index.size() / 3; ++j)
		{
			std::array<uint32_t, 3> indices;
			std::array<std::pair<uint32_t, uint32_t>, 3> edge;

			meshlet.GetTriangle(j, indices);
			edge[0] = { remapIndex[indices[0]], remapIndex[indices[1]] };
			edge[1] = { remapIndex[indices[1]], remapIndex[indices[2]] };
			edge[2] = { remapIndex[indices[2]], remapIndex[indices[0]] };

			for (int k = 0; k < 3; ++k)
			{
				auto& currentEdge = edge[k];
				uint32_t head = std::min(currentEdge.first, currentEdge.second);
				uint32_t tail = std::max(currentEdge.first, currentEdge.second);

				currentEdge = { head, tail };
				if (edgeSeen.find(currentEdge) == edgeSeen.end())
				{
					edgeSeen.insert({ currentEdge, 1 });
				}
				else
				{
					edgeSeen[currentEdge] += 1;
				}
			}
		}

		// find edges shows up only once as border of current meshlet
		for (const auto& edgeAndCount : edgeSeen)
		{
			if (edgeAndCount.second == 1)
			{
				myMeshlet.boundaries.insert(edgeAndCount.first);
			}
		}
	}
}

void VirtualGeometry::_RecordMeshletConnections(uint32_t inLod)
{
	std::unordered_map<
		std::pair<uint32_t, uint32_t>, 
		std::vector<uint32_t>, 
		common_utils::PairHash> edgeMeshlet; // edge to meshlets that have the edge

	// fill edge meshlet map
	for (uint32_t i = 0; i < m_meshlets[inLod].size(); ++i)
	{
		const auto& currentMeshlet = m_meshlets[inLod][i];
		for (const auto& edge : currentMeshlet.boundaries)
		{
			if (edgeMeshlet.find(edge) == edgeMeshlet.end())
			{
				edgeMeshlet.insert({ edge, { i } });
			}
			else
			{
				edgeMeshlet[edge].push_back(i);
			}
		}
	}

	// for each edge, update meshlets connected to it
	for (const auto& p : edgeMeshlet)
	{
		for (uint32_t i = 0; i < p.second.size(); ++i)
		{
			uint32_t meshletId = p.second[i];
			auto& meshlet = m_meshlets[inLod][meshletId];
			for (uint32_t j = 0; j < p.second.size(); ++j)
			{
				if (j == i) continue;
				uint32_t meshletId2 = p.second[j];
				if (meshlet.adjacentWeight.find(meshletId2) == meshlet.adjacentWeight.end())
				{
					meshlet.adjacentWeight.insert({ meshletId2, 1 });
				}
				else
				{
					meshlet.adjacentWeight[meshletId2]++;
				}
			}
		}
	}
}

bool VirtualGeometry::_DivideMeshletGroup(
	uint32_t inLod, 
	uint32_t inGroupCount, 
	std::vector<std::vector<uint32_t>>& outClusterGroups) const
{
	std::vector<idx_t> xadj;
	std::vector<idx_t> adjncy;
	std::vector<idx_t> adjwgt;
	std::vector<idx_t> part;
	idx_t nvtxs = m_meshlets[inLod].size();
	//idx_t nparts = std::max(static_cast<idx_t>(m_meshlets[inLod].size() / 4), 1);
	idx_t nparts = static_cast<idx_t>(inGroupCount);
	idx_t edgecut = 0;
	idx_t options[METIS_NOPTIONS];
	idx_t ncon = 1;
	std::function<int(idx_t*, idx_t*, idx_t*, idx_t*, idx_t*, idx_t*, idx_t*, idx_t*, real_t*, real_t*, idx_t*, idx_t*, idx_t*)> metisFunc;
	int result = 0;

	METIS_SetDefaultOptions(options);  // Initialize default options
	part.resize(nvtxs);
	_PrepareMETIS(inLod, xadj, adjncy, adjwgt);
	if (nparts > 8)
	{
		metisFunc = METIS_PartGraphKway;
	}
	else
	{
		metisFunc = METIS_PartGraphRecursive;
	}
	result = metisFunc(
		&nvtxs,				// nvtxs, Number of vertices
		&ncon,				// ncon, Number of balancing constraints (usually NULL)
		xadj.data(),			// xadj, Pointer to xadj array
		adjncy.data(),		// adjncy, Pointer to adjncy array
		NULL,					// vwgt, Vertex weights (NULL if not used)
		NULL,					// vsize, Sizes of vertex weights (NULL if not used)
		adjwgt.data(),		// adjwgt, Edge weights (NULL if not used)
		&nparts,				// nparts, Number of parts
		NULL,					// tpwgts, Imbalance tolerance (NULL uses default)
		NULL,					// ubvec, Edge weight factor (NULL uses default)
		options,				// options, Options array
		&edgecut,			// edgecut, Output: Edge cut value
		part.data());			// part, Output: Partition vector

	if (result == METIS_OK)
	{
		outClusterGroups.resize(nparts, {});
		for (uint32_t i = 0; i < nvtxs; ++i)
		{
			idx_t groupId = part[i];
			outClusterGroups[groupId].push_back(i);
		}
	}

	return result == METIS_OK;
}

float VirtualGeometry::_SimplifyGroupTriangles(
	uint32_t inSrcLod, 
	const std::vector<uint32_t>& inClusterGroup,
	std::vector<uint32_t>& inoutIndex) const
{
	std::vector<uint32_t> meshletIndex;
	MeshOptimizer optimizer{};
	float error = 0.0f;

	for (size_t i = 0; i < inClusterGroup.size(); ++i)
	{
		const auto& meshlet = m_meshlets[inSrcLod][inClusterGroup[i]].meshlet;
		meshlet.GetIndices(meshletIndex);
	}

	error = optimizer.SimplifyMesh(
		_GetCompleteVertices(inSrcLod),
		meshletIndex,
		meshletIndex.size() / 2,
		inoutIndex);

	return error;
}

void VirtualGeometry::_BuildMeshletFromGroup(
	const std::vector<Vertex>& _vertex,
	const std::vector<uint32_t>& _index, 
	std::vector<Meshlet>& _meshlet) const
{
	MeshOptimizer optimizer{};
	optimizer.BuildMeshlets(_vertex, _index, _meshlet);
}

void VirtualGeometry::_SplitMeshLODs()
{
	int maxLOD = 31;
	MeshOptimizer optimizer{};
	//std::vector<std::vector<uint32_t>> meshletGroups;
	m_meshlets.resize(maxLOD + 1);

	std::cout << "Start build virtual geometry..." << std::endl;
	std::cout << "===============================" << std::endl;
	//_BuildVirtualIndexMap();
	for (int i = 0; i < (maxLOD + 1); ++i)
	{
		// Build meshlets for current LOD
		if (i == 0)
		{
			std::vector<Meshlet> meshlets{};
			uint32_t firstIndx;
			uint32_t numAdded;

			std::cout << "Start build LOD " << i << " meshlets...";
			optimizer.BuildMeshlets(m_pBaseMesh->verts, m_pBaseMesh->indices, meshlets);
			_AddMyMeshlet(i, 0.0f, meshlets, {}, &firstIndx, &numAdded);
			std::cout << "triangle count: " << m_pBaseMesh->indices.size() / 3;
			std::cout << ", vertex count: " << _GetCompleteVertices(i).size() << std::endl;
			std::cout << "DONE, meshlets added: " << numAdded << std::endl;
		}
		else
		{
			// For each group of meshlets, build a new list of triangles approximating the original group
			// For each simplified group, break them apart into new meshlets
			uint32_t firstIndx;
			uint32_t numAdded = 0u;
			uint32_t subNumAdded;
			uint32_t numTrig = 0u;
			std::vector<std::vector<uint32_t>> groupIndices;
			std::vector<float> simplifyError;

			std::cout << "Start build LOD " << i << " meshlets...";
			
			// simplify triangles in group
			for (const auto& group : m_groups.back())
			{
				std::vector<uint32_t> simplifiedIndex;
				float error = 0.0f;

				// For each group of meshlets, build a new list of triangles approximating the original group
				error = _SimplifyGroupTriangles(i - 1, group, simplifiedIndex);
				numTrig += simplifiedIndex.size() / 3;
				groupIndices.push_back(simplifiedIndex);
				simplifyError.push_back(error);
			}

			// remove redundant vertices
			//uint32_t maxVerticesIndex = 0;
			////std::cout << "vertex count before simplify: " << _GetIncompleteVertices(i).size() << std::endl;
			//for (auto& simplifiedIndex : groupIndices)
			//{
			//	std::vector<uint32_t> newIndex(simplifiedIndex.size());
			//	
			//	optimizer.RemapIndex(_GetIncompleteVertices(i), simplifiedIndex, newIndex, sizeof(Vertex));
			//	simplifiedIndex = newIndex;
			//	maxVerticesIndex = std::max(maxVerticesIndex, *std::max_element(simplifiedIndex.begin(), simplifiedIndex.end()));
			//}
			//_GetIncompleteVertices(i).resize(maxVerticesIndex + 1);
			std::cout << "triangle count: " << numTrig << std::endl;

			// build meshlets from simplified triangles
			for (size_t j = 0; j < groupIndices.size(); ++j)
			{
				std::vector<Meshlet> meshlets{};
				const auto& simplifiedIndices = groupIndices[j];
				float error = simplifyError[j];

				_BuildMeshletFromGroup(_GetCompleteVertices(i), simplifiedIndices, meshlets);
				_AddMyMeshlet(i, error, meshlets, m_groups.back()[j], &firstIndx, &subNumAdded);
				numAdded += subNumAdded;
			}
			std::cout << "DONE, meshlets added: " << numAdded << std::endl;
		}

		// if LOD reaches max we don't need to group meshlet any more, break;
		if (m_meshlets[i].size() == 1)
		{
			m_groups.push_back({ { 0 } });
			m_meshlets[i][0].groupIndex = 0;
			break;
		}

		// For each meshlet, find the set of all edges making up the triangles within the meshlet
		std::cout << "Start find meshlets' border...";
		_FindMeshletsBorder(i);
		std::cout << "DONE" << std::endl;

		// For each meshlet, find the set of connected meshlets(sharing an edge)
		std::cout << "Start record connections...";
		_RecordMeshletConnections(i);
		std::cout << "DONE" << std::endl;

		// Divide meshlets into groups of roughly 8
		//meshletGroups.clear();
		std::cout << "Start LOD " << i << " meshlets partition...";
		uint32_t groupCount = m_meshlets[i].size() / 8;
		groupCount = groupCount > 0 ? groupCount : 1;
		m_groups.push_back({});
		auto divideSuccess = _DivideMeshletGroup(i, groupCount, m_groups.back());

		// update mymeshlet group id attribute
		for (size_t groupId = 0; groupId < m_groups.back().size(); ++groupId)
		{
			for (auto meshletId : m_groups.back()[groupId])
			{
				m_meshlets[i][meshletId].groupIndex = groupId;
			}
		}

		// Remove data we don't fill
		if (!divideSuccess)
		{
			m_meshlets.resize(i + 1);
			std::cout << "ERROR: Cannot divide meshlet groups, break!" << std::endl;
			break;
		}
		std::cout << "DONE, divides meshlets into " << m_groups.back().size() << " groups.\r\n" << std::endl;
	}
	std::cout << "===============================\r\n" << std::endl;
}

void VirtualGeometry::_BuildHierarchy()
{
	std::vector<HierarchyNode>& treeNodes = m_hierarchy;
	std::vector<ClusterGroupData>& deviceGroups = m_deviceGroups;
	std::vector<IntermediateNode>& deviceNodes = m_deviceNodes;
	std::vector<std::vector<uint32_t>> LODClusterID; // leaf index of each LOD
	std::vector<uint32_t> LODRoots;
	uint32_t processedGroup = 0;

	treeNodes.clear();
	deviceGroups.clear();
	deviceNodes.clear();

	// build base node from cluster groups
	LODClusterID.resize(m_groups.size());
	for (size_t i = 0; i < LODClusterID.size(); ++i)
	{
		size_t clusterGroupsCount = m_groups[i].size();
		
		LODClusterID[i].resize(clusterGroupsCount);
		for (size_t j = 0; j < clusterGroupsCount; ++j)
		{
			const auto& currentGroup = m_groups[i][j];
			HierarchyNode newNode{};
			ClusterGroupData deviceGroup{};
			Meshlet::DeviceData clusterCompactData{};
			uint32_t clusterCount = 0;

			LODClusterID[i][j] = treeNodes.size();
			newNode.isClusterGroup = true;
			newNode.bounding = m_meshlets[i][currentGroup[0]].boundingSphere;
			for (auto clusterId : currentGroup)
			{
				const auto& currentCluster = m_meshlets[i][clusterId];
				Meshlet::DeviceDataRef clusterCompactRef{};

				newNode.error = std::max(newNode.error, currentCluster.groupError);
				newNode.bounding = _MergeBounds(newNode.bounding, currentCluster.boundingSphere);
				Meshlet::CompressToDeviceData(currentCluster.meshlet, clusterCompactData, clusterCompactRef);
				deviceGroup._SetClusterData(
					clusterCount, 
					clusterCompactRef.vertexOffset, 
					clusterCompactRef.vertexCount, 
					clusterCompactRef.indexOffset / 3,
					clusterCompactRef.triangleCount,
					glm::vec3(currentCluster.boundingSphere),
					currentCluster.boundingSphere.w,
					currentCluster.clusterError);
				++clusterCount;
			}
			newNode.children[0] = processedGroup;
			newNode.groupLod = i;
			newNode.groupIndex = j;
			deviceGroup._SetClusterCount(clusterCount);
			deviceGroup._SetMeshletCompactData(clusterCompactData.meshletVertices, clusterCompactData.meshletIndices);

			++processedGroup;
			treeNodes.push_back(newNode);
			deviceGroups.push_back(deviceGroup);
		}
	}

	// build hierarchy for each LOD
	for (size_t i = 0; i < LODClusterID.size(); ++i)
	{
		if (LODClusterID[i].size() == 0) continue;
		uint32_t lodRootId = _BuildHierarchyHelper(LODClusterID[i], treeNodes);
		LODRoots.push_back(lodRootId);
	}

	// build top hierarchy with LOD trees
	m_rootIndex = _BuildHierarchyHelper(LODRoots, treeNodes);

	// convert to device nodes
	for (size_t i = 0; i < treeNodes.size(); ++i)
	{
		auto& treeNode = treeNodes[i];
		IntermediateNode deviceNode{};

		deviceNode._SetError(treeNode.error);
		deviceNode._SetBoundingSphere(glm::vec3(treeNode.bounding.x, treeNode.bounding.y, treeNode.bounding.z), treeNode.bounding.w);
		if (treeNode.isClusterGroup)
		{
			deviceNode._SetChildrenNodesOrClusterGroup(treeNode.children[0]);
		}
		else
		{
			deviceNode._SetChildrenNodesOrClusterGroup(treeNode.children);
		}
		deviceNodes.push_back(deviceNode);
	}
}

uint32_t VirtualGeometry::_BuildHierarchyHelper(std::vector<uint32_t>& _bottomNodeIndex, std::vector<HierarchyNode>& _fullTree) const
{
	CHECK_TRUE(_bottomNodeIndex.size() > 0, "Empty tree is not allowed!");

	// if there is only one node
	if (_bottomNodeIndex.size() == 1) return _bottomNodeIndex[0];

	// if there is only one child layer
	if (_bottomNodeIndex.size() <= VG_HIERARCHY_MAX_CHILD)
	{
		HierarchyNode newParentNode{};
		uint32_t firstChildIndex = _bottomNodeIndex[0];
		uint32_t newNodeIndex = _fullTree.size();

		// use first bottom node as start
		newParentNode = _fullTree[firstChildIndex];
		newParentNode.children = {firstChildIndex , ~0u, ~0u, ~0u }; // to be filled
		newParentNode.isClusterGroup = false;
		
		for (size_t i = 1; i < _bottomNodeIndex.size(); ++i)
		{
			uint32_t childIndex = _bottomNodeIndex[i];
			const HierarchyNode& child = _fullTree[childIndex];

			_MergeHierarchyNode(child, newParentNode);
			newParentNode.children[i] = childIndex;
		}

		_fullTree.push_back(newParentNode);
		return newNodeIndex;
	}

	// if there are more layers
	{
		uint32_t maxSubNodeCount = VG_HIERARCHY_MAX_CHILD;	// maximum possible leaf count of each subtree
		uint32_t baseSubNodeCount;							// baseline leaf count of each subtree
		uint32_t leftSubNodeCount; // if following subtree only hold baseSubNodeCount how many node will remain
		uint32_t maxExtraHold;
		HierarchyNode newNode{};
		uint32_t result = 0;
		size_t offset = 0; // offset of _bottomNodeIndex for subtree

		while (maxSubNodeCount * VG_HIERARCHY_MAX_CHILD <= _bottomNodeIndex.size())
		{
			maxSubNodeCount *= VG_HIERARCHY_MAX_CHILD;
		}
		baseSubNodeCount = maxSubNodeCount / VG_HIERARCHY_MAX_CHILD;
		leftSubNodeCount = _bottomNodeIndex.size() - baseSubNodeCount * VG_HIERARCHY_MAX_CHILD;
		maxExtraHold = maxSubNodeCount - baseSubNodeCount;

		std::array<uint32_t, VG_HIERARCHY_MAX_CHILD> subNodeCounts{}; // how many leaf nodes each child should handle
		for (size_t i = 0; i < VG_HIERARCHY_MAX_CHILD; ++i)
		{
			if (leftSubNodeCount > maxExtraHold)
			{
				subNodeCounts[i] = maxSubNodeCount;
				leftSubNodeCount -= (maxExtraHold - baseSubNodeCount);
			}
			else
			{
				subNodeCounts[i] = baseSubNodeCount + leftSubNodeCount;
				leftSubNodeCount = 0;
			}
		}
		_OptimizeHierarchyNodeGroups(_fullTree, subNodeCounts, _bottomNodeIndex);

		// build each subtree
		for (size_t i = 0; i < VG_HIERARCHY_MAX_CHILD; ++i)
		{
			std::vector<uint32_t> subBottomNodes(
				_bottomNodeIndex.cbegin() + offset, 
				_bottomNodeIndex.cbegin() + offset + subNodeCounts[i]);
			const uint32_t childId = _BuildHierarchyHelper(subBottomNodes, _fullTree);
			const HierarchyNode& child = _fullTree[childId];

			if (i == 0)
			{
				newNode.bounding = child.bounding;
				newNode.error = child.error;
				newNode.isClusterGroup = false;
			}
			else
			{
				_MergeHierarchyNode(child, newNode);
			}
			newNode.children[i] = childId;
			offset += subNodeCounts[i];
		}
		result = _fullTree.size();
		_fullTree.push_back(newNode);

		return result;
	}

	return 0;
}

void VirtualGeometry::_OptimizeHierarchyNodeGroups(
	const std::vector<HierarchyNode>& _fullTree, 
	const std::array<uint32_t, VG_HIERARCHY_MAX_CHILD>& _groupSize,
	std::vector<uint32_t>& _nodeIndices) const
{
	uint32_t levelCount = 1;
	auto funcSortByAxis =
		[&](uint32_t axis, uint32_t beginOffset, uint32_t endOffset)
		{
			std::function<bool(uint32_t, uint32_t)> funcCmp;
			switch (axis)
			{
			case 0:
				funcCmp = [&](uint32_t l, uint32_t r)->bool
					{
						return _fullTree[l].bounding.x < _fullTree[r].bounding.x;
					};
				break;
			case 1:
				funcCmp = [&](uint32_t l, uint32_t r)->bool
					{
						return _fullTree[l].bounding.y < _fullTree[r].bounding.y;
					};
				break;
			case 2:
				funcCmp = [&](uint32_t l, uint32_t r)->bool
					{
						return _fullTree[l].bounding.z < _fullTree[r].bounding.z;
					};
				break;
			default:
				CHECK_TRUE(false, "No such axis!");
			}
			std::sort(_nodeIndices.begin() + beginOffset, _nodeIndices.begin() + endOffset, funcCmp);
		};
	
	auto funcCalculateCost = [](
		const std::vector<VirtualGeometry::HierarchyNode>& inFullTree,
		const std::vector<uint32_t>& inNodeIndices,
		uint32_t inBegin,
		uint32_t inEnd)
		{
			auto mergedSphere = inFullTree[inNodeIndices[inBegin]].bounding;

			for (uint32_t i = inBegin + 1; i < inEnd; ++i)
			{
				mergedSphere = _MergeBounds(mergedSphere, inFullTree[inNodeIndices[i]].bounding);
			}

			return mergedSphere.w * mergedSphere.w; // we want to calculate area
		};

	while ((1 << levelCount) < VG_HIERARCHY_MAX_CHILD)
	{
		levelCount++;
	}
	CHECK_TRUE((1 << levelCount) == VG_HIERARCHY_MAX_CHILD, "");

	for (uint32_t level = 0; level < levelCount; ++level)
	{
		const uint32_t bucketCount = 1 << level;
		const uint32_t groupPerBucket = VG_HIERARCHY_MAX_CHILD >> level;

		// sort each bucket
		uint32_t bucketNodeOffset = 0;
		for (uint32_t i = 0; i < bucketCount; ++i)
		{
			const uint32_t offset = groupPerBucket * i;
			uint32_t sizes[2] = {};
			uint32_t beginOffset; // begin of current bucket stride
			uint32_t endOffset;	// end of current bucket stride
			float minCost = std::numeric_limits<float>::max();
			uint32_t bestAxis = 0;

			for (uint32_t j = 0; j < (groupPerBucket / 2); ++j)
			{
				sizes[0] += _groupSize[offset + j];
				sizes[1] += _groupSize[offset + j + (groupPerBucket / 2)];
			}
			
			beginOffset = bucketNodeOffset;
			endOffset = beginOffset + (sizes[0] + sizes[1]);
			bucketNodeOffset = endOffset;

			// try to sort axis by x, y, z to find which sort will give us the best division
			// which produces minimal bounding spheres
			for (uint32_t axis = 0; axis < 3; ++axis)
			{
				float cost;
				
				funcSortByAxis(axis, beginOffset, endOffset);
				cost = funcCalculateCost(_fullTree, _nodeIndices, beginOffset, beginOffset + sizes[0])
					+ funcCalculateCost(_fullTree, _nodeIndices, beginOffset + sizes[0], endOffset);

				if (cost < minCost)
				{
					bestAxis = axis;
					minCost = cost;
				}
			}

			funcSortByAxis(bestAxis, beginOffset, endOffset);
		}
	}
}

void VirtualGeometry::_MergeHierarchyNode(const HierarchyNode& _node1, HierarchyNode& _merged) const
{
	_merged.bounding = _MergeBounds(_node1.bounding, _merged.bounding);
	_merged.error = std::max(_node1.error, _merged.error);
}

void VirtualGeometry::PresetStaticMesh(const StaticMesh& _original)
{
	m_pBaseMesh = &_original;
}

void VirtualGeometry::Init()
{
	_SplitMeshLODs();
	_BuildHierarchy();
}

void VirtualGeometry::GetMeshletsAtLOD(uint32_t _lod, std::vector<Meshlet>& _meshlet) const
{
	CHECK_TRUE(_lod < m_meshlets.size(), "Don't have this LOD");
	_meshlet.reserve(m_meshlets[_lod].size());
	for (const auto& myMeshlet : m_meshlets[_lod])
	{
		_meshlet.push_back(myMeshlet.meshlet);
	}
}

void VirtualGeometry::GetVirtualGeometryDeviceData(
	uint32_t& outRootNodeIndex, 
	std::vector<VirtualGeometry::IntermediateNode>& outHierarchyNodes, 
	std::vector<VirtualGeometry::ClusterGroupData>& outClusterGroupData) const
{
	outRootNodeIndex = m_rootIndex;
	outHierarchyNodes = m_deviceNodes;
	outClusterGroupData = m_deviceGroups;
}

void VirtualGeometry::IntermediateNode::_SetError(float inError)
{
	m_data[8] = inError;
}

void VirtualGeometry::IntermediateNode::_SetBoundingSphere(const glm::vec3& inCenter, float inRadius)
{
	m_data[4] = inCenter.x;
	m_data[5] = inCenter.y;
	m_data[6] = inCenter.z;
	m_data[7] = inRadius;
}

void VirtualGeometry::IntermediateNode::_SetChildrenNodesOrClusterGroup(
	const std::variant<std::array<uint32_t, VG_HIERARCHY_MAX_CHILD>, uint32_t>& inChildren)
{
	bool setChildrenNodes = std::holds_alternative<std::array<uint32_t, VG_HIERARCHY_MAX_CHILD>>(inChildren);

	if (setChildrenNodes)
	{
		const auto& srcData = std::get<std::array<uint32_t, VG_HIERARCHY_MAX_CHILD>>(inChildren);
		memcpy(m_data.data(), srcData.data(), sizeof(std::array<uint32_t, VG_HIERARCHY_MAX_CHILD>));
	}
	else
	{
		m_data[0] = ~0;
		m_data[1] = std::get<uint32_t>(inChildren);
	}
}

float VirtualGeometry::IntermediateNode::GetError() const
{
	float result;
	memcpy(&result, (m_data.data() + 8), sizeof(float));
	return result;
}

void VirtualGeometry::IntermediateNode::GetBoundingSphere(glm::vec3& outCenter, float& outRadius) const
{
	std::array<float, 3> xyz;

	memcpy(xyz.data(), (m_data.data() + 4), sizeof(xyz));
	outCenter = glm::vec3(xyz[0], xyz[1], xyz[2]);
	memcpy(&outRadius, (m_data.data() + 7), sizeof(float));
}

void VirtualGeometry::IntermediateNode::GetChildren(std::array<uint32_t, VG_HIERARCHY_MAX_CHILD>& outChildren) const
{
	memcpy(outChildren.data(), m_data.data(), sizeof(uint32_t) * VG_HIERARCHY_MAX_CHILD);
}

bool VirtualGeometry::IntermediateNode::IsLeaf() const
{
	return m_data[0] == ~0;
}

bool VirtualGeometry::IntermediateNode::ShouldTraverse(float inErrorThreshold) const
{
	return GetError() > inErrorThreshold;
}

uint32_t VirtualGeometry::IntermediateNode::GetClusterGroupDataIndex() const
{
	CHECK_TRUE(IsLeaf(), "Only leaf node has cluster group data index!");
	return m_data[1];
}

void VirtualGeometry::IntermediateNode::GetDataToCopyToDevice(const void*& outSrcPtr, size_t& outSize) const
{
	outSrcPtr = static_cast<const void*>(m_data.data());
	outSize = m_data.size() * sizeof(uint32_t);
}

void VirtualGeometry::ClusterGroupData::GetDataToCopyToDevice(const void*& outSrcPtr, size_t& outSize) const
{
	outSrcPtr = static_cast<const void*>(m_data.data());
	outSize = m_data.size() * sizeof(uint32_t);
}

uint32_t VirtualGeometry::ClusterGroupData::GetClusterChildGroupDataIndex(uint32_t inClusterId) const
{
	return m_childrenGroupDataIndex.at(inClusterId);
}

uint32_t VirtualGeometry::ClusterGroupData::_GetClusterDataOffset(uint32_t inClusterId) const
{
	CHECK_TRUE(inClusterId < GetClusterCount(), "Cluster ID out of range!");

	return 1 + inClusterId * 9;
}

void VirtualGeometry::ClusterGroupData::_SetClusterCount(uint32_t inClusterCount)
{
	inClusterCount = inClusterCount & 0x000000FF;
	m_data[0] = (m_data[0] & 0x00FFFFFF) | (inClusterCount << 24);
}

void VirtualGeometry::ClusterGroupData::_SetClusterData(
	uint32_t inIndex, 
	uint32_t inVertexOffset, 
	uint32_t inVertexCount, 
	uint32_t inTriangleOffset, 
	uint32_t inTriangleCount, 
	const glm::vec3& inBoundingCenter, 
	float inRadius, 
	float inClusterError)
{
	uint32_t offset = _GetClusterDataOffset(inIndex);

	m_data[offset + 0] = inVertexOffset;
	m_data[offset + 2] = inVertexCount;
	m_data[offset + 1] = inTriangleOffset;
	m_data[offset + 3] = inTriangleCount;
	m_data[offset + 4] = inBoundingCenter.x;
	m_data[offset + 5] = inBoundingCenter.y;
	m_data[offset + 6] = inBoundingCenter.z;
	m_data[offset + 7] = inRadius;
	m_data[offset + 8] = inClusterError;
}

void VirtualGeometry::ClusterGroupData::_SetMeshletCompactData(
	const std::vector<uint32_t>& inVertexRemap, 
	const std::vector<uint8_t>& inLocalIndices)
{
	uint32_t vertexCount = (0x00FFFFFF & (inVertexRemap.size()));
	uint32_t index4Count = (inLocalIndices.size() + 3) / 4;
	const std::array<uint32_t, 4> byteMasks = { 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000 };
	
	m_data.resize(289 + vertexCount + index4Count, 0);
	m_data[0] = (m_data[0] & 0xFF000000) | vertexCount;
	std::copy(inVertexRemap.cbegin(), inVertexRemap.cend(), m_data.begin() + 289);
	for (size_t i = 0; i < inLocalIndices.size(); ++i)
	{
		uint32_t currentIndex = inLocalIndices[i];
		uint32_t dataIndex = i / 4 + 289 + vertexCount;
		uint32_t byteIndex = i % 4;

		m_data[dataIndex] = (m_data[dataIndex] & ~(byteMasks[byteIndex])) | (currentIndex << (byteIndex * 8));
	}
}

uint32_t VirtualGeometry::ClusterGroupData::GetClusterCount() const
{
	uint32_t result = m_data[0] >> 24;
	return result;
}

uint32_t VirtualGeometry::ClusterGroupData::GetVertexCount() const
{
	const uint32_t VERTEX_COUNT_MASK = 0x00FFFFFF;
	return m_data[0] & VERTEX_COUNT_MASK;
}

uint32_t VirtualGeometry::ClusterGroupData::GetClusterVertexCount(uint32_t inClusterId) const
{
	uint32_t offset = _GetClusterDataOffset(inClusterId) + 2;
	return m_data[offset];
}

uint32_t VirtualGeometry::ClusterGroupData::GetClusterTriangleCount(uint32_t inClusterId) const
{
	uint32_t offset = _GetClusterDataOffset(inClusterId) + 3;
	return m_data[offset];
}

uint32_t VirtualGeometry::ClusterGroupData::GetClusterMeshVertex(uint32_t inClusterId, uint8_t inLocalIndex) const
{
	uint32_t vertexCount = GetVertexCount();
	uint32_t offset = _GetClusterDataOffset(inClusterId);
	uint32_t clusterVertexOffset = m_data[offset];
	uint32_t vertexIndex = m_data[clusterVertexOffset + static_cast<uint32_t>(inLocalIndex) + 289];
	return vertexIndex;
}

void VirtualGeometry::ClusterGroupData::GetClusterTriangleIndices(
	uint32_t inClusterId, 
	uint32_t inTriangleIndex, 
	uint8_t& outX, 
	uint8_t& outY, 
	uint8_t& outZ) const
{
	uint32_t triangleStrideInBytes = 3; // each triangle use 3 uint8_t
	uint32_t clusterTriangleOffset = m_data[_GetClusterDataOffset(inClusterId) + 1];
	uint32_t triangleDataStart = 289 + GetVertexCount(); // each vertex use 1 uint32_t, plus 289 uint32_t header
	uint32_t byteOffset = triangleStrideInBytes * (clusterTriangleOffset + inTriangleIndex);
	const std::array<uint32_t, 4> byteMasks = { 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000 };
	std::array<uint8_t*, 3> triangleIndices = { &outX, &outY, &outZ };
	for (int i = 0; i < 3; ++i)
	{
		uint32_t dataIndex = (byteOffset + i) / 4;
		uint32_t byteIndex = (byteOffset + i) % 4;

		*triangleIndices[i] = static_cast<uint8_t>((m_data[triangleDataStart + dataIndex] & byteMasks[byteIndex]) >> (byteIndex * 8));
	}
}

void VirtualGeometry::ClusterGroupData::GetClusterBoundingSphere(uint32_t inClusterId, glm::vec3& outCenter, float& outRadius) const
{
	uint32_t offset = _GetClusterDataOffset(inClusterId) + 4;
	std::array<float, 3> xyz;
	memcpy(xyz.data(), (m_data.data() + offset), sizeof(xyz));
	outCenter = glm::vec3(xyz[0], xyz[1], xyz[2]);
	memcpy(&outRadius, (m_data.data() + offset + 3), sizeof(float));
}

float VirtualGeometry::ClusterGroupData::GetClusterError(uint32_t inClusterId) const
{
	uint32_t offset = _GetClusterDataOffset(inClusterId) + 8;
	return m_data[offset];
}


