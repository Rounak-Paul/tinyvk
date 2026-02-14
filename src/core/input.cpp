/**
 * @file input.cpp
 * @brief Input handling implementation
 */

#include "tinyvk/core/input.h"
#include <cstring>
#include <cmath>

namespace tvk {

void Input::Init(GLFWwindow* window) {
    s_Window = window;
    s_FirstMouse = true;
    s_LastMousePos = GetMousePosition();
    
    memset(s_CurrentKeyState, 0, sizeof(s_CurrentKeyState));
    memset(s_PreviousKeyState, 0, sizeof(s_PreviousKeyState));
    memset(s_CurrentMouseState, 0, sizeof(s_CurrentMouseState));
    memset(s_PreviousMouseState, 0, sizeof(s_PreviousMouseState));
    
    glfwSetScrollCallback(window, ScrollCallback);
}

bool Input::IsKeyPressed(Key key) {
    int keyCode = static_cast<int>(key);
    if (keyCode < 0 || keyCode >= KEY_COUNT) return false;
    return s_CurrentKeyState[keyCode];
}

bool Input::IsKeyDown(Key key) {
    int keyCode = static_cast<int>(key);
    if (keyCode < 0 || keyCode >= KEY_COUNT) return false;
    return s_CurrentKeyState[keyCode] && !s_PreviousKeyState[keyCode];
}

bool Input::IsKeyUp(Key key) {
    int keyCode = static_cast<int>(key);
    if (keyCode < 0 || keyCode >= KEY_COUNT) return false;
    return !s_CurrentKeyState[keyCode] && s_PreviousKeyState[keyCode];
}

bool Input::IsMouseButtonPressed(MouseButton button) {
    int buttonCode = static_cast<int>(button);
    if (buttonCode < 0 || buttonCode >= MOUSE_BUTTON_COUNT) return false;
    return s_CurrentMouseState[buttonCode];
}

bool Input::IsMouseButtonDown(MouseButton button) {
    int buttonCode = static_cast<int>(button);
    if (buttonCode < 0 || buttonCode >= MOUSE_BUTTON_COUNT) return false;
    return s_CurrentMouseState[buttonCode] && !s_PreviousMouseState[buttonCode];
}

bool Input::IsMouseButtonUp(MouseButton button) {
    int buttonCode = static_cast<int>(button);
    if (buttonCode < 0 || buttonCode >= MOUSE_BUTTON_COUNT) return false;
    return !s_CurrentMouseState[buttonCode] && s_PreviousMouseState[buttonCode];
}

Vec2 Input::GetMousePosition() {
    double x, y;
    glfwGetCursorPos(s_Window, &x, &y);
    return {static_cast<float>(x), static_cast<float>(y)};
}

Vec2 Input::GetMouseDelta() {
    return s_MouseDelta;
}

Vec2 Input::GetScrollDelta() {
    return s_ScrollDelta;
}

void Input::SetCursorMode(int mode) {
    glfwSetInputMode(s_Window, GLFW_CURSOR, mode);
}

void Input::Update() {
    memcpy(s_PreviousKeyState, s_CurrentKeyState, sizeof(s_CurrentKeyState));
    memcpy(s_PreviousMouseState, s_CurrentMouseState, sizeof(s_CurrentMouseState));

    for (int i = GLFW_KEY_SPACE; i <= GLFW_KEY_GRAVE_ACCENT; ++i) {
        s_CurrentKeyState[i] = glfwGetKey(s_Window, i) == GLFW_PRESS;
    }
    for (int i = GLFW_KEY_ESCAPE; i <= GLFW_KEY_LAST; ++i) {
        s_CurrentKeyState[i] = glfwGetKey(s_Window, i) == GLFW_PRESS;
    }

    for (int i = 0; i < MOUSE_BUTTON_COUNT; ++i) {
        s_CurrentMouseState[i] = glfwGetMouseButton(s_Window, i) == GLFW_PRESS;
    }

    Vec2 currentPos = GetMousePosition();
    
    if (s_FirstMouse) {
        s_LastMousePos = currentPos;
        s_FirstMouse = false;
    }
    
    s_MouseDelta = currentPos - s_LastMousePos;
    s_LastMousePos = currentPos;
    
    // Detect activity: key state changes, mouse movement, mouse button changes, or scroll
    s_HasActivity = false;
    
    // Check for key state changes
    for (int i = 0; i < KEY_COUNT; ++i) {
        if (s_CurrentKeyState[i] != s_PreviousKeyState[i]) {
            s_HasActivity = true;
            break;
        }
    }
    
    // Check for mouse button changes
    if (!s_HasActivity) {
        for (int i = 0; i < MOUSE_BUTTON_COUNT; ++i) {
            if (s_CurrentMouseState[i] != s_PreviousMouseState[i]) {
                s_HasActivity = true;
                break;
            }
        }
    }
    
    // Check for mouse movement (with small threshold to ignore jitter)
    if (!s_HasActivity) {
        constexpr float MOUSE_THRESHOLD = 0.5f;
        if (std::abs(s_MouseDelta.x) > MOUSE_THRESHOLD || std::abs(s_MouseDelta.y) > MOUSE_THRESHOLD) {
            s_HasActivity = true;
        }
    }
    
    // Scroll delta is checked in HasRecentActivity since it's reset here
    s_ScrollDelta = {0.0f, 0.0f};
}

bool Input::HasRecentActivity() {
    return s_HasActivity || (s_ScrollDelta.x != 0.0f || s_ScrollDelta.y != 0.0f);
}

void Input::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    s_ScrollDelta = {static_cast<float>(xoffset), static_cast<float>(yoffset)};
    s_HasActivity = true;  // Mark activity on scroll
}

} // namespace tvk
