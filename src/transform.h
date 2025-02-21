#pragma once
#include "common.h"
class Transform
{
protected:
    glm::mat4 m_modelMat;
    glm::mat3 m_modelMatInvTr;
    glm::vec3 m_pos;
    glm::vec3 m_rot;
    glm::vec3 m_scale;
    void _ComputeModelMatrix();
public:
    Transform();
    void SetRotation(float x, float y, float z);
    void SetPosition(float x, float y, float z);
    void SetScale(float x, float y, float z);
    void SetRotation(const glm::vec3& i);
    void SetPosition(const glm::vec3& i);
    void SetScale(const glm::vec3& i);
    glm::mat4 GetModelMatrix()const;
    glm::mat3 GetModelInverseTransposeMatrix()const;
};

