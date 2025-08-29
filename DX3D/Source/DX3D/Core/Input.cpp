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
    // If it was actually down before releasing
    if (m_keyStates[keyCode])
        m_justReleased[keyCode] = true;

    m_keyStates[keyCode] = false;
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

	m_mouseJustPressed.clear();
	m_mouseJustReleased.clear();
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

Vec2 Input::getMousePositionScreen() const
{
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    return Vec2((float)cursorPos.x, (float)cursorPos.y);
}
Vec2 Input::getMousePositionClient() const
{
    POINT cursorPos;
    GetCursorPos(&cursorPos);

    if (m_windowHandle && ScreenToClient(m_windowHandle, &cursorPos))
    {
        return Vec2((float)cursorPos.x, (float)cursorPos.y);
    }

    return Vec2((float)cursorPos.x, (float)cursorPos.y); // fallback
}

Vec2 Input::getMousePositionNDC() const
{
    Vec2 clientPos = getMousePositionClient();

    RECT rect;
    GetClientRect(m_windowHandle, &rect);
    float width = (float)(rect.right - rect.left);
    float height = (float)(rect.bottom - rect.top);

    // Normalize 0..1 (left=0, right=1, top=0, bottom=1)
    float u = clientPos.x / width;
    float v = clientPos.y / height;

    // Flip origin: bottom-right = (0,0), top-right = (1,1)
    v = 1.0f - v;   // flip Y so top=1, bottom=0

    return Vec2(u, v);
}