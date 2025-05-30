#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>
Camera::Camera() :
    Camera(400, 400)
{}

Camera::Camera(unsigned int w, unsigned int h) :
    Camera(w, h, glm::vec3(0, 0, 10), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0))
{}

Camera::Camera(unsigned int w, unsigned int h, const glm::vec3& e, const glm::vec3& r, const glm::vec3& worldUp) :
    width(w),
    height(h),
    near_clip(.1f),
    far_clip(300.f),
    eye(e),
    ref(r),
    world_up(worldUp)
{
    RecomputeAttributes();
}

Camera::Camera(const Camera& c) :
    width(c.width),
    height(c.height),
    near_clip(c.near_clip),
    far_clip(c.far_clip),
    aspect(c.aspect),
    eye(c.eye),
    ref(c.ref),
    look(c.look),
    up(c.up),
    right(c.right),
    world_up(c.world_up)
{}

void Camera::RecomputeAttributes()
{
    look = glm::normalize(ref - eye);
    right = glm::normalize(glm::cross(look, world_up));
    up = glm::cross(right, look);
    aspect = width / height;
}

glm::mat4 Camera::GetViewProjectionMatrix()const
{
    return GetProjectionMatrix()*GetViewMatrix();
}

glm::mat4 Camera::GetViewMatrix()const
{
    return glm::lookAt(eye, ref, up);
}

glm::mat4 Camera::GetInverseTransposeViewMatrix() const
{
    return (glm::transpose(glm::inverse(GetViewMatrix())));
}

void Camera::Reset()
{
    eye = glm::vec3(0, 0, 12);
    ref = glm::vec3(0, 0, 0);
    world_up = glm::vec3(0, 1, 0);
    RecomputeAttributes();
}

void Camera::LookAlong(const glm::vec3& dir) {
    ref = eye + dir;
    RecomputeAttributes();
}

void Camera::RotateAboutUp(float deg)
{
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), deg, up);
    ref = ref - eye;
    ref = glm::vec3(rotation * glm::vec4(ref, 1));
    ref = ref + eye;
    RecomputeAttributes();
}
void Camera::RotateAboutWorldUp(float deg) {
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), deg, world_up);
    ref = ref - eye;
    ref = glm::vec3(rotation * glm::vec4(ref, 1));
    ref = ref + eye;
    RecomputeAttributes();
}
void Camera::RotateAboutRight(float deg)
{
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), deg, right);
    ref = ref - eye;
    ref = glm::vec3(rotation * glm::vec4(ref, 1));
    ref = ref + eye;
    RecomputeAttributes();
}

void Camera::RotateTheta(float deg)
{
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), deg, right);
    eye = eye - ref;
    eye = glm::vec3(rotation * glm::vec4(eye, 1.f));
    eye = eye + ref;
    RecomputeAttributes();
}

void Camera::RotatePhi(float deg)
{
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), deg, up);
    eye = eye - ref;
    eye = glm::vec3(rotation * glm::vec4(eye, 1.f));
    eye = eye + ref;
    RecomputeAttributes();
}

void Camera::Zoom(float amt)
{
    glm::vec3 translation = look * amt;
    eye += translation;
}

Frustum Camera::GetFrustum() const
{
	// https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
	Frustum ret{};
	glm::mat4 viewProj = GetViewProjectionMatrix();
	ret.leftPlane   = /*- viewProj[3]*/ - viewProj[0];
	ret.rightPlane  = - viewProj[3] + viewProj[0];
	ret.bottomPlane = - viewProj[3] - viewProj[1];
	ret.topPlane    = - viewProj[3] + viewProj[1];
	ret.nearPlane   = - viewProj[3] - viewProj[2];
	ret.farPlane    = - viewProj[3] + viewProj[2];

	// normalize plane so that the signed distance won't be stretched
	ret.leftPlane   /= glm::dot(glm::vec3(ret.leftPlane), glm::vec3(ret.leftPlane));
	ret.rightPlane  /= glm::dot(glm::vec3(ret.rightPlane), glm::vec3(ret.rightPlane));
	ret.bottomPlane /= glm::dot(glm::vec3(ret.bottomPlane), glm::vec3(ret.bottomPlane));
	ret.topPlane    /= glm::dot(glm::vec3(ret.topPlane), glm::vec3(ret.topPlane));
	ret.nearPlane   /= glm::dot(glm::vec3(ret.nearPlane), glm::vec3(ret.nearPlane));
	ret.farPlane    /= glm::dot(glm::vec3(ret.farPlane), glm::vec3(ret.farPlane));

	return ret;
}

void Camera::TranslateAlongLook(float amt)
{
    glm::vec3 translation = look * amt;
    eye += translation;
    ref += translation;
}
void Camera::TranslateAlongRight(float amt)
{
    glm::vec3 translation = right * amt;
    eye += translation;
    ref += translation;
}
void Camera::TranslateAlongUp(float amt)
{
    glm::vec3 translation = up * amt;
    eye += translation;
    ref += translation;
}
void Camera::MoveTo(const glm::vec3& wPos) {
    glm::vec3 translation = wPos - eye;
    eye = wPos;
    ref += translation;
}

PersCamera::PersCamera() :
    PersCamera(400, 400)
{}
PersCamera::PersCamera(unsigned int w, unsigned int h) :
    PersCamera(w, h, glm::vec3(0, 0, 10), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0))
{}
PersCamera::PersCamera(unsigned int w, unsigned int h, const glm::vec3& e, const glm::vec3& r, const glm::vec3& worldUp)
    :fovy(45), Camera(w, h, e, r, worldUp)
{
    RecomputeAttributes();
}
PersCamera::PersCamera(const PersCamera& c) :
    fovy(c.fovy), H(c.H), V(c.V), Camera(c)
{}
glm::mat4 PersCamera::GetProjectionMatrix()const {
    return glm::perspective(glm::radians(fovy), float(width) / float(height), near_clip, far_clip);
}
void PersCamera::RecomputeAttributes() {
    Camera::RecomputeAttributes();
    float tan_fovy = glm::tan(glm::radians(fovy) / 2);
    float len = glm::length(ref - eye);
    V = up * len * tan_fovy;
    H = right * len * aspect * tan_fovy;
}
void PersCamera::Reset() {
    fovy = 45.f;
    eye = glm::vec3(0, 0, 12);
    ref = glm::vec3(0, 0, 0);
    world_up = glm::vec3(0, 1, 0);
    RecomputeAttributes();
}

float PersCamera::GetInverseTangentHalfFOVy() const
{
    float tan_fovy = glm::tan(glm::radians(fovy) / 2);
    return 1.0f/ tan_fovy;
}

OrthoCamera::OrthoCamera() 
    :OrthoCamera(400,400)
{
    look = glm::vec3(0, 0, -1);
    up = glm::vec3(0, 1, 0);
    right = glm::vec3(1, 0, 0);
}
OrthoCamera::OrthoCamera(unsigned int w, unsigned int h)
    :OrthoCamera(w, h, glm::vec3(0, 0, 10), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0))
{}
OrthoCamera::OrthoCamera(unsigned int w, unsigned int h, const glm::vec3& e, const glm::vec3& r, const glm::vec3& worldUp)
    :Camera(w, h, e, r, worldUp)
{}
OrthoCamera::OrthoCamera(const OrthoCamera& c)
    :Camera(c)
{}
glm::mat4 OrthoCamera::GetProjectionMatrix()const {
    float halfWidth = float(width) / float(2);
    float halfHeight = float(height) / float(2);
    return glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, near_clip, far_clip);
}
