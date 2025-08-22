#pragma once
#include <unordered_map>
#include <Windows.h>
#include <DX3D/Math/Geometry.h>

namespace dx3d {
    enum class MouseClick
    {
        LeftMouse = WM_LBUTTONDOWN,
        RightMouse = WM_RBUTTONDOWN,
        MiddleMouse = WM_MBUTTONDOWN
    };
    // Key codes enum for better readability (optional, you can still use raw codes)
    enum class Key {
        // Letters
        A = 0x41, B = 0x42, C = 0x43, D = 0x44,
        E = 0x45, F = 0x46, G = 0x47, H = 0x48,
        I = 0x49, J = 0x4A, K = 0x4B, L = 0x4C,
        M = 0x4D, N = 0x4E, O = 0x4F, P = 0x50,
        Q = 0x51, R = 0x52, S = 0x53, T = 0x54,
        U = 0x55, V = 0x56, W = 0x57, X = 0x58,
        Y = 0x59, Z = 0x5A,

        // Numbers (top row)
        Num0 = 0x30, Num1 = 0x31, Num2 = 0x32, Num3 = 0x33,
        Num4 = 0x34, Num5 = 0x35, Num6 = 0x36, Num7 = 0x37,
        Num8 = 0x38, Num9 = 0x39,

        // Function keys
        F1 = VK_F1, F2 = VK_F2, F3 = VK_F3, F4 = VK_F4,
        F5 = VK_F5, F6 = VK_F6, F7 = VK_F7, F8 = VK_F8,
        F9 = VK_F9, F10 = VK_F10, F11 = VK_F11, F12 = VK_F12,

        // Modifiers
        Shift = VK_SHIFT,
        Control = VK_CONTROL,
        Alt = VK_MENU,

        // Navigation
        Left = VK_LEFT,
        Right = VK_RIGHT,
        Up = VK_UP,
        Down = VK_DOWN,
        Home = VK_HOME,
        End = VK_END,
        PageUp = VK_PRIOR,
        PageDown = VK_NEXT,
        Insert = VK_INSERT,
        Delete = VK_DELETE,

        // Misc
        Space = VK_SPACE,
        Escape = VK_ESCAPE,
        Enter = VK_RETURN,
        Tab = VK_TAB,
        Backspace = VK_BACK,
        CapsLock = VK_CAPITAL,

        // Numpad
        Numpad0 = VK_NUMPAD0, Numpad1 = VK_NUMPAD1, Numpad2 = VK_NUMPAD2,
        Numpad3 = VK_NUMPAD3, Numpad4 = VK_NUMPAD4, Numpad5 = VK_NUMPAD5,
        Numpad6 = VK_NUMPAD6, Numpad7 = VK_NUMPAD7, Numpad8 = VK_NUMPAD8,
        Numpad9 = VK_NUMPAD9,
        NumpadAdd = VK_ADD,
        NumpadSubtract = VK_SUBTRACT,
        NumpadMultiply = VK_MULTIPLY,
        NumpadDivide = VK_DIVIDE,
        NumpadDecimal = VK_DECIMAL,
    };


    class Input {
    public:
        static Input& getInstance();
        void setWindowHandle(HWND hwnd) { m_windowHandle = hwnd; }
        // Called by the game engine when keys are pressed/released
        void setKeyDown(int keyCode);
        void setKeyUp(int keyCode);

        // Query methods for scenes/systems to use
        bool isKeyDown(int keyCode) const;
        bool isKeyDown(Key key) const;

        bool isKeyUp(int keyCode) const;
        bool isKeyUp(Key key) const;

        // Check if key was just pressed this frame (useful for single-press actions)
        bool wasKeyJustPressed(int keyCode) const;
        bool wasKeyJustPressed(Key key) const;

        // Check if key was just released this frame
        bool wasKeyJustReleased(int keyCode) const;
        bool wasKeyJustReleased(Key key) const;

        // Called once per frame to update "just pressed/released" state
        void update();

        // Utility methods
        void reset(); // Clear all input states

        
        // Mouse
        void setMouseDown(MouseClick button);
        void setMouseUp(MouseClick button);

        bool isMouseDown(MouseClick button) const;
        bool isMouseUp(MouseClick button) const;

        bool wasMouseJustPressed(MouseClick button) const;
        bool wasMouseJustReleased(MouseClick button) const;

        Vec2 getMousePosition() const;


    private:
        HWND m_windowHandle = nullptr;
        Input() = default;
        ~Input() = default;
        Input(const Input&) = delete;
        Input& operator=(const Input&) = delete;

        std::unordered_map<int, bool> m_keyStates;           // Current frame key states
        std::unordered_map<int, bool> m_previousKeyStates;   // Previous frame key states
        std::unordered_map<int, bool> m_justPressed;         // Keys that were just pressed
        std::unordered_map<int, bool> m_justReleased;        // Keys that were just released

        std::unordered_map<MouseClick, bool> m_mouseStates;
        std::unordered_map<MouseClick, bool> m_prevMouseStates;
        std::unordered_map<MouseClick, bool> m_mouseJustPressed;
        std::unordered_map<MouseClick, bool> m_mouseJustReleased;
    };

    // Convenience macros for common usage patterns
#define INPUT_KEY_DOWN(key) dx3d::Input::getInstance().isKeyDown(key)
#define INPUT_KEY_UP(key) dx3d::Input::getInstance().isKeyUp(key)
#define INPUT_KEY_JUST_PRESSED(key) dx3d::Input::getInstance().wasKeyJustPressed(key)
#define INPUT_KEY_JUST_RELEASED(key) dx3d::Input::getInstance().wasKeyJustReleased(key)
}