#pragma once
#include "common.h"
class Camera
{
public:
    unsigned int width, height;  // Screen dimensions
    float near_clip;  // Near clip plane distance
    float far_clip;  // Far clip plane distance
    glm::vec3 eye,      //The position of the camera in world space
            ref,      //The point in world space towards which the camera is pointing
            look,     //The normalized vector from eye to ref. Is also known as the camera's "forward" vector.
            up,       //The normalized vector pointing upwards IN CAMERA SPACE. This vector is perpendicular to LOOK and RIGHT.
            right,    //The normalized vector pointing rightwards IN CAMERA SPACE. It is perpendicular to UP and LOOK.
            world_up; //The normalized vector pointing upwards IN WORLD SPACE. This is primarily used for computing the camera's initial UP vector.

public:
    Camera();
    Camera(unsigned int w, unsigned int h);
    Camera(unsigned int w, unsigned int h, const glm::vec3& e, const glm::vec3& r, const glm::vec3& worldUp);
    Camera(const Camera& c);
    //Computed attributes
    float aspect;
    glm::mat4 GetViewProj()const;
    glm::mat4 GetViewMatrix()const;
    virtual glm::mat4 GetProjectionMatrix()const = 0;
    virtual void RecomputeAttributes();

    void LookAlong(const glm::vec3& dir);
    
    void RotateAboutUp(float deg);
    void RotateAboutWorldUp(float deg);
    void RotateAboutRight(float deg);

    void RotateTheta(float deg);
    void RotatePhi(float deg);

    void TranslateAlongLook(float amt);
    void TranslateAlongRight(float amt);
    void TranslateAlongUp(float amt);

    void MoveTo(const glm::vec3& wPos);

    void Zoom(float amt);

    virtual void Reset();
};

class PersCamera
    :public Camera
{
public:
    float fovy;
    glm::vec3 V,        //Represents the vertical component of the plane of the viewing frustum that passes through the camera's reference point. Used in Camera::Raycast.
              H;        //Represents the horizontal component of the plane of the viewing frustum that passes through the camera's reference point. Used in Camera::Raycast.
public:
    PersCamera();
    PersCamera(unsigned int w, unsigned int h);
    PersCamera(unsigned int w, unsigned int h, const glm::vec3& e, const glm::vec3& r, const glm::vec3& worldUp);
    PersCamera(const PersCamera& c);
    virtual glm::mat4 GetProjectionMatrix()const override;
    virtual void RecomputeAttributes();
    virtual void Reset();
};

class OrthoCamera
    : public Camera
{
public:
    OrthoCamera();
    OrthoCamera(unsigned int w, unsigned int h);
    OrthoCamera(unsigned int w, unsigned int h, const glm::vec3& e, const glm::vec3& r, const glm::vec3& worldUp);
    OrthoCamera(const OrthoCamera& c);
    virtual glm::mat4 GetProjectionMatrix()const override;
};

