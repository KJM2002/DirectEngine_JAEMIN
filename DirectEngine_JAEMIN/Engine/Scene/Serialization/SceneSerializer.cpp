#include "SceneSerializer.h"

#include "Engine/Core/Log.h"
#include "Engine/Physics/ColliderComponent.h"
#include "Engine/Graphics/Renderer/Material.h"
#include "Engine/Resource/ResourceManager.h"
#include "Engine/Scene/Camera.h"
#include "Engine/Scene/GameObject.h"
#include "Engine/Scene/Light.h"
#include "Engine/Scene/LightComponent.h"
#include "Engine/Scene/MeshRendererComponent.h"
#include "Engine/Scene/PlayerStartComponent.h"
#include "Engine/Scene/PostProcessComponent.h"
#include "Engine/Scene/Scene.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>

namespace Engine::Scene
{
    namespace
    {
        enum class Section
        {
            None,
            GameObject,
            Camera,
            DirectionalLight
        };

        struct ObjectDesc
        {
            struct ColliderDesc
            {
                Physics::ColliderType type = Physics::ColliderType::AABB;
                Math::Vector3 center = { 0.0f, 0.0f, 0.0f };
                Math::Vector3 rotation = { 0.0f, 0.0f, 0.0f };
                Math::Vector3 size = { 1.0f, 1.0f, 1.0f };
                float radius = 0.5f;
                bool isTrigger = false;
            };

            std::string name = "GameObject";
            std::string folder;
            Math::Vector3 position = { 0.0f, 0.0f, 0.0f };
            Math::Vector3 rotation = { 0.0f, 0.0f, 0.0f };
            Math::Vector3 scale = { 1.0f, 1.0f, 1.0f };
            std::wstring meshPath;
            std::wstring materialPath;
            std::string meshGuid;
            std::string materialGuid;
            bool hasInlineMaterial = false;
            std::string materialName;
            std::wstring materialVertexShaderPath = L"Shaders/BasicVertex.hlsl";
            std::wstring materialPixelShaderPath = L"Shaders/BasicPixel.hlsl";
            std::wstring materialTexturePath;
            std::wstring materialRoughnessTexturePath;
            std::wstring materialMetallicTexturePath;
            std::wstring materialNormalTexturePath;
            Math::Vector4 materialBaseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
            float materialRoughness = 0.5f;
            float materialMetallic = 0.0f;
            bool hasLight = false;
            LightType lightType = LightType::Point;
            Math::Vector3 lightColor = { 1.0f, 1.0f, 1.0f };
            Math::Vector3 lightDirection = { 0.0f, -1.0f, 0.0f };
            Math::Vector3 ambientColor = { 0.0f, 0.0f, 0.0f };
            float lightIntensity = 1.0f;
            float lightRange = 5.0f;
            float innerConeAngle = 20.0f;
            float outerConeAngle = 35.0f;
            bool hasCollider = false;
            Physics::ColliderType colliderType = Physics::ColliderType::AABB;
            Math::Vector3 colliderCenter = { 0.0f, 0.0f, 0.0f };
            Math::Vector3 colliderRotation = { 0.0f, 0.0f, 0.0f };
            Math::Vector3 colliderSize = { 1.0f, 1.0f, 1.0f };
            float colliderRadius = 0.5f;
            bool colliderIsTrigger = false;
            std::vector<ColliderDesc> colliders;
            bool hasPostProcess = false;
            bool postProcessEnabled = true;
            Renderer::PostProcessEffect postProcessEffect = Renderer::PostProcessEffect::None;
            bool postProcessGrayscaleEnabled = false;
            bool postProcessVignetteEnabled = false;
            bool postProcessToneMappingEnabled = false;
            bool postProcessColorAdjustEnabled = false;
            float postProcessGrayscaleIntensity = 1.0f;
            float postProcessExposure = 1.0f;
            float postProcessGamma = 2.2f;
            float postProcessVignetteStrength = 0.35f;
            float postProcessVignetteRadius = 0.85f;
            float postProcessVignetteSoftness = 0.45f;
            float postProcessBrightness = 0.0f;
            float postProcessContrast = 1.0f;
            float postProcessSaturation = 1.0f;
            bool hasPlayerStart = false;
            float playerHeight = 1.7f;
            float playerMoveSpeed = 4.0f;
            float playerFastMoveMultiplier = 3.0f;
            float playerMouseSensitivity = 0.003f;
            bool hasData = false;
        };

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

        std::string ToNarrowAscii(const std::wstring& value, const wchar_t* fallback)
        {
            return value.empty() ? ToNarrowAscii(fallback) : ToNarrowAscii(value);
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

        bool ParseVector3(const std::string& text, Math::Vector3& outValue)
        {
            std::stringstream stream(text);
            std::string part;
            float values[3] = {};
            for (int i = 0; i < 3; ++i)
            {
                if (!std::getline(stream, part, ','))
                {
                    return false;
                }
                values[i] = std::stof(Trim(part));
            }
            outValue = { values[0], values[1], values[2] };
            return true;
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

        bool ParseBool(const std::string& value)
        {
            return value == "true" || value == "1" || value == "yes";
        }

        LightType ParseLightType(const std::string& value)
        {
            if (value == "Directional")
            {
                return LightType::Directional;
            }
            if (value == "Spot")
            {
                return LightType::Spot;
            }
            return LightType::Point;
        }

        const char* ToLightTypeString(LightType type)
        {
            switch (type)
            {
            case LightType::Directional:
                return "Directional";
            case LightType::Spot:
                return "Spot";
            case LightType::Point:
            default:
                return "Point";
            }
        }

        Physics::ColliderType ParseColliderType(const std::string& value)
        {
            return value == "Sphere" ? Physics::ColliderType::Sphere : Physics::ColliderType::AABB;
        }

        const char* ToColliderTypeString(Physics::ColliderType type)
        {
            return type == Physics::ColliderType::Sphere ? "Sphere" : "AABB";
        }

        Renderer::PostProcessEffect ParsePostProcessEffect(const std::string& value)
        {
            if (value == "Grayscale")
            {
                return Renderer::PostProcessEffect::Grayscale;
            }
            if (value == "Vignette")
            {
                return Renderer::PostProcessEffect::Vignette;
            }
            if (value == "ToneMapping")
            {
                return Renderer::PostProcessEffect::ToneMapping;
            }
            return Renderer::PostProcessEffect::None;
        }

        const char* ToPostProcessEffectString(Renderer::PostProcessEffect effect)
        {
            switch (effect)
            {
            case Renderer::PostProcessEffect::Grayscale:
                return "Grayscale";
            case Renderer::PostProcessEffect::Vignette:
                return "Vignette";
            case Renderer::PostProcessEffect::ToneMapping:
                return "ToneMapping";
            case Renderer::PostProcessEffect::None:
            default:
                return "None";
            }
        }

        float ToRadians(float degrees)
        {
            return degrees * DirectX::XM_PI / 180.0f;
        }

        float ToDegrees(float radians)
        {
            return radians * 180.0f / DirectX::XM_PI;
        }

        void FlushObject(ObjectDesc& desc, Scene& scene, Resource::ResourceManager& resourceManager)
        {
            if (!desc.hasData)
            {
                return;
            }

            GameObject& object = scene.CreateGameObject(desc.name);
            object.SetOutlinerFolder(desc.folder);
            if (!desc.folder.empty())
            {
                scene.AddOutlinerFolder(desc.folder);
            }
            object.GetTransform().position = desc.position;
            object.GetTransform().rotation = desc.rotation;
            object.GetTransform().scale = desc.scale;

            if (!desc.meshPath.empty() || !desc.materialPath.empty() || !desc.meshGuid.empty() || !desc.materialGuid.empty() || desc.hasInlineMaterial)
            {
                MeshRendererComponent& meshRenderer = object.AddComponent<MeshRendererComponent>();
                meshRenderer.SetMeshGuid(desc.meshGuid);
                meshRenderer.SetMaterialGuid(desc.materialGuid);
                if (!desc.meshPath.empty())
                {
                    meshRenderer.SetMesh(resourceManager.LoadMesh(desc.meshPath));
                    meshRenderer.SetMeshPath(desc.meshPath);
                }
                if (!desc.materialPath.empty())
                {
                    meshRenderer.SetMaterial(resourceManager.LoadMaterial(desc.materialPath));
                    meshRenderer.SetMaterialPath(desc.materialPath);
                }
                else if (desc.hasInlineMaterial)
                {
                    const std::string materialKey = "scene:inline_material:" + desc.name + ":" + desc.materialName;
                    std::shared_ptr<Renderer::Material> material = resourceManager.CreateMaterial(materialKey, desc.materialBaseColor);
                    if (material)
                    {
                        material->SetName(desc.materialName.empty() ? materialKey : desc.materialName);
                        material->SetBaseColor(desc.materialBaseColor);
                        material->SetRoughness(desc.materialRoughness);
                        material->SetMetallic(desc.materialMetallic);
                        material->SetVertexShaderPath(desc.materialVertexShaderPath);
                        material->SetPixelShaderPath(desc.materialPixelShaderPath);
                        if (!desc.materialVertexShaderPath.empty())
                        {
                            material->SetVertexShader(resourceManager.LoadVertexShader("scene:vs:" + ToNarrowAscii(desc.materialVertexShaderPath), desc.materialVertexShaderPath));
                        }
                        if (!desc.materialPixelShaderPath.empty())
                        {
                            material->SetPixelShader(resourceManager.LoadPixelShader("scene:ps:" + ToNarrowAscii(desc.materialPixelShaderPath), desc.materialPixelShaderPath));
                        }
                        if (!desc.materialTexturePath.empty())
                        {
                            material->SetBaseColorTexture(resourceManager.LoadTexture(desc.materialTexturePath));
                            material->SetBaseColorTexturePath(desc.materialTexturePath);
                        }
                        if (!desc.materialRoughnessTexturePath.empty())
                        {
                            material->SetRoughnessTexture(resourceManager.LoadTexture(desc.materialRoughnessTexturePath));
                            material->SetRoughnessTexturePath(desc.materialRoughnessTexturePath);
                        }
                        if (!desc.materialMetallicTexturePath.empty())
                        {
                            material->SetMetallicTexture(resourceManager.LoadTexture(desc.materialMetallicTexturePath));
                            material->SetMetallicTexturePath(desc.materialMetallicTexturePath);
                        }
                        if (!desc.materialNormalTexturePath.empty())
                        {
                            material->SetNormalTexture(resourceManager.LoadTexture(desc.materialNormalTexturePath));
                            material->SetNormalTexturePath(desc.materialNormalTexturePath);
                        }
                        meshRenderer.SetMaterial(material);
                        meshRenderer.SetMaterialPath({});
                    }
                }
                if (!meshRenderer.GetMesh())
                {
                    meshRenderer.SetMesh(resourceManager.CreateCubeMesh("fallback:mesh:cube"));
                    meshRenderer.SetMeshPath(L"builtin:mesh:cube");
                }
                if (!meshRenderer.GetMaterial())
                {
                    meshRenderer.SetMaterial(resourceManager.CreateMaterial("fallback:material:default", { 0.8f, 0.8f, 0.8f, 1.0f }));
                    meshRenderer.SetMaterialPath(L"fallback:material:default");
                }
            }

            if (desc.hasLight)
            {
                LightComponent& light = object.AddComponent<LightComponent>();
                light.type = desc.lightType;
                light.color = desc.lightColor;
                light.direction = desc.lightDirection;
                light.ambientColor = desc.ambientColor;
                light.intensity = desc.lightIntensity;
                light.range = desc.lightRange;
                light.innerConeAngle = desc.innerConeAngle;
                light.outerConeAngle = desc.outerConeAngle;
                light.ClampConeAngles();
            }

            if (desc.hasCollider)
            {
                if (desc.colliders.empty())
                {
                    desc.colliders.push_back({ desc.colliderType, desc.colliderCenter, desc.colliderRotation, desc.colliderSize, desc.colliderRadius, desc.colliderIsTrigger });
                }

                for (const ObjectDesc::ColliderDesc& source : desc.colliders)
                {
                    Physics::ColliderComponent& collider = object.AddComponent<Physics::ColliderComponent>();
                    collider.type = source.type;
                    collider.center = source.center;
                    collider.rotation = source.rotation;
                    collider.size = source.size;
                    collider.radius = source.radius;
                    collider.isTrigger = source.isTrigger;
                }
            }

            if (desc.hasPostProcess)
            {
                PostProcessComponent& postProcess = object.AddComponent<PostProcessComponent>();
                postProcess.enabled = desc.postProcessEnabled;
                postProcess.effect = desc.postProcessEffect;
                postProcess.grayscaleEnabled = desc.postProcessGrayscaleEnabled || desc.postProcessEffect == Renderer::PostProcessEffect::Grayscale;
                postProcess.vignetteEnabled = desc.postProcessVignetteEnabled || desc.postProcessEffect == Renderer::PostProcessEffect::Vignette;
                postProcess.toneMappingEnabled = desc.postProcessToneMappingEnabled || desc.postProcessEffect == Renderer::PostProcessEffect::ToneMapping;
                postProcess.colorAdjustEnabled = desc.postProcessColorAdjustEnabled;
                postProcess.effect = Renderer::PostProcessEffect::None;
                postProcess.grayscaleIntensity = desc.postProcessGrayscaleIntensity;
                postProcess.exposure = desc.postProcessExposure;
                postProcess.gamma = desc.postProcessGamma;
                postProcess.vignetteStrength = desc.postProcessVignetteStrength;
                postProcess.vignetteRadius = desc.postProcessVignetteRadius;
                postProcess.vignetteSoftness = desc.postProcessVignetteSoftness;
                postProcess.brightness = desc.postProcessBrightness;
                postProcess.contrast = desc.postProcessContrast;
                postProcess.saturation = desc.postProcessSaturation;
            }

            if (desc.hasPlayerStart)
            {
                PlayerStartComponent& playerStart = object.AddComponent<PlayerStartComponent>();
                playerStart.playerHeight = desc.playerHeight;
                playerStart.moveSpeed = desc.playerMoveSpeed;
                playerStart.fastMoveMultiplier = desc.playerFastMoveMultiplier;
                playerStart.mouseSensitivity = desc.playerMouseSensitivity;
            }

            desc = {};
        }

        void WriteVector3(std::ofstream& file, const char* key, const Math::Vector3& value)
        {
            file << key << "=" << value.x << "," << value.y << "," << value.z << "\n";
        }

        void WriteVector4(std::ofstream& file, const char* key, const Math::Vector4& value)
        {
            file << key << "=" << value.x << "," << value.y << "," << value.z << "," << value.w << "\n";
        }
    }

    bool SceneSerializer::LoadScene(const std::wstring& path, Scene& scene, Resource::ResourceManager& resourceManager)
    {
        std::ifstream file(ToNarrowAscii(path));
        if (!file.is_open())
        {
            Core::Log::Error("Failed to open scene file: " + ToNarrowAscii(path));
            return false;
        }

        scene.Clear();

        Section section = Section::None;
        ObjectDesc objectDesc;
        std::shared_ptr<Camera> camera;
        std::shared_ptr<DirectionalLight> light;

        std::string line;
        while (std::getline(file, line))
        {
            line = Trim(line);
            if (line.empty() || line[0] == '#')
            {
                continue;
            }

            if (line == "[GameObject]")
            {
                FlushObject(objectDesc, scene, resourceManager);
                section = Section::GameObject;
                objectDesc.hasData = true;
                continue;
            }
            if (line == "[Camera]")
            {
                FlushObject(objectDesc, scene, resourceManager);
                section = Section::Camera;
                camera = std::make_shared<Camera>();
                continue;
            }
            if (line == "[DirectionalLight]")
            {
                FlushObject(objectDesc, scene, resourceManager);
                section = Section::DirectionalLight;
                light = std::make_shared<DirectionalLight>();
                continue;
            }

            const std::size_t separator = line.find('=');
            if (separator == std::string::npos)
            {
                continue;
            }

            const std::string key = Trim(line.substr(0, separator));
            const std::string value = Trim(line.substr(separator + 1));

            try
            {
                if (key == "outliner_folder")
                {
                    scene.AddOutlinerFolder(value);
                    continue;
                }

                if (section == Section::GameObject)
                {
                    if (key == "name")
                    {
                        objectDesc.name = value;
                    }
                    else if (key == "folder")
                    {
                        objectDesc.folder = value;
                    }
                    else if (key == "position")
                    {
                        ParseVector3(value, objectDesc.position);
                    }
                    else if (key == "rotation")
                    {
                        ParseVector3(value, objectDesc.rotation);
                    }
                    else if (key == "scale")
                    {
                        ParseVector3(value, objectDesc.scale);
                    }
                    else if (key == "mesh")
                    {
                        objectDesc.meshPath = ToWideAscii(value);
                    }
                    else if (key == "meshGuid")
                    {
                        objectDesc.meshGuid = value;
                    }
                    else if (key == "material")
                    {
                        objectDesc.materialPath = ToWideAscii(value);
                    }
                    else if (key == "materialGuid")
                    {
                        objectDesc.materialGuid = value;
                    }
                    else if (key == "material_inline")
                    {
                        objectDesc.hasInlineMaterial = ParseBool(value);
                    }
                    else if (key == "material_name")
                    {
                        objectDesc.hasInlineMaterial = true;
                        objectDesc.materialName = value;
                    }
                    else if (key == "material_vertex_shader")
                    {
                        objectDesc.hasInlineMaterial = true;
                        objectDesc.materialVertexShaderPath = ToWideAscii(value);
                    }
                    else if (key == "material_pixel_shader")
                    {
                        objectDesc.hasInlineMaterial = true;
                        objectDesc.materialPixelShaderPath = ToWideAscii(value);
                    }
                    else if (key == "material_base_color")
                    {
                        objectDesc.hasInlineMaterial = true;
                        ParseVector4(value, objectDesc.materialBaseColor);
                    }
                    else if (key == "material_roughness")
                    {
                        objectDesc.hasInlineMaterial = true;
                        objectDesc.materialRoughness = std::stof(value);
                    }
                    else if (key == "material_metallic")
                    {
                        objectDesc.hasInlineMaterial = true;
                        objectDesc.materialMetallic = std::stof(value);
                    }
                    else if (key == "material_texture")
                    {
                        objectDesc.hasInlineMaterial = true;
                        objectDesc.materialTexturePath = ToWideAscii(value);
                    }
                    else if (key == "material_roughness_texture")
                    {
                        objectDesc.hasInlineMaterial = true;
                        objectDesc.materialRoughnessTexturePath = ToWideAscii(value);
                    }
                    else if (key == "material_metallic_texture")
                    {
                        objectDesc.hasInlineMaterial = true;
                        objectDesc.materialMetallicTexturePath = ToWideAscii(value);
                    }
                    else if (key == "material_normal_texture")
                    {
                        objectDesc.hasInlineMaterial = true;
                        objectDesc.materialNormalTexturePath = ToWideAscii(value);
                    }
                    else if (key == "component" && value == "Light")
                    {
                        objectDesc.hasLight = true;
                    }
                    else if (key == "component" && value == "Collider")
                    {
                        objectDesc.hasCollider = true;
                        objectDesc.colliders.push_back({});
                    }
                    else if (key == "component" && value == "PostProcess")
                    {
                        objectDesc.hasPostProcess = true;
                    }
                    else if (key == "component" && value == "PlayerStart")
                    {
                        objectDesc.hasPlayerStart = true;
                    }
                    else if (key == "light_type")
                    {
                        objectDesc.hasLight = true;
                        objectDesc.lightType = ParseLightType(value);
                    }
                    else if (key == "color")
                    {
                        objectDesc.hasLight = true;
                        ParseVector3(value, objectDesc.lightColor);
                    }
                    else if (key == "direction")
                    {
                        objectDesc.hasLight = true;
                        ParseVector3(value, objectDesc.lightDirection);
                    }
                    else if (key == "ambient")
                    {
                        objectDesc.hasLight = true;
                        ParseVector3(value, objectDesc.ambientColor);
                    }
                    else if (key == "intensity")
                    {
                        objectDesc.hasLight = true;
                        objectDesc.lightIntensity = std::stof(value);
                    }
                    else if (key == "range")
                    {
                        objectDesc.hasLight = true;
                        objectDesc.lightRange = std::stof(value);
                    }
                    else if (key == "inner_cone")
                    {
                        objectDesc.hasLight = true;
                        objectDesc.innerConeAngle = std::stof(value);
                    }
                    else if (key == "outer_cone")
                    {
                        objectDesc.hasLight = true;
                        objectDesc.outerConeAngle = std::stof(value);
                    }
                    else if (key == "collider_type")
                    {
                        objectDesc.hasCollider = true;
                        objectDesc.colliderType = ParseColliderType(value);
                        if (objectDesc.colliders.empty())
                        {
                            objectDesc.colliders.push_back({});
                        }
                        objectDesc.colliders.back().type = objectDesc.colliderType;
                    }
                    else if (key == "center")
                    {
                        objectDesc.hasCollider = true;
                        ParseVector3(value, objectDesc.colliderCenter);
                        if (objectDesc.colliders.empty())
                        {
                            objectDesc.colliders.push_back({});
                        }
                        objectDesc.colliders.back().center = objectDesc.colliderCenter;
                    }
                    else if (key == "collider_rotation")
                    {
                        objectDesc.hasCollider = true;
                        ParseVector3(value, objectDesc.colliderRotation);
                        if (objectDesc.colliders.empty())
                        {
                            objectDesc.colliders.push_back({});
                        }
                        objectDesc.colliders.back().rotation = objectDesc.colliderRotation;
                    }
                    else if (key == "size")
                    {
                        objectDesc.hasCollider = true;
                        ParseVector3(value, objectDesc.colliderSize);
                        if (objectDesc.colliders.empty())
                        {
                            objectDesc.colliders.push_back({});
                        }
                        objectDesc.colliders.back().size = objectDesc.colliderSize;
                    }
                    else if (key == "radius")
                    {
                        objectDesc.hasCollider = true;
                        objectDesc.colliderRadius = std::stof(value);
                        if (objectDesc.colliders.empty())
                        {
                            objectDesc.colliders.push_back({});
                        }
                        objectDesc.colliders.back().radius = objectDesc.colliderRadius;
                    }
                    else if (key == "is_trigger")
                    {
                        objectDesc.hasCollider = true;
                        objectDesc.colliderIsTrigger = ParseBool(value);
                        if (objectDesc.colliders.empty())
                        {
                            objectDesc.colliders.push_back({});
                        }
                        objectDesc.colliders.back().isTrigger = objectDesc.colliderIsTrigger;
                    }
                    else if (key == "postprocess_enabled")
                    {
                        objectDesc.hasPostProcess = true;
                        objectDesc.postProcessEnabled = ParseBool(value);
                    }
                    else if (key == "postprocess_effect")
                    {
                        objectDesc.hasPostProcess = true;
                        objectDesc.postProcessEffect = ParsePostProcessEffect(value);
                    }
                    else if (key == "postprocess_grayscale")
                    {
                        objectDesc.hasPostProcess = true;
                        objectDesc.postProcessGrayscaleEnabled = ParseBool(value);
                    }
                    else if (key == "postprocess_grayscale_intensity")
                    {
                        objectDesc.hasPostProcess = true;
                        objectDesc.postProcessGrayscaleIntensity = std::stof(value);
                    }
                    else if (key == "postprocess_vignette_enabled")
                    {
                        objectDesc.hasPostProcess = true;
                        objectDesc.postProcessVignetteEnabled = ParseBool(value);
                    }
                    else if (key == "postprocess_tonemapping")
                    {
                        objectDesc.hasPostProcess = true;
                        objectDesc.postProcessToneMappingEnabled = ParseBool(value);
                    }
                    else if (key == "postprocess_color_adjust")
                    {
                        objectDesc.hasPostProcess = true;
                        objectDesc.postProcessColorAdjustEnabled = ParseBool(value);
                    }
                    else if (key == "postprocess_exposure")
                    {
                        objectDesc.hasPostProcess = true;
                        objectDesc.postProcessExposure = std::stof(value);
                    }
                    else if (key == "postprocess_gamma")
                    {
                        objectDesc.hasPostProcess = true;
                        objectDesc.postProcessGamma = std::stof(value);
                    }
                    else if (key == "postprocess_vignette")
                    {
                        objectDesc.hasPostProcess = true;
                        objectDesc.postProcessVignetteStrength = std::stof(value);
                    }
                    else if (key == "postprocess_vignette_radius")
                    {
                        objectDesc.hasPostProcess = true;
                        objectDesc.postProcessVignetteRadius = std::stof(value);
                    }
                    else if (key == "postprocess_vignette_softness")
                    {
                        objectDesc.hasPostProcess = true;
                        objectDesc.postProcessVignetteSoftness = std::stof(value);
                    }
                    else if (key == "postprocess_brightness")
                    {
                        objectDesc.hasPostProcess = true;
                        objectDesc.postProcessBrightness = std::stof(value);
                    }
                    else if (key == "postprocess_contrast")
                    {
                        objectDesc.hasPostProcess = true;
                        objectDesc.postProcessContrast = std::stof(value);
                    }
                    else if (key == "postprocess_saturation")
                    {
                        objectDesc.hasPostProcess = true;
                        objectDesc.postProcessSaturation = std::stof(value);
                    }
                    else if (key == "player_height")
                    {
                        objectDesc.hasPlayerStart = true;
                        objectDesc.playerHeight = std::stof(value);
                    }
                    else if (key == "player_move_speed")
                    {
                        objectDesc.hasPlayerStart = true;
                        objectDesc.playerMoveSpeed = std::stof(value);
                    }
                    else if (key == "player_fast_multiplier")
                    {
                        objectDesc.hasPlayerStart = true;
                        objectDesc.playerFastMoveMultiplier = std::stof(value);
                    }
                    else if (key == "player_mouse_sensitivity")
                    {
                        objectDesc.hasPlayerStart = true;
                        objectDesc.playerMouseSensitivity = std::stof(value);
                    }
                }
                else if (section == Section::Camera && camera)
                {
                    if (key == "name")
                    {
                        camera->name = value;
                    }
                    else if (key == "position")
                    {
                        ParseVector3(value, camera->position);
                    }
                    else if (key == "rotation")
                    {
                        Math::Vector3 rotation = {};
                        ParseVector3(value, rotation);
                        camera->pitch = ToRadians(rotation.x);
                        camera->yaw = ToRadians(rotation.y);
                    }
                    else if (key == "fov")
                    {
                        camera->fov = ToRadians(std::stof(value));
                    }
                    else if (key == "near")
                    {
                        camera->nearPlane = std::stof(value);
                    }
                    else if (key == "far")
                    {
                        camera->farPlane = std::stof(value);
                    }
                    else if (key == "active")
                    {
                        camera->active = ParseBool(value);
                    }
                }
                else if (section == Section::DirectionalLight && light)
                {
                    if (key == "name")
                    {
                        light->name = value;
                    }
                    else if (key == "direction")
                    {
                        ParseVector3(value, light->direction);
                    }
                    else if (key == "color")
                    {
                        ParseVector3(value, light->color);
                    }
                    else if (key == "intensity")
                    {
                        light->intensity = std::stof(value);
                    }
                    else if (key == "ambient")
                    {
                        ParseVector3(value, light->ambientColor);
                    }
                }
            }
            catch (const std::exception&)
            {
                Core::Log::Warning("Skipped malformed scene property: " + line);
            }
        }

        FlushObject(objectDesc, scene, resourceManager);
        if (camera && camera->active)
        {
            scene.SetActiveCamera(camera);
        }
        if (light)
        {
            scene.SetDirectionalLight(light);
        }

        scene.EnsureDefaultCameraAndLight();
        scene.SetFilePath(std::filesystem::path(path));
        scene.ClearDirty();
        return true;
    }

    bool SceneSerializer::SaveScene(const std::wstring& path, const Scene& scene)
    {
        const std::filesystem::path targetPath(path);
        const std::filesystem::path tempPath = std::filesystem::path(path).concat(L".tmp");
        const std::filesystem::path backupPath = std::filesystem::path(path).concat(L".bak");

        std::error_code error;
        std::filesystem::create_directories(targetPath.parent_path(), error);
        if (error)
        {
            Core::Log::Error("Failed to create scene folder: " + error.message());
            return false;
        }

        std::ofstream file(ToNarrowAscii(tempPath.generic_wstring()), std::ios::trunc);
        if (!file.is_open())
        {
            Core::Log::Error("Failed to open temp scene file: " + ToNarrowAscii(tempPath.generic_wstring()));
            return false;
        }

        file << "scene_name=SavedScene\n";
        for (const std::string& folder : scene.GetOutlinerFolders())
        {
            file << "outliner_folder=" << folder << "\n";
        }
        file << "\n";

        if (const std::shared_ptr<Camera>& camera = scene.GetActiveCamera())
        {
            file << "[Camera]\n";
            file << "name=" << camera->name << "\n";
            WriteVector3(file, "position", camera->position);
            file << "rotation=" << ToDegrees(camera->pitch) << "," << ToDegrees(camera->yaw) << ",0\n";
            file << "fov=" << ToDegrees(camera->fov) << "\n";
            file << "near=" << camera->nearPlane << "\n";
            file << "far=" << camera->farPlane << "\n";
            file << "active=" << (camera->active ? "true" : "false") << "\n\n";
        }

        if (const std::shared_ptr<DirectionalLight>& light = scene.GetDirectionalLight())
        {
            file << "[DirectionalLight]\n";
            file << "name=" << light->name << "\n";
            WriteVector3(file, "direction", light->direction);
            WriteVector3(file, "color", light->color);
            file << "intensity=" << light->intensity << "\n";
            WriteVector3(file, "ambient", light->ambientColor);
            file << "\n";
        }

        for (const std::unique_ptr<GameObject>& object : scene.GetGameObjects())
        {
            file << "[GameObject]\n";
            file << "name=" << object->GetName() << "\n";
            if (!object->GetOutlinerFolder().empty())
            {
                file << "folder=" << object->GetOutlinerFolder() << "\n";
            }
            WriteVector3(file, "position", object->GetTransform().position);
            WriteVector3(file, "rotation", object->GetTransform().rotation);
            WriteVector3(file, "scale", object->GetTransform().scale);

            const MeshRendererComponent* meshRenderer = object->GetComponent<MeshRendererComponent>();
            if (meshRenderer)
            {
                if (!meshRenderer->GetMeshPath().empty())
                {
                    file << "mesh=" << ToNarrowAscii(meshRenderer->GetMeshPath(), L"") << "\n";
                }
                if (!meshRenderer->GetMeshGuid().empty())
                {
                    file << "meshGuid=" << meshRenderer->GetMeshGuid() << "\n";
                }
                const bool materialPathIsAsset = !meshRenderer->GetMaterialPath().empty()
                    && std::filesystem::path(meshRenderer->GetMaterialPath()).extension() == L".mat";
                if (materialPathIsAsset)
                {
                    file << "material=" << ToNarrowAscii(meshRenderer->GetMaterialPath(), L"") << "\n";
                }
                if (!meshRenderer->GetMaterialGuid().empty())
                {
                    file << "materialGuid=" << meshRenderer->GetMaterialGuid() << "\n";
                }
                else if (const Renderer::Material* material = meshRenderer->GetMaterial().get())
                {
                    file << "material_inline=true\n";
                    file << "material_name=" << material->GetName() << "\n";
                    file << "material_vertex_shader=" << ToNarrowAscii(material->GetVertexShaderPath(), L"Shaders/BasicVertex.hlsl") << "\n";
                    file << "material_pixel_shader=" << ToNarrowAscii(material->GetPixelShaderPath(), L"Shaders/BasicPixel.hlsl") << "\n";
                    WriteVector4(file, "material_base_color", material->GetBaseColor());
                    file << "material_roughness=" << material->GetRoughness() << "\n";
                    file << "material_metallic=" << material->GetMetallic() << "\n";
                    if (!material->GetBaseColorTexturePath().empty())
                    {
                        file << "material_texture=" << ToNarrowAscii(material->GetBaseColorTexturePath(), L"") << "\n";
                    }
                    if (!material->GetRoughnessTexturePath().empty())
                    {
                        file << "material_roughness_texture=" << ToNarrowAscii(material->GetRoughnessTexturePath(), L"") << "\n";
                    }
                    if (!material->GetMetallicTexturePath().empty())
                    {
                        file << "material_metallic_texture=" << ToNarrowAscii(material->GetMetallicTexturePath(), L"") << "\n";
                    }
                    if (!material->GetNormalTexturePath().empty())
                    {
                        file << "material_normal_texture=" << ToNarrowAscii(material->GetNormalTexturePath(), L"") << "\n";
                    }
                }
            }

            const LightComponent* light = object->GetComponent<LightComponent>();
            if (light)
            {
                file << "component=Light\n";
                file << "light_type=" << ToLightTypeString(light->type) << "\n";
                WriteVector3(file, "color", light->color);
                file << "intensity=" << light->intensity << "\n";
                file << "range=" << light->range << "\n";
                WriteVector3(file, "direction", light->direction);
                WriteVector3(file, "ambient", light->ambientColor);
                file << "inner_cone=" << light->innerConeAngle << "\n";
                file << "outer_cone=" << light->outerConeAngle << "\n";
            }

            for (const Physics::ColliderComponent* collider : object->GetComponents<Physics::ColliderComponent>())
            {
                if (!collider)
                {
                    continue;
                }

                file << "component=Collider\n";
                file << "collider_type=" << ToColliderTypeString(collider->type) << "\n";
                WriteVector3(file, "center", collider->center);
                WriteVector3(file, "collider_rotation", collider->rotation);
                WriteVector3(file, "size", collider->size);
                file << "radius=" << collider->radius << "\n";
                file << "is_trigger=" << (collider->isTrigger ? "true" : "false") << "\n";
            }

            const PostProcessComponent* postProcess = object->GetComponent<PostProcessComponent>();
            if (postProcess)
            {
                file << "component=PostProcess\n";
                file << "postprocess_enabled=" << (postProcess->enabled ? "true" : "false") << "\n";
                file << "postprocess_effect=" << ToPostProcessEffectString(Renderer::PostProcessEffect::None) << "\n";
                file << "postprocess_grayscale=" << (postProcess->grayscaleEnabled ? "true" : "false") << "\n";
                file << "postprocess_grayscale_intensity=" << postProcess->grayscaleIntensity << "\n";
                file << "postprocess_vignette_enabled=" << (postProcess->vignetteEnabled ? "true" : "false") << "\n";
                file << "postprocess_tonemapping=" << (postProcess->toneMappingEnabled ? "true" : "false") << "\n";
                file << "postprocess_color_adjust=" << (postProcess->colorAdjustEnabled ? "true" : "false") << "\n";
                file << "postprocess_exposure=" << postProcess->exposure << "\n";
                file << "postprocess_gamma=" << postProcess->gamma << "\n";
                file << "postprocess_vignette=" << postProcess->vignetteStrength << "\n";
                file << "postprocess_vignette_radius=" << postProcess->vignetteRadius << "\n";
                file << "postprocess_vignette_softness=" << postProcess->vignetteSoftness << "\n";
                file << "postprocess_brightness=" << postProcess->brightness << "\n";
                file << "postprocess_contrast=" << postProcess->contrast << "\n";
                file << "postprocess_saturation=" << postProcess->saturation << "\n";
            }

            const PlayerStartComponent* playerStart = object->GetComponent<PlayerStartComponent>();
            if (playerStart)
            {
                file << "component=PlayerStart\n";
                file << "player_height=" << playerStart->playerHeight << "\n";
                file << "player_move_speed=" << playerStart->moveSpeed << "\n";
                file << "player_fast_multiplier=" << playerStart->fastMoveMultiplier << "\n";
                file << "player_mouse_sensitivity=" << playerStart->mouseSensitivity << "\n";
            }
            file << "\n";
        }

        file.flush();
        if (!file.good())
        {
            file.close();
            std::filesystem::remove(tempPath, error);
            Core::Log::Error("Failed while writing temp scene file: " + ToNarrowAscii(tempPath.generic_wstring()));
            return false;
        }
        file.close();

        error.clear();
        if (std::filesystem::exists(targetPath))
        {
            std::filesystem::remove(backupPath, error);
            error.clear();
            std::filesystem::rename(targetPath, backupPath, error);
            if (error)
            {
                std::filesystem::remove(tempPath);
                Core::Log::Error("Failed to move original scene to backup: " + error.message());
                return false;
            }
        }

        error.clear();
        std::filesystem::rename(tempPath, targetPath, error);
        if (error)
        {
            std::filesystem::remove(tempPath);
            std::error_code restoreError;
            if (std::filesystem::exists(backupPath))
            {
                std::filesystem::rename(backupPath, targetPath, restoreError);
            }
            Core::Log::Error("Failed to replace scene file safely: " + error.message());
            return false;
        }

        return true;
    }
}
