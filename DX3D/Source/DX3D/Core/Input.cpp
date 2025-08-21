#include <DX3D/Core/Input.h>

namespace dx3d {

    Input& Input::getInstance() {
        static Input instance;
        return instance;
    }

    void Input::setKeyDown(int keyCode) {
        // Store previous state before updating
        m_previousKeyStates[keyCode] = m_keyStates[keyCode];
        m_keyStates[keyCode] = true;

        // Check if this is a new press (wasn't down before, now is down)
        if (!m_previousKeyStates[keyCode] && m_keyStates[keyCode]) {
            m_justPressed[keyCode] = true;
        }
    }

    void Input::setKeyUp(int keyCode) {
        // Store previous state before updating
        m_previousKeyStates[keyCode] = m_keyStates[keyCode];
        m_keyStates[keyCode] = false;

        // Check if this is a new release (was down before, now is up)
        if (m_previousKeyStates[keyCode] && !m_keyStates[keyCode]) {
            m_justReleased[keyCode] = true;
        }
    }

    bool Input::isKeyDown(int keyCode) const {
        auto it = m_keyStates.find(keyCode);
        return it != m_keyStates.end() ? it->second : false;
    }

    bool Input::isKeyDown(Key key) const {
        return isKeyDown(static_cast<int>(key));
    }

    bool Input::isKeyUp(int keyCode) const {
        return !isKeyDown(keyCode);
    }

    bool Input::isKeyUp(Key key) const {
        return !isKeyDown(key);
    }

    bool Input::wasKeyJustPressed(int keyCode) const {
        auto it = m_justPressed.find(keyCode);
        return it != m_justPressed.end() ? it->second : false;
    }

    bool Input::wasKeyJustPressed(Key key) const {
        return wasKeyJustPressed(static_cast<int>(key));
    }

    bool Input::wasKeyJustReleased(int keyCode) const {
        auto it = m_justReleased.find(keyCode);
        return it != m_justReleased.end() ? it->second : false;
    }

    bool Input::wasKeyJustReleased(Key key) const {
        return wasKeyJustReleased(static_cast<int>(key));
    }

    void Input::update() {
        // Clear the "just pressed" and "just released" states for next frame
        // These should only be true for one frame
        m_justPressed.clear();
        m_justReleased.clear();

        // Update previous states for next frame
        // (This is already handled in setKeyDown/setKeyUp, but we could do it here instead)
    }

    void Input::reset() {
        m_keyStates.clear();
        m_previousKeyStates.clear();
        m_justPressed.clear();
        m_justReleased.clear();
    }
}