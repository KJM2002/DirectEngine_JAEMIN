#pragma once

namespace Engine::Debug
{
    class ImGuiDebugUI
    {
    public:
        bool Initialize(void* nativeWindowHandle, void* nativeDeviceHandle, void* nativeContextHandle);
        void Shutdown();
        void BeginFrame();
        void EndFrame();
        bool IsAvailable() const;
        static bool HandleWin32Message(void* hwnd, unsigned int message, unsigned long long wParam, long long lParam);

    private:
        bool m_available = false;
    };
}
