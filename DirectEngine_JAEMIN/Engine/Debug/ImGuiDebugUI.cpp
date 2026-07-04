#include "ImGuiDebugUI.h"

#include "Engine/Core/Log.h"

#include "imgui.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"

#include <Windows.h>
#include <d3d11.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Engine::Debug
{
    bool ImGuiDebugUI::Initialize(void* nativeWindowHandle, void* nativeDeviceHandle, void* nativeContextHandle)
    {
        if (!nativeWindowHandle || !nativeDeviceHandle || !nativeContextHandle)
        {
            Core::Log::Error("Failed to initialize ImGui: native handles are missing.");
            return false;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui::StyleColorsDark();

        const bool win32Initialized = ImGui_ImplWin32_Init(nativeWindowHandle);
        const bool dx11Initialized = ImGui_ImplDX11_Init(
            static_cast<ID3D11Device*>(nativeDeviceHandle),
            static_cast<ID3D11DeviceContext*>(nativeContextHandle));

        if (!win32Initialized || !dx11Initialized)
        {
            Core::Log::Error("Failed to initialize ImGui backend.");
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            return false;
        }

        m_available = true;
        Core::Log::Info("ImGui debug UI initialized.");
        return true;
    }

    void ImGuiDebugUI::Shutdown()
    {
        if (m_available)
        {
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
        }

        m_available = false;
    }

    void ImGuiDebugUI::BeginFrame()
    {
        if (!m_available)
        {
            return;
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiDebugUI::EndFrame()
    {
        if (!m_available)
        {
            return;
        }

        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    bool ImGuiDebugUI::IsAvailable() const
    {
        return m_available;
    }

    bool ImGuiDebugUI::HandleWin32Message(void* hwnd, unsigned int message, unsigned long long wParam, long long lParam)
    {
        return ImGui_ImplWin32_WndProcHandler(
            static_cast<HWND>(hwnd),
            message,
            static_cast<WPARAM>(wParam),
            static_cast<LPARAM>(lParam)) != 0;
    }
}
