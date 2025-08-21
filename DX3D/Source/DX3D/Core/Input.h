#pragma once
#include <unordered_map>
#include <Windows.h>

namespace dx3d {

    // Key codes enum for better readability (optional, you can still use raw codes)
    enum class Key {
        W = 0x57,
        A = 0x41,
        S = 0x53,
        D = 0x44,
        Q = 0x51,
        E = 0x45,
        Shift = VK_SHIFT,
        Space = VK_SPACE,
        Escape = VK_ESCAPE,
        Enter = VK_RETURN,
        Tab = VK_TAB,
        // Add more as needed
    };

    class Input {
    public:
        static Input& getInstance();

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

    private:
        Input() = default;
        ~Input() = default;
        Input(const Input&) = delete;
        Input& operator=(const Input&) = delete;

        std::unordered_map<int, bool> m_keyStates;           // Current frame key states
        std::unordered_map<int, bool> m_previousKeyStates;   // Previous frame key states
        std::unordered_map<int, bool> m_justPressed;         // Keys that were just pressed
        std::unordered_map<int, bool> m_justReleased;        // Keys that were just released
    };

    // Convenience macros for common usage patterns
#define INPUT_KEY_DOWN(key) dx3d::Input::getInstance().isKeyDown(key)
#define INPUT_KEY_UP(key) dx3d::Input::getInstance().isKeyUp(key)
#define INPUT_KEY_JUST_PRESSED(key) dx3d::Input::getInstance().wasKeyJustPressed(key)
#define INPUT_KEY_JUST_RELEASED(key) dx3d::Input::getInstance().wasKeyJustReleased(key)
}