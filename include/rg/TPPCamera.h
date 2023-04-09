#ifndef TPPCAMERA_H
#define TPPCAMERA_H

#include <learnopengl/camera.h>


// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class TPPCamera : public Camera
{
public:
    glm::vec3 Target;
    // constructor with vectors
    TPPCamera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
              float yaw = YAW, float pitch = PITCH)
    : Camera(position, up, yaw, pitch), Target(target)
    {
    }
    // constructor with scalar values
    TPPCamera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
    : Camera(posX, posY, posZ, upX, upY, upZ, yaw, pitch)
    {

    }

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix() override
    {
        return glm::lookAt(Position, Target+glm::vec3(0.f, .7f, 0.f), Up);
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;
        Pitch += yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (Pitch > 44.0f)
                Pitch = 44.0f;
            if (Pitch < -50.0f)
                Pitch = -50.0f;
        }

        // update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors(Target);
    }

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset)
    {
        Distance -= (float)yoffset*0.5;
        if (Distance < 10.0f)
            Distance = 10.0f;
        if (Distance > 14.0f)
            Distance = 14.0f;

        updateCameraVectors(Target);
    }

    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors(glm::vec3 target) override
    {
        Target = target;
        // calculate the new Front vector
        glm::vec3 pos;
        pos.x = Distance * cos(glm::radians(Yaw)) * cos(glm::radians(Pitch)) + Target.x;
        pos.z = Distance * sin(glm::radians(Yaw)) * cos(glm::radians(Pitch)) + Target.z;
        pos.y = sin(glm::radians(Pitch)) + Target.y;
        Position = pos;
        Front = Target - Position;
        // also re-calculate the Right and Up vector
        Right = glm::normalize(glm::cross(Front, WorldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        Up    = glm::normalize(glm::cross(Right, Front));
    }
};
#endif