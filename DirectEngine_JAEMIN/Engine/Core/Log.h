#pragma once

#include <string_view>

namespace Engine::Core
{
    // Tiny logging facade used by early engine code before a full diagnostics system exists.
    class Log
    {
    public:
        static void Info(std::string_view message);
        static void Warning(std::string_view message);
        static void Error(std::string_view message);
    };
}
