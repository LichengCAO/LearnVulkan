#pragma once
#include "common.h"
class Transform
{
protected:
    glm::mat4 m_modelMat;
    glm::mat4 m_modelMatInvTr;
    glm::vec3 m_pos;
    glm::quat m_rot;
    glm::vec3 m_scale;
	bool m_needUpdate = false;
    void _ComputeModelMatrix(glm::mat4& _modelMat, glm::mat4& _modelMatInvTr) const;
public:
    Transform();
	// set rotation with euler angle
    void SetRotation(float x, float y, float z);
	// set rotation with quaternion
	void SetRotation(float x, float y, float z, float w);
    void SetPosition(float x, float y, float z);
    void SetScale(float x, float y, float z);
    void SetRotation(const glm::vec3& i);
    void SetPosition(const glm::vec3& i);
    void SetScale(const glm::vec3& i);
	glm::mat4 GetModelMatrix()const;
	glm::mat4 GetModelMatrix();
	glm::mat4 GetModelInverseTransposeMatrix()const;
	glm::mat4 GetModelInverseTransposeMatrix();
};

