#include "transform.h"
#include <glm/gtc/matrix_transform.hpp>
void Transform::_ComputeModelMatrix(glm::mat4& _modelMat, glm::mat4& _modelMatInvTr) const {
	glm::mat4 scale = glm::scale(glm::mat4(1.), m_scale);
	glm::mat4 rot = glm::mat4_cast(m_rot);
	glm::mat4 trans = glm::translate(glm::mat4(1.),m_pos);
	
	_modelMat = trans * rot * scale;
	_modelMatInvTr = (glm::transpose(glm::inverse(m_modelMat)));
	_modelMatInvTr[3] = { 0, 0, 0, 1 };
}
void Transform::SetRotation(float x, float y, float z) {
	SetRotation(glm::vec3(x, y, z));
}
void Transform::SetRotation(float x, float y, float z, float w)
{
	m_rot = glm::quat(w, x, y, z);
	m_needUpdate = true;
}
void Transform::SetPosition(float x, float y, float z) {
	SetPosition(glm::vec3(x, y, z));
}
void Transform::SetScale(float x, float y, float z) {
	SetScale(glm::vec3(x, y, z));
}
void Transform::SetRotation(const glm::vec3& i) {
	glm::vec3 eulerAngles(glm::radians(i.x), glm::radians(i.y), glm::radians(i.z));
	// Convert Euler angles to quaternion
	m_rot = glm::quat(eulerAngles);
	m_needUpdate = true;
}
void Transform::SetPosition(const glm::vec3& i) {
	m_pos = i;
	m_needUpdate = true;
}
void Transform::SetScale(const glm::vec3& i) {
	m_scale = i;
	m_needUpdate = true;
}
glm::mat4 Transform::GetModelMatrix()const {
	glm::mat4 ret;
	glm::mat4 ret2;
	_ComputeModelMatrix(ret, ret2);
	return ret;
}
glm::mat4 Transform::GetModelMatrix()
{
	if (m_needUpdate)
	{
		_ComputeModelMatrix(m_modelMat, m_modelMatInvTr);
		m_needUpdate = false;
	}
	return m_modelMat;
}
glm::mat4 Transform::GetModelInverseTransposeMatrix()const {
	glm::mat4 ret;
	glm::mat4 ret2;
	_ComputeModelMatrix(ret, ret2);
	return ret2;
}
glm::mat4 Transform::GetModelInverseTransposeMatrix()
{
	if (m_needUpdate)
	{
		_ComputeModelMatrix(m_modelMat, m_modelMatInvTr);
		m_needUpdate = false;
	}
	return m_modelMatInvTr;
}
Transform::Transform()
	:m_modelMat(glm::mat4(1.f)), m_modelMatInvTr(glm::mat4(1.f)),
	m_pos(glm::vec3(0.f)), m_rot(glm::vec3(0.f)), m_scale(glm::vec3(1.f)), m_needUpdate(true)
{
}

