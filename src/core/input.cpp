/**
 * @file input.cpp
 * @brief Input handling implementation
 */

#include "tinyvk/core/input.h"

namespace tvk {

void Input::Init(GLFWwindow* window) {
    s_Window = window;
    s_FirstMouse = true;
    s_LastMousePos = GetMousePosition();
    
    glfwSetScrollCallback(window, ScrollCallback);
}

bool Input::IsKeyPressed(Key key) {
    int state = glfwGetKey(s_Window, static_cast<int>(key));
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool Input::IsKeyDown(Key key) {
    // Note: For proper implementation, you'd need to track previous frame state
    return glfwGetKey(s_Window, static_cast<int>(key)) == GLFW_PRESS;
}

bool Input::IsKeyUp(Key key) {
    // Note: For proper implementation, you'd need to track previous frame state
    return glfwGetKey(s_Window, static_cast<int>(key)) == GLFW_RELEASE;
}

bool Input::IsMouseButtonPressed(MouseButton button) {
    int state = glfwGetMouseButton(s_Window, static_cast<int>(button));
    return state == GLFW_PRESS;
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
    Vec2 currentPos = GetMousePosition();
    
    if (s_FirstMouse) {
        s_LastMousePos = currentPos;
        s_FirstMouse = false;
    }
    
    s_MouseDelta = currentPos - s_LastMousePos;
    s_LastMousePos = currentPos;
    
    // Reset scroll delta each frame
    s_ScrollDelta = {0.0f, 0.0f};
}

void Input::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    s_ScrollDelta = {static_cast<float>(xoffset), static_cast<float>(yoffset)};
}

} // namespace tvk
