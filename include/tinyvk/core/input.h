/**
 * @file input.h
 * @brief Input handling for TinyVK
 */

#pragma once

#include "types.h"
#include <GLFW/glfw3.h>

namespace tvk {

/**
 * @brief Key codes (mirrors GLFW key codes)
 */
enum class Key : i32 {
    Unknown = GLFW_KEY_UNKNOWN,
    
    // Printable keys
    Space = GLFW_KEY_SPACE,
    Apostrophe = GLFW_KEY_APOSTROPHE,
    Comma = GLFW_KEY_COMMA,
    Minus = GLFW_KEY_MINUS,
    Period = GLFW_KEY_PERIOD,
    Slash = GLFW_KEY_SLASH,
    
    // Numbers
    Num0 = GLFW_KEY_0, Num1 = GLFW_KEY_1, Num2 = GLFW_KEY_2, Num3 = GLFW_KEY_3,
    Num4 = GLFW_KEY_4, Num5 = GLFW_KEY_5, Num6 = GLFW_KEY_6, Num7 = GLFW_KEY_7,
    Num8 = GLFW_KEY_8, Num9 = GLFW_KEY_9,
    
    Semicolon = GLFW_KEY_SEMICOLON,
    Equal = GLFW_KEY_EQUAL,
    
    // Letters
    A = GLFW_KEY_A, B = GLFW_KEY_B, C = GLFW_KEY_C, D = GLFW_KEY_D,
    E = GLFW_KEY_E, F = GLFW_KEY_F, G = GLFW_KEY_G, H = GLFW_KEY_H,
    I = GLFW_KEY_I, J = GLFW_KEY_J, K = GLFW_KEY_K, L = GLFW_KEY_L,
    M = GLFW_KEY_M, N = GLFW_KEY_N, O = GLFW_KEY_O, P = GLFW_KEY_P,
    Q = GLFW_KEY_Q, R = GLFW_KEY_R, S = GLFW_KEY_S, T = GLFW_KEY_T,
    U = GLFW_KEY_U, V = GLFW_KEY_V, W = GLFW_KEY_W, X = GLFW_KEY_X,
    Y = GLFW_KEY_Y, Z = GLFW_KEY_Z,
    
    LeftBracket = GLFW_KEY_LEFT_BRACKET,
    Backslash = GLFW_KEY_BACKSLASH,
    RightBracket = GLFW_KEY_RIGHT_BRACKET,
    GraveAccent = GLFW_KEY_GRAVE_ACCENT,
    
    // Function keys
    Escape = GLFW_KEY_ESCAPE,
    Enter = GLFW_KEY_ENTER,
    Tab = GLFW_KEY_TAB,
    Backspace = GLFW_KEY_BACKSPACE,
    Insert = GLFW_KEY_INSERT,
    Delete = GLFW_KEY_DELETE,
    Right = GLFW_KEY_RIGHT,
    Left = GLFW_KEY_LEFT,
    Down = GLFW_KEY_DOWN,
    Up = GLFW_KEY_UP,
    PageUp = GLFW_KEY_PAGE_UP,
    PageDown = GLFW_KEY_PAGE_DOWN,
    Home = GLFW_KEY_HOME,
    End = GLFW_KEY_END,
    CapsLock = GLFW_KEY_CAPS_LOCK,
    ScrollLock = GLFW_KEY_SCROLL_LOCK,
    NumLock = GLFW_KEY_NUM_LOCK,
    PrintScreen = GLFW_KEY_PRINT_SCREEN,
    Pause = GLFW_KEY_PAUSE,
    
    F1 = GLFW_KEY_F1, F2 = GLFW_KEY_F2, F3 = GLFW_KEY_F3, F4 = GLFW_KEY_F4,
    F5 = GLFW_KEY_F5, F6 = GLFW_KEY_F6, F7 = GLFW_KEY_F7, F8 = GLFW_KEY_F8,
    F9 = GLFW_KEY_F9, F10 = GLFW_KEY_F10, F11 = GLFW_KEY_F11, F12 = GLFW_KEY_F12,
    
    // Modifiers
    LeftShift = GLFW_KEY_LEFT_SHIFT,
    LeftControl = GLFW_KEY_LEFT_CONTROL,
    LeftAlt = GLFW_KEY_LEFT_ALT,
    LeftSuper = GLFW_KEY_LEFT_SUPER,
    RightShift = GLFW_KEY_RIGHT_SHIFT,
    RightControl = GLFW_KEY_RIGHT_CONTROL,
    RightAlt = GLFW_KEY_RIGHT_ALT,
    RightSuper = GLFW_KEY_RIGHT_SUPER,
    Menu = GLFW_KEY_MENU
};

/**
 * @brief Mouse button codes
 */
enum class MouseButton : i32 {
    Left = GLFW_MOUSE_BUTTON_LEFT,
    Right = GLFW_MOUSE_BUTTON_RIGHT,
    Middle = GLFW_MOUSE_BUTTON_MIDDLE,
    Button4 = GLFW_MOUSE_BUTTON_4,
    Button5 = GLFW_MOUSE_BUTTON_5,
    Button6 = GLFW_MOUSE_BUTTON_6,
    Button7 = GLFW_MOUSE_BUTTON_7,
    Button8 = GLFW_MOUSE_BUTTON_8
};

/**
 * @brief Input handling class
 */
class Input {
public:
    /**
     * @brief Initialize input system with window
     */
    static void Init(GLFWwindow* window);

    /**
     * @brief Check if a key is currently pressed
     */
    static bool IsKeyPressed(Key key);

    /**
     * @brief Check if a key was just pressed this frame
     */
    static bool IsKeyDown(Key key);

    /**
     * @brief Check if a key was just released this frame
     */
    static bool IsKeyUp(Key key);

    /**
     * @brief Check if a mouse button is pressed
     */
    static bool IsMouseButtonPressed(MouseButton button);

    /**
     * @brief Get current mouse position
     */
    static Vec2 GetMousePosition();

    /**
     * @brief Get mouse delta since last frame
     */
    static Vec2 GetMouseDelta();

    /**
     * @brief Get scroll wheel delta
     */
    static Vec2 GetScrollDelta();

    /**
     * @brief Set cursor mode (normal, hidden, disabled)
     */
    static void SetCursorMode(int mode);

    /**
     * @brief Update input state (call once per frame)
     */
    static void Update();

private:
    static inline GLFWwindow* s_Window = nullptr;
    static inline Vec2 s_LastMousePos = {0.0f, 0.0f};
    static inline Vec2 s_MouseDelta = {0.0f, 0.0f};
    static inline Vec2 s_ScrollDelta = {0.0f, 0.0f};
    static inline bool s_FirstMouse = true;

    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
};

} // namespace tvk
