#pragma once

#include <cstdint>
#include <string>

namespace Engine::Platform
{
    // Thin Win32 wrapper. The rest of the engine sees only an opaque native handle.
    class Win32Window
    {
    public:
        Win32Window();
        ~Win32Window();

        bool Create(const wchar_t* title, std::uint32_t width, std::uint32_t height);
        void Destroy();
        bool ProcessMessages();
        void SetTitle(const std::wstring& title);
        bool ConsumeResizeEvent(std::uint32_t& outWidth, std::uint32_t& outHeight);
        bool ConsumeCloseRequest();
        void HandleResize(std::uint32_t width, std::uint32_t height);
        void HandleCloseRequest();

        void* GetNativeHandle() const;
        std::uint32_t GetWidth() const;
        std::uint32_t GetHeight() const;

    private:
        void* m_hwnd = nullptr;
        std::uint32_t m_width = 0;
        std::uint32_t m_height = 0;
        bool m_resizePending = false;
        bool m_closeRequested = false;
    };
}
