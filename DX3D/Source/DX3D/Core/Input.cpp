#include <DX3D/Core/Input.h>
#include <Windows.h>

using namespace dx3d;

//
// Singleton
//
Input& Input::getInstance()
{
    static Input instance;
    return instance;
}

//
// Keyboard handling
//
void Input::setKeyDown(int keyCode)
{
    m_keyStates[keyCode] = true;

    // Just pressed this frame (if wasn't down previously)
    if (!m_previousKeyStates[keyCode])
        m_justPressed[keyCode] = true;
}

void Input::setKeyUp(int keyCode)
{
    m_keyStates[keyCode] = false;

    // Just released this frame (if was down previously)
    if (m_previousKeyStates[keyCode])
        m_justReleased[keyCode] = true;
}

bool Input::isKeyDown(int keyCode) const
{
    auto it = m_keyStates.find(keyCode);
    return it != m_keyStates.end() && it->second;
}

bool Input::isKeyDown(Key key) const
{
    return isKeyDown(static_cast<int>(key));
}

bool Input::isKeyUp(int keyCode) const
{
    return !isKeyDown(keyCode);
}

bool Input::isKeyUp(Key key) const
{
    return isKeyUp(static_cast<int>(key));
}

bool Input::wasKeyJustPressed(int keyCode) const
{
    auto it = m_justPressed.find(keyCode);
    return it != m_justPressed.end() && it->second;
}

bool Input::wasKeyJustPressed(Key key) const
{
    return wasKeyJustPressed(static_cast<int>(key));
}

bool Input::wasKeyJustReleased(int keyCode) const
{
    auto it = m_justReleased.find(keyCode);
    return it != m_justReleased.end() && it->second;
}

bool Input::wasKeyJustReleased(Key key) const
{
    return wasKeyJustReleased(static_cast<int>(key));
}

//
// Per-frame update
//
void Input::update()
{
    // Save current states for comparison in the next frame
    m_previousKeyStates = m_keyStates;

    // Reset transient states
    m_justPressed.clear();
    m_justReleased.clear();
}

//
// Reset all keys
//
void Input::reset()
{
    m_keyStates.clear();
    m_previousKeyStates.clear();
    m_justPressed.clear();
    m_justReleased.clear();
}

void Input::setMouseDown(MouseClick button)
{
    m_prevMouseStates[button] = m_mouseStates[button];
    m_mouseStates[button] = true;

    if (!m_prevMouseStates[button] && m_mouseStates[button])
        m_mouseJustPressed[button] = true;
}

void Input::setMouseUp(MouseClick button)
{
    m_prevMouseStates[button] = m_mouseStates[button];
    m_mouseStates[button] = false;

    if (m_prevMouseStates[button] && !m_mouseStates[button])
        m_mouseJustReleased[button] = true;
}

bool Input::isMouseDown(MouseClick button) const
{
    auto it = m_mouseStates.find(button);
    return it != m_mouseStates.end() ? it->second : false;
}

bool Input::isMouseUp(MouseClick button) const
{
    return !isMouseDown(button);
}

bool Input::wasMouseJustPressed(MouseClick button) const
{
    auto it = m_mouseJustPressed.find(button);
    return it != m_mouseJustPressed.end() ? it->second : false;
}

bool Input::wasMouseJustReleased(MouseClick button) const
{
    auto it = m_mouseJustReleased.find(button);
    return it != m_mouseJustReleased.end() ? it->second : false;
}

Vec2 Input::getMousePosition() const
{
    POINT cursorPos;
    GetCursorPos(&cursorPos);

    // If we have a window handle, convert to client coordinates
    if (m_windowHandle && ScreenToClient(m_windowHandle, &cursorPos))
    {
        return Vec2((float)cursorPos.x, (float)cursorPos.y);
    }

    // Fallback to screen coordinates
    return Vec2((float)cursorPos.x, (float)cursorPos.y);
}