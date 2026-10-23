#include <engine/rendering/FPSCamera.hpp>

namespace engine {

Matrix FPSCamera::GetProjectionMatrix() const {
    // Engine uses ReverseZ, so planes are swapped
    return Matrix::CreatePerspectiveFieldOfView(fov, aspectRatio, farPlane, nearPlane);
}

Matrix FPSCamera::GetViewMatrix() const {
    const auto rotationMatrix = Matrix::CreateFromYawPitchRoll(yaw, pitch, 0.0f);
    const auto forward = Vector3::Transform(Vector3::Forward, rotationMatrix);
    const auto focusPosition = position + forward;

    return Matrix::CreateLookAt(position, focusPosition, Vector3::Up);
}

}
