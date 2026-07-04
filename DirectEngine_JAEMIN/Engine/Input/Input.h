#pragma once

#include <array>

namespace Engine::Input
{
    enum KeyCode
    {
        KeyW = 'W',
        KeyA = 'A',
        KeyS = 'S',
        KeyD = 'D',
        KeyQ = 'Q',
        KeyE = 'E',
        KeyControl = 17,
        KeyShift = 16,
        KeyAlt = 18,
        KeyEscape = 27,
        KeyDelete = 46
    };

    enum MouseButton
    {
        MouseButtonLeft = 0,
        MouseButtonRight = 1,
        MouseButtonMiddle = 2
    };

    class Input
    {
    public:
        Input();

        void BeginFrame();
        void SetMouseCaptured(bool captured);
        void SetInputBlocked(bool blocked);

        bool GetKey(int keyCode) const;
        bool GetKeyDown(int keyCode) const;
        bool GetKeyUp(int keyCode) const;
        bool GetKeyRaw(int keyCode) const;
        bool GetMouseButton(MouseButton button) const;
        bool GetMouseButtonDown(MouseButton button) const;
        bool GetMouseButtonUp(MouseButton button) const;
        bool GetMouseButtonRaw(MouseButton button) const;
        bool GetMouseButtonDownRaw(MouseButton button) const;
        bool GetMouseButtonUpRaw(MouseButton button) const;
        int GetMouseDeltaX() const;
        int GetMouseDeltaY() const;
        int GetMouseX() const;
        int GetMouseY() const;
        bool IsMouseCaptured() const;
        bool IsInputBlocked() const;

        static void SetActiveInput(Input* input);
        static Input* GetActiveInput();
        static void HandleKeyDown(int keyCode);
        static void HandleKeyUp(int keyCode);
        static void HandleMouseMove(int x, int y);
        static void HandleMouseButtonDown(MouseButton button);
        static void HandleMouseButtonUp(MouseButton button);

    private:
        void OnKeyDown(int keyCode);
        void OnKeyUp(int keyCode);
        void OnMouseMove(int x, int y);
        void OnMouseButtonDown(MouseButton button);
        void OnMouseButtonUp(MouseButton button);

        std::array<bool, 256> m_currentKeys = {};
        std::array<bool, 256> m_previousKeys = {};
        std::array<bool, 3> m_currentMouseButtons = {};
        std::array<bool, 3> m_previousMouseButtons = {};
        int m_mouseDeltaX = 0;
        int m_mouseDeltaY = 0;
        int m_lastMouseX = 0;
        int m_lastMouseY = 0;
        bool m_hasLastMousePosition = false;
        bool m_mouseCaptured = true;
        bool m_inputBlocked = false;
    };
}
