#include "Input.h"

#include <Windows.h>

namespace Engine::Input
{
    namespace
    {
        Input* g_activeInput = nullptr;

        bool IsValidKeyCode(int keyCode)
        {
            return keyCode >= 0 && keyCode < 256;
        }
    }

    Input::Input()
    {
        SetActiveInput(this);
    }

    void Input::BeginFrame()
    {
        m_previousKeys = m_currentKeys;
        m_previousMouseButtons = m_currentMouseButtons;
        m_mouseDeltaX = 0;
        m_mouseDeltaY = 0;
    }

    void Input::SetMouseCaptured(bool captured)
    {
        if (m_mouseCaptured == captured)
        {
            return;
        }

        m_mouseCaptured = captured;
        m_hasLastMousePosition = false;
        ShowCursor(captured ? FALSE : TRUE);
    }

    void Input::SetInputBlocked(bool blocked)
    {
        m_inputBlocked = blocked;
    }

    bool Input::GetKey(int keyCode) const
    {
        return !m_inputBlocked && IsValidKeyCode(keyCode) && m_currentKeys[static_cast<std::size_t>(keyCode)];
    }

    bool Input::GetKeyDown(int keyCode) const
    {
        return !m_inputBlocked && IsValidKeyCode(keyCode)
            && m_currentKeys[static_cast<std::size_t>(keyCode)]
            && !m_previousKeys[static_cast<std::size_t>(keyCode)];
    }

    bool Input::GetKeyUp(int keyCode) const
    {
        return !m_inputBlocked && IsValidKeyCode(keyCode)
            && !m_currentKeys[static_cast<std::size_t>(keyCode)]
            && m_previousKeys[static_cast<std::size_t>(keyCode)];
    }

    bool Input::GetKeyRaw(int keyCode) const
    {
        return IsValidKeyCode(keyCode) && m_currentKeys[static_cast<std::size_t>(keyCode)];
    }

    bool Input::GetMouseButton(MouseButton button) const
    {
        return !m_inputBlocked && m_currentMouseButtons[static_cast<std::size_t>(button)];
    }

    bool Input::GetMouseButtonRaw(MouseButton button) const
    {
        return m_currentMouseButtons[static_cast<std::size_t>(button)];
    }

    bool Input::GetMouseButtonDownRaw(MouseButton button) const
    {
        const std::size_t index = static_cast<std::size_t>(button);
        return m_currentMouseButtons[index] && !m_previousMouseButtons[index];
    }

    bool Input::GetMouseButtonUpRaw(MouseButton button) const
    {
        const std::size_t index = static_cast<std::size_t>(button);
        return !m_currentMouseButtons[index] && m_previousMouseButtons[index];
    }

    bool Input::GetMouseButtonDown(MouseButton button) const
    {
        const std::size_t index = static_cast<std::size_t>(button);
        return !m_inputBlocked && m_currentMouseButtons[index] && !m_previousMouseButtons[index];
    }

    bool Input::GetMouseButtonUp(MouseButton button) const
    {
        const std::size_t index = static_cast<std::size_t>(button);
        return !m_inputBlocked && !m_currentMouseButtons[index] && m_previousMouseButtons[index];
    }

    int Input::GetMouseDeltaX() const
    {
        return m_mouseDeltaX;
    }

    int Input::GetMouseDeltaY() const
    {
        return m_mouseDeltaY;
    }

    int Input::GetMouseX() const
    {
        return m_lastMouseX;
    }

    int Input::GetMouseY() const
    {
        return m_lastMouseY;
    }

    bool Input::IsMouseCaptured() const
    {
        return m_mouseCaptured;
    }

    bool Input::IsInputBlocked() const
    {
        return m_inputBlocked;
    }

    void Input::SetActiveInput(Input* input)
    {
        g_activeInput = input;
    }

    Input* Input::GetActiveInput()
    {
        return g_activeInput;
    }

    void Input::HandleKeyDown(int keyCode)
    {
        if (g_activeInput)
        {
            g_activeInput->OnKeyDown(keyCode);
        }
    }

    void Input::HandleKeyUp(int keyCode)
    {
        if (g_activeInput)
        {
            g_activeInput->OnKeyUp(keyCode);
        }
    }

    void Input::HandleMouseMove(int x, int y)
    {
        if (g_activeInput)
        {
            g_activeInput->OnMouseMove(x, y);
        }
    }

    void Input::HandleMouseButtonDown(MouseButton button)
    {
        if (g_activeInput)
        {
            g_activeInput->OnMouseButtonDown(button);
        }
    }

    void Input::HandleMouseButtonUp(MouseButton button)
    {
        if (g_activeInput)
        {
            g_activeInput->OnMouseButtonUp(button);
        }
    }

    void Input::OnKeyDown(int keyCode)
    {
        if (IsValidKeyCode(keyCode))
        {
            m_currentKeys[static_cast<std::size_t>(keyCode)] = true;
        }
    }

    void Input::OnKeyUp(int keyCode)
    {
        if (IsValidKeyCode(keyCode))
        {
            m_currentKeys[static_cast<std::size_t>(keyCode)] = false;
        }
    }

    void Input::OnMouseMove(int x, int y)
    {
        if (m_hasLastMousePosition)
        {
            m_mouseDeltaX += x - m_lastMouseX;
            m_mouseDeltaY += y - m_lastMouseY;
        }

        m_lastMouseX = x;
        m_lastMouseY = y;
        m_hasLastMousePosition = true;
    }

    void Input::OnMouseButtonDown(MouseButton button)
    {
        m_currentMouseButtons[static_cast<std::size_t>(button)] = true;
    }

    void Input::OnMouseButtonUp(MouseButton button)
    {
        m_currentMouseButtons[static_cast<std::size_t>(button)] = false;
    }
}
