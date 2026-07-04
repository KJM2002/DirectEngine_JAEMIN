#include "DebugUI.h"

#include <iomanip>
#include <sstream>

namespace Engine::Debug
{
    bool DebugUI::Initialize()
    {
        m_windowTitle = L"DirectEngine JAEMIN";
        m_titleUpdateTimer = 0.0f;
        return true;
    }

    void DebugUI::Shutdown()
    {
        m_windowTitle.clear();
        m_titleUpdateTimer = 0.0f;
    }

    bool DebugUI::Update(const DebugStats& stats)
    {
        m_titleUpdateTimer += stats.deltaTime;
        if (m_titleUpdateTimer < 0.25f)
        {
            return false;
        }

        m_titleUpdateTimer = 0.0f;
        m_windowTitle = BuildWindowTitle(stats);
        return true;
    }

    const std::wstring& DebugUI::GetWindowTitle() const
    {
        return m_windowTitle;
    }

    std::wstring DebugUI::BuildWindowTitle(const DebugStats& stats) const
    {
        std::wstringstream stream;
        stream << std::fixed << std::setprecision(1);
        stream << L"DirectEngine JAEMIN"
            << L" | FPS " << stats.fps
            << L" | DT " << stats.deltaTime * 1000.0f << L"ms"
            << L" | Obj " << stats.objectCount
            << L" | Draw " << stats.drawCallCount
            << L" | Tri " << stats.triangleCount
            << L" | Cam ("
            << stats.cameraPosition.x << L", "
            << stats.cameraPosition.y << L", "
            << stats.cameraPosition.z << L")";
        return stream.str();
    }
}
