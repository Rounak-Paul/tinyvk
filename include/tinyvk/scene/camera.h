/**
 * @file camera.h
 * @brief Camera system for 3D rendering
 */

#pragma once

#include "../core/types.h"

namespace tvk {

enum class ProjectionType {
    Perspective,
    Orthographic
};

class Camera {
public:
    Camera() = default;
    ~Camera() = default;

    void SetPerspective(f32 fov_degrees, f32 aspect_ratio, f32 near_plane, f32 far_plane);
    void SetOrthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near_plane, f32 far_plane);

    void SetPosition(const Vec3& position);
    void SetRotation(const Quat& rotation);
    void SetRotationEuler(const Vec3& euler_degrees);

    void LookAt(const Vec3& target, const Vec3& up = Vec3(0.0f, 1.0f, 0.0f));

    void SetAspectRatio(f32 aspect_ratio);
    void SetFov(f32 fov_degrees);
    void SetNearPlane(f32 near_plane);
    void SetFarPlane(f32 far_plane);

    const Mat4& GetViewMatrix();
    const Mat4& GetProjectionMatrix();
    Mat4 GetViewProjectionMatrix();

    Vec3 GetPosition() const { return _position; }
    Quat GetRotation() const { return _rotation; }
    Vec3 GetForward() const;
    Vec3 GetRight() const;
    Vec3 GetUp() const;

    f32 GetFov() const { return _fov; }
    f32 GetAspectRatio() const { return _aspect_ratio; }
    f32 GetNearPlane() const { return _near_plane; }
    f32 GetFarPlane() const { return _far_plane; }
    ProjectionType GetProjectionType() const { return _projection_type; }

    Vec3 ScreenToWorldRay(const Vec2& screen_pos, const Vec2& screen_size) const;
    Vec3 WorldToScreen(const Vec3& world_pos, const Vec2& screen_size) const;

private:
    void UpdateViewMatrix();
    void UpdateProjectionMatrix();

    Vec3 _position = Vec3(0.0f, 0.0f, 0.0f);
    Quat _rotation = Quat(1.0f, 0.0f, 0.0f, 0.0f);

    ProjectionType _projection_type = ProjectionType::Perspective;
    f32 _fov = 45.0f;
    f32 _aspect_ratio = 16.0f / 9.0f;
    f32 _near_plane = 0.1f;
    f32 _far_plane = 1000.0f;

    f32 _ortho_left = -10.0f;
    f32 _ortho_right = 10.0f;
    f32 _ortho_bottom = -10.0f;
    f32 _ortho_top = 10.0f;

    Mat4 _view_matrix = Mat4(1.0f);
    Mat4 _projection_matrix = Mat4(1.0f);

    bool _view_dirty = true;
    bool _projection_dirty = true;
};

class CameraController {
public:
    CameraController() = default;
    explicit CameraController(Camera* camera);

    void SetCamera(Camera* camera) { _camera = camera; }
    Camera* GetCamera() const { return _camera; }

    void Update(f32 delta_time);

    void SetMoveSpeed(f32 speed) { _move_speed = speed; }
    void SetRotateSpeed(f32 speed) { _rotate_speed = speed; }
    void SetZoomSpeed(f32 speed) { _zoom_speed = speed; }

    void SetEnabled(bool enabled) { _enabled = enabled; }
    bool IsEnabled() const { return _enabled; }

    f32 GetYaw() const { return _yaw; }
    f32 GetPitch() const { return _pitch; }
    void SetYaw(f32 yaw) { _yaw = yaw; }
    void SetPitch(f32 pitch);

private:
    Camera* _camera = nullptr;

    f32 _move_speed = 5.0f;
    f32 _rotate_speed = 0.1f;
    f32 _zoom_speed = 2.0f;

    f32 _yaw = -90.0f;
    f32 _pitch = 0.0f;

    bool _enabled = true;
};

} // namespace tvk
