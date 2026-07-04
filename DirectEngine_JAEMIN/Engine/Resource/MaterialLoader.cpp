#include "MaterialLoader.h"

#include "Engine/Core/Log.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace Engine::Resource
{
    namespace
    {
        std::string ToNarrowAscii(const std::wstring& value)
        {
            std::string result;
            result.reserve(value.size());
            for (wchar_t character : value)
            {
                result.push_back(character <= 0x7f ? static_cast<char>(character) : '?');
            }
            return result;
        }

        std::wstring ToWideAscii(const std::string& value)
        {
            std::wstring result;
            result.reserve(value.size());
            for (char character : value)
            {
                result.push_back(static_cast<unsigned char>(character));
            }
            return result;
        }

        std::string Trim(const std::string& value)
        {
            const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c); });
            const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) { return std::isspace(c); }).base();
            if (first >= last)
            {
                return {};
            }
            return std::string(first, last);
        }

        bool ParseVector4(const std::string& text, Math::Vector4& outValue)
        {
            std::stringstream stream(text);
            std::string part;
            float values[4] = {};
            for (int i = 0; i < 4; ++i)
            {
                if (!std::getline(stream, part, ','))
                {
                    return false;
                }
                values[i] = std::stof(Trim(part));
            }

            outValue = { values[0], values[1], values[2], values[3] };
            return true;
        }
    }

    bool MaterialLoader::LoadFromFile(const std::wstring& path, MaterialDesc& outDesc)
    {
        outDesc = {};
        std::ifstream file(ToNarrowAscii(path));
        if (!file.is_open())
        {
            Core::Log::Error("Failed to open material file: " + ToNarrowAscii(path));
            return false;
        }

        std::string line;
        while (std::getline(file, line))
        {
            line = Trim(line);
            if (line.empty() || line[0] == '#')
            {
                continue;
            }

            const std::size_t separator = line.find('=');
            if (separator == std::string::npos)
            {
                continue;
            }

            const std::string key = Trim(line.substr(0, separator));
            const std::string value = Trim(line.substr(separator + 1));

            if (key == "material_name")
            {
                outDesc.name = value;
            }
            else if (key == "vertex_shader")
            {
                outDesc.vertexShaderPath = ToWideAscii(value);
            }
            else if (key == "pixel_shader")
            {
                outDesc.pixelShaderPath = ToWideAscii(value);
            }
            else if (key == "base_color")
            {
                try
                {
                    if (!ParseVector4(value, outDesc.baseColor))
                    {
                        Core::Log::Warning("Invalid base_color in material file: " + ToNarrowAscii(path));
                    }
                }
                catch (const std::exception&)
                {
                    Core::Log::Warning("Invalid base_color in material file: " + ToNarrowAscii(path));
                }
            }
            else if (key == "texture" || key == "base_texture")
            {
                outDesc.texturePath = ToWideAscii(value);
            }
            else if (key == "roughness_texture")
            {
                outDesc.roughnessTexturePath = ToWideAscii(value);
            }
            else if (key == "metallic_texture")
            {
                outDesc.metallicTexturePath = ToWideAscii(value);
            }
            else if (key == "normal_texture")
            {
                outDesc.normalTexturePath = ToWideAscii(value);
            }
            else if (key == "roughness")
            {
                outDesc.roughness = std::clamp(std::stof(value), 0.0f, 1.0f);
            }
            else if (key == "metallic")
            {
                outDesc.metallic = std::clamp(std::stof(value), 0.0f, 1.0f);
            }
        }

        if (outDesc.vertexShaderPath.empty())
        {
            outDesc.vertexShaderPath = L"Shaders/BasicVertex.hlsl";
        }
        if (outDesc.pixelShaderPath.empty())
        {
            outDesc.pixelShaderPath = L"Shaders/BasicPixel.hlsl";
        }

        return true;
    }

    bool MaterialLoader::SaveToFile(const std::wstring& path, const MaterialDesc& desc)
    {
        std::ofstream file(ToNarrowAscii(path));
        if (!file.is_open())
        {
            Core::Log::Error("Failed to save material file: " + ToNarrowAscii(path));
            return false;
        }

        file << "material_name=" << desc.name << "\n";
        file << "vertex_shader=" << ToNarrowAscii(desc.vertexShaderPath.empty() ? L"Shaders/BasicVertex.hlsl" : desc.vertexShaderPath) << "\n";
        file << "pixel_shader=" << ToNarrowAscii(desc.pixelShaderPath.empty() ? L"Shaders/BasicPixel.hlsl" : desc.pixelShaderPath) << "\n";
        file << "base_color=" << desc.baseColor.x << "," << desc.baseColor.y << "," << desc.baseColor.z << "," << desc.baseColor.w << "\n";
        file << "roughness=" << desc.roughness << "\n";
        file << "metallic=" << desc.metallic << "\n";
        file << "base_texture=" << ToNarrowAscii(desc.texturePath) << "\n";
        file << "roughness_texture=" << ToNarrowAscii(desc.roughnessTexturePath) << "\n";
        file << "metallic_texture=" << ToNarrowAscii(desc.metallicTexturePath) << "\n";
        file << "normal_texture=" << ToNarrowAscii(desc.normalTexturePath) << "\n";
        return true;
    }
}
