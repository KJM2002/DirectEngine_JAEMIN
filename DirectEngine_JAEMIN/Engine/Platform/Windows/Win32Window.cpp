#include "Win32Window.h"

#include "Engine/Debug/ImGuiDebugUI.h"
#include "Engine/Input/Input.h"

#include <Windows.h>
#include <windowsx.h>

namespace Engine::Platform
{
    namespace
    {
        constexpr const wchar_t* WindowClassName = L"DirectEngineWindowClass";

        LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
        {
            Win32Window* window = reinterpret_cast<Win32Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            Engine::Debug::ImGuiDebugUI::HandleWin32Message(hwnd, message, wParam, lParam);

            // WM_DESTROY is the single point where the app-level message loop is asked to stop.
            if (message == WM_DESTROY)
            {
                PostQuitMessage(0);
                return 0;
            }

            if (message == WM_CLOSE)
            {
                if (window)
                {
                    window->HandleCloseRequest();
                    return 0;
                }
            }

            if (message == WM_SIZE)
            {
                if (window && wParam != SIZE_MINIMIZED)
                {
                    const std::uint32_t width = static_cast<std::uint32_t>(LOWORD(lParam));
                    const std::uint32_t height = static_cast<std::uint32_t>(HIWORD(lParam));
                    window->HandleResize(width, height);
                }
                return 0;
            }

            if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN)
            {
                Input::Input::HandleKeyDown(static_cast<int>(wParam));
                return 0;
            }

            if (message == WM_KEYUP || message == WM_SYSKEYUP)
            {
                Input::Input::HandleKeyUp(static_cast<int>(wParam));
                return 0;
            }

            if (message == WM_MOUSEMOVE)
            {
                Input::Input::HandleMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
                return 0;
            }

            if (message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN || message == WM_MBUTTONDOWN)
            {
                SetCapture(hwnd);
                const Input::MouseButton button = message == WM_LBUTTONDOWN ? Input::MouseButtonLeft
                    : message == WM_RBUTTONDOWN ? Input::MouseButtonRight
                    : Input::MouseButtonMiddle;
                Input::Input::HandleMouseButtonDown(button);
                return 0;
            }

            if (message == WM_LBUTTONUP || message == WM_RBUTTONUP || message == WM_MBUTTONUP)
            {
                ReleaseCapture();
                const Input::MouseButton button = message == WM_LBUTTONUP ? Input::MouseButtonLeft
                    : message == WM_RBUTTONUP ? Input::MouseButtonRight
                    : Input::MouseButtonMiddle;
                Input::Input::HandleMouseButtonUp(button);
                return 0;
            }

            return DefWindowProc(hwnd, message, wParam, lParam);
        }
    }

    Win32Window::Win32Window() = default;

    Win32Window::~Win32Window()
    {
        Destroy();
    }

    bool Win32Window::Create(const wchar_t* title, std::uint32_t width, std::uint32_t height)
    {
        m_width = width;
        m_height = height;

        HINSTANCE instance = GetModuleHandle(nullptr);

        WNDCLASSEXW windowClass = {};
        windowClass.cbSize = sizeof(WNDCLASSEXW);
        windowClass.lpfnWndProc = WindowProc;
        windowClass.hInstance = instance;
        windowClass.lpszClassName = WindowClassName;
        windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);

        // Registering the same class more than once is harmless for this early single-window app.
        RegisterClassExW(&windowClass);

        RECT rect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

        HWND hwnd = CreateWindowExW(
            0,
            WindowClassName,
            title,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            rect.right - rect.left,
            rect.bottom - rect.top,
            nullptr,
            nullptr,
            instance,
            nullptr);

        if (hwnd == nullptr)
        {
            return false;
        }

        m_hwnd = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        ShowWindow(hwnd, SW_SHOW);
        return true;
    }

    void Win32Window::Destroy()
    {
        if (m_hwnd != nullptr)
        {
            DestroyWindow(static_cast<HWND>(m_hwnd));
            m_hwnd = nullptr;
        }
    }

    bool Win32Window::ProcessMessages()
    {
        MSG message = {};
        // Non-blocking pump keeps the game loop responsible for update/render cadence.
        while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_QUIT)
            {
                return false;
            }

            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        return true;
    }

    void Win32Window::SetTitle(const std::wstring& title)
    {
        if (m_hwnd != nullptr)
        {
            SetWindowTextW(static_cast<HWND>(m_hwnd), title.c_str());
        }
    }

    bool Win32Window::ConsumeResizeEvent(std::uint32_t& outWidth, std::uint32_t& outHeight)
    {
        if (!m_resizePending)
        {
            return false;
        }

        m_resizePending = false;
        outWidth = m_width;
        outHeight = m_height;
        return true;
    }

    bool Win32Window::ConsumeCloseRequest()
    {
        if (!m_closeRequested)
        {
            return false;
        }

        m_closeRequested = false;
        return true;
    }

    void Win32Window::HandleResize(std::uint32_t width, std::uint32_t height)
    {
        if (width == 0 || height == 0 || (width == m_width && height == m_height))
        {
            return;
        }

        m_width = width;
        m_height = height;
        m_resizePending = true;
    }

    void Win32Window::HandleCloseRequest()
    {
        m_closeRequested = true;
    }

    void* Win32Window::GetNativeHandle() const
    {
        return m_hwnd;
    }

    std::uint32_t Win32Window::GetWidth() const
    {
        return m_width;
    }

    std::uint32_t Win32Window::GetHeight() const
    {
        return m_height;
    }
}
