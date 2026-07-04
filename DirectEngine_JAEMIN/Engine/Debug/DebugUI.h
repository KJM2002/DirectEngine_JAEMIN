#pragma once

#include "DebugStats.h"

#include <string>

namespace Engine::Debug
{
    class DebugUI
    {
    public:
        bool Initialize();
        void Shutdown();
        bool Update(const DebugStats& stats);

        const std::wstring& GetWindowTitle() const;

    private:
        std::wstring BuildWindowTitle(const DebugStats& stats) const;

        std::wstring m_windowTitle = L"DirectEngine JAEMIN";
        float m_titleUpdateTimer = 0.0f;
    };
}
