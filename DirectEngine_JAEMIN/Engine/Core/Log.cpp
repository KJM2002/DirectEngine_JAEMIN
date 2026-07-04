#include "Log.h"

#include <iostream>

namespace Engine::Core
{
    void Log::Info(std::string_view message)
    {
        std::cout << "[Info] " << message << '\n';
    }

    void Log::Warning(std::string_view message)
    {
        std::cout << "[Warning] " << message << '\n';
    }

    void Log::Error(std::string_view message)
    {
        std::cerr << "[Error] " << message << '\n';
    }
}
