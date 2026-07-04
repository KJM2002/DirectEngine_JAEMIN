#pragma once

#include "Engine/Resource/ImageData.h"

#include <string>

namespace Engine::Platform::Windows
{
    class WICImageLoader
    {
    public:
        static bool LoadFromFile(const std::wstring& path, Resource::ImageData& outImage);
    };
}
