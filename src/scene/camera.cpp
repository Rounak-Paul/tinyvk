/**
 * @file camera.cpp
 * @brief Camera implementation
 */

#include "tinyvk/scene/camera.h"
#include "tinyvk/core/input.h"
#include <glm/gtc/quaternion.hpp>
#include <algorithm>

namespace tvk {

void Camera::SetPerspective(f32 fov_degrees, f32 aspect_ratio, f32 near_plane, f32 far_plane) {
    _projection_type = ProjectionType::Perspective;
    _fov = fov_degrees;
    _aspect_ratio = aspect_ratio;
    _near_plane = near_plane;
    _far_plane = far_plane;
    _projection_dirty = true;
}

void Camera::SetOrthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near_plane, f32 far_plane) {
    _projection_type = ProjectionType::Orthographic;
    _ortho_left = left;
    _ortho_right = right;
    _ortho_bottom = bottom;
    _ortho_top = top;
    _near_plane = near_plane;
    _far_plane = far_plane;
    _projection_dirty = true;
}

void Camera::SetPosition(const Vec3& position) {
    _position = position;
    _view_dirty = true;
}

void Camera::SetRotation(const Quat& rotation) {
    _rotation = rotation;
    _view_dirty = true;
}

void Camera::SetRotationEuler(const Vec3& euler_degrees) {
    Vec3 radians = glm::radians(euler_degrees);
    _rotation = Quat(radians);
    _view_dirty = true;
}

void Camera::LookAt(const Vec3& target, const Vec3& up) {
    Vec3 direction = glm::normalize(target - _position);
    _rotation = glm::quatLookAt(direction, up);
    _view_dirty = true;
}

void Camera::SetAspectRatio(f32 aspect_ratio) {
    _aspect_ratio = aspect_ratio;
    _projection_dirty = true;
}

void Camera::SetFov(f32 fov_degrees) {
    _fov = fov_degrees;
    _projection_dirty = true;
}

void Camera::SetNearPlane(f32 near_plane) {
    _near_plane = near_plane;
    _projection_dirty = true;
}

void Camera::SetFarPlane(f32 far_plane) {
    _far_plane = far_plane;
    _projection_dirty = true;
}

const Mat4& Camera::GetViewMatrix() {
    if (_view_dirty) {
        UpdateViewMatrix();
    }
    return _view_matrix;
}

const Mat4& Camera::GetProjectionMatrix() {
    if (_projection_dirty) {
        UpdateProjectionMatrix();
    }
    return _projection_matrix;
}

Mat4 Camera::GetViewProjectionMatrix() {
    return GetProjectionMatrix() * GetViewMatrix();
}

Vec3 Camera::GetForward() const {
    return glm::normalize(_rotation * Vec3(0.0f, 0.0f, -1.0f));
}

Vec3 Camera::GetRight() const {
    return glm::normalize(_rotation * Vec3(1.0f, 0.0f, 0.0f));
}

Vec3 Camera::GetUp() const {
    return glm::normalize(_rotation * Vec3(0.0f, 1.0f, 0.0f));
}

Vec3 Camera::ScreenToWorldRay(const Vec2& screen_pos, const Vec2& screen_size) const {
    Vec2 ndc;
    ndc.x = (2.0f * screen_pos.x) / screen_size.x - 1.0f;
    ndc.y = 1.0f - (2.0f * screen_pos.y) / screen_size.y;

    Vec4 clip_coords(ndc.x, ndc.y, -1.0f, 1.0f);
    Mat4 inv_proj = glm::inverse(_projection_matrix);
    Vec4 eye_coords = inv_proj * clip_coords;
    eye_coords.z = -1.0f;
    eye_coords.w = 0.0f;

    Mat4 inv_view = glm::inverse(_view_matrix);
    Vec4 world_coords = inv_view * eye_coords;

    return glm::normalize(Vec3(world_coords));
}

Vec3 Camera::WorldToScreen(const Vec3& world_pos, const Vec2& screen_size) const {
    Vec4 clip_coords = _projection_matrix * _view_matrix * Vec4(world_pos, 1.0f);
    
    if (clip_coords.w <= 0.0f) {
        return Vec3(-1.0f);
    }

    Vec3 ndc = Vec3(clip_coords) / clip_coords.w;
    
    Vec3 screen;
    screen.x = (ndc.x + 1.0f) * 0.5f * screen_size.x;
    screen.y = (1.0f - ndc.y) * 0.5f * screen_size.y;
    screen.z = ndc.z;

    return screen;
}

void Camera::UpdateViewMatrix() {
    Mat4 rotation_matrix = glm::mat4_cast(glm::conjugate(_rotation));
    Mat4 translation_matrix = glm::translate(Mat4(1.0f), -_position);
    _view_matrix = rotation_matrix * translation_matrix;
    _view_dirty = false;
}

void Camera::UpdateProjectionMatrix() {
    if (_projection_type == ProjectionType::Perspective) {
        _projection_matrix = glm::perspective(
            glm::radians(_fov),
            _aspect_ratio,
            _near_plane,
            _far_plane
        );
    } else {
        _projection_matrix = glm::ortho(
            _ortho_left,
            _ortho_right,
            _ortho_bottom,
            _ortho_top,
            _near_plane,
            _far_plane
        );
    }
    _projection_matrix[1][1] *= -1.0f;
    _projection_dirty = false;
}

CameraController::CameraController(Camera* camera) : _camera(camera) {
}

void CameraController::Update(f32 delta_time) {
    if (!_camera || !_enabled) return;

    Vec3 position = _camera->GetPosition();
    Vec3 forward = _camera->GetForward();
    Vec3 right = _camera->GetRight();
    Vec3 up = Vec3(0.0f, 1.0f, 0.0f);

    f32 velocity = _move_speed * delta_time;

    if (Input::IsKeyPressed(Key::W)) {
        position += forward * velocity;
    }
    if (Input::IsKeyPressed(Key::S)) {
        position -= forward * velocity;
    }
    if (Input::IsKeyPressed(Key::A)) {
        position -= right * velocity;
    }
    if (Input::IsKeyPressed(Key::D)) {
        position += right * velocity;
    }
    if (Input::IsKeyPressed(Key::Q)) {
        position -= up * velocity;
    }
    if (Input::IsKeyPressed(Key::E)) {
        position += up * velocity;
    }

    if (Input::IsMouseButtonPressed(MouseButton::Right)) {
        Vec2 delta = Input::GetMouseDelta();
        _yaw += delta.x * _rotate_speed;
        SetPitch(_pitch - delta.y * _rotate_speed);
    }

    Vec2 scroll = Input::GetScrollDelta();
    if (scroll.y != 0.0f) {
        position += forward * scroll.y * _zoom_speed;
    }

    _camera->SetPosition(position);

    Vec3 direction;
    direction.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
    direction.y = sin(glm::radians(_pitch));
    direction.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));

    Vec3 target = position + glm::normalize(direction);
    _camera->LookAt(target, up);
}

void CameraController::SetPitch(f32 pitch) {
    _pitch = std::clamp(pitch, -89.0f, 89.0f);
}

} // namespace tvk
