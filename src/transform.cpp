#include "transform.h"
#include <glm/gtc/matrix_transform.hpp>
void Transform::_ComputeModelMatrix() {
	glm::mat4 scale = glm::scale(glm::mat4(1.), m_scale);
	glm::mat4 rotateX = glm::rotate(glm::mat4(1.), glm::radians(m_rot.x), glm::vec3(1, 0, 0));
	glm::mat4 rotateY = glm::rotate(glm::mat4(1.), glm::radians(m_rot.y), glm::vec3(0, 1, 0));
	glm::mat4 rotateZ = glm::rotate(glm::mat4(1.), glm::radians(m_rot.z), glm::vec3(0, 0, 1));
	glm::mat4 trans = glm::translate(glm::mat4(1.),m_pos);
	m_modelMat = trans * rotateZ * rotateY * rotateX * scale;
	m_modelMatInvTr = (glm::transpose(glm::inverse(m_modelMat)));

	m_modelMatInvTr[3] = { 0, 0, 0, 1 };
}
void Transform::SetRotation(float x, float y, float z) {
	SetRotation(glm::vec3(x, y, z));
}
void Transform::SetPosition(float x, float y, float z) {
	SetPosition(glm::vec3(x, y, z));
}
void Transform::SetScale(float x, float y, float z) {
	SetScale(glm::vec3(x, y, z));
}
void Transform::SetRotation(const glm::vec3& i) {
	m_rot = i;
	_ComputeModelMatrix();
}
void Transform::SetPosition(const glm::vec3& i) {
	m_pos = i;
	_ComputeModelMatrix();
}
void Transform::SetScale(const glm::vec3& i) {
	m_scale = i;
	_ComputeModelMatrix();
}
glm::mat4 Transform::GetModelMatrix()const {
	return m_modelMat;
}
glm::mat4 Transform::GetModelInverseTransposeMatrix()const {
	return m_modelMatInvTr;
}
Transform::Transform()
	:m_modelMat(glm::mat4(1.f)), m_modelMatInvTr(glm::mat4(1.f)),
	m_pos(glm::vec3(0.f)), m_rot(glm::vec3(0.f)), m_scale(glm::vec3(1.f))
{
	_ComputeModelMatrix();
}

