#include "EditorLayer.h"

#include "Engine/Core/Log.h"
#include "Engine/Debug/DebugStats.h"
#include "Engine/Editor/Command/CommandManager.h"
#include "Engine/Editor/Command/CreateGameObjectCommand.h"
#include "Engine/Editor/Command/DeleteGameObjectCommand.h"
#include "Engine/Editor/Command/DuplicateGameObjectCommand.h"
#include "Engine/Editor/Command/GameObjectSnapshot.h"
#include "Engine/Editor/Command/InspectorEditCommand.h"
#include "Engine/Editor/Command/RenameGameObjectCommand.h"
#include "Engine/Editor/Command/SetObjectFolderCommand.h"
#include "Engine/Editor/Command/TransformCommand.h"
#include "Engine/Editor/Property/PropertySystem.h"
#include "Engine/Input/Input.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Graphics/Renderer/Material.h"
#include "Engine/Physics/ColliderComponent.h"
#include "Engine/Resource/MaterialLoader.h"
#include "Engine/Resource/ResourceManager.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Physics/Ray.h"
#include "Engine/Physics/RaycastHit.h"
#include "Engine/Scene/Camera.h"
#include "Engine/Scene/GameObject.h"
#include "Engine/Scene/LightComponent.h"
#include "Engine/Scene/MeshRendererComponent.h"
#include "Engine/Scene/PlayerStartComponent.h"
#include "Engine/Scene/PostProcessComponent.h"
#include "Engine/Scene/Serialization/SceneSerializer.h"
#include "Engine/Scene/Scene.h"

#include "imgui.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <commdlg.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cfloat>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <sstream>

namespace Engine::Editor
{
    namespace
    {
        const char* ToLightTypeName(Scene::LightType type)
        {
            switch (type)
            {
            case Scene::LightType::Directional:
                return "Directional";
            case Scene::LightType::Spot:
                return "Spot";
            case Scene::LightType::Point:
            default:
                return "Point";
            }
        }

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
            const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char character)
            {
                return std::isspace(character) != 0;
            });
            const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char character)
            {
                return std::isspace(character) != 0;
            }).base();

            if (first >= last)
            {
                return {};
            }

            return std::string(first, last);
        }

        bool HasExtension(const std::filesystem::path& path, std::initializer_list<const wchar_t*> extensions)
        {
            const std::wstring extension = path.extension().wstring();
            for (const wchar_t* candidate : extensions)
            {
                if (_wcsicmp(extension.c_str(), candidate) == 0)
                {
                    return true;
                }
            }
            return false;
        }

        std::wstring ToAssetPath(const std::filesystem::path& path)
        {
            return path.generic_wstring();
        }

        const char* ToColliderTypeName(Physics::ColliderType type)
        {
            return type == Physics::ColliderType::Sphere ? "Sphere" : "AABB";
        }

        const char* ToPostProcessEffectName(Renderer::PostProcessEffect effect)
        {
            switch (effect)
            {
            case Renderer::PostProcessEffect::Grayscale:
                return "Grayscale";
            case Renderer::PostProcessEffect::Vignette:
                return "Vignette";
            case Renderer::PostProcessEffect::ToneMapping:
                return "Tone Mapping";
            case Renderer::PostProcessEffect::None:
            default:
                return "None";
            }
        }

        const char* ToPostProcessStackSummary(const Scene::PostProcessComponent& postProcess)
        {
            if (!postProcess.enabled)
            {
                return "Disabled";
            }
            if (!postProcess.grayscaleEnabled
                && !postProcess.vignetteEnabled
                && !postProcess.toneMappingEnabled
                && !postProcess.colorAdjustEnabled
                && postProcess.effect == Renderer::PostProcessEffect::None)
            {
                return "No active effect";
            }
            return "Stack active";
        }

        Physics::Ray MakeCenterRayFromCamera(const Scene::Camera& camera)
        {
            const float cosPitch = std::cos(camera.pitch);
            DirectX::XMVECTOR direction = DirectX::XMVector3Normalize(DirectX::XMVectorSet(
                std::sin(camera.yaw) * cosPitch,
                -std::sin(camera.pitch),
                std::cos(camera.yaw) * cosPitch,
                0.0f));

            Physics::Ray ray;
            ray.origin = camera.position;
            DirectX::XMStoreFloat3(&ray.direction, direction);
            return ray;
        }

        Physics::Ray MakeMouseRayFromCamera(const Scene::Camera& camera, int mouseX, int mouseY, float width, float height)
        {
            if (width <= 0.0f || height <= 0.0f)
            {
                return MakeCenterRayFromCamera(camera);
            }

            const float aspectRatio = width / height;
            const DirectX::XMMATRIX view = camera.GetViewMatrix();
            const DirectX::XMMATRIX projection = camera.GetProjectionMatrix(aspectRatio);
            const DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();
            const DirectX::XMVECTOR nearPoint = DirectX::XMVector3Unproject(
                DirectX::XMVectorSet(static_cast<float>(mouseX), static_cast<float>(mouseY), 0.0f, 1.0f),
                0.0f,
                0.0f,
                width,
                height,
                0.0f,
                1.0f,
                projection,
                view,
                world);
            const DirectX::XMVECTOR farPoint = DirectX::XMVector3Unproject(
                DirectX::XMVectorSet(static_cast<float>(mouseX), static_cast<float>(mouseY), 1.0f, 1.0f),
                0.0f,
                0.0f,
                width,
                height,
                0.0f,
                1.0f,
                projection,
                view,
                world);

            Physics::Ray ray;
            DirectX::XMStoreFloat3(&ray.origin, nearPoint);
            DirectX::XMStoreFloat3(&ray.direction, DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(farPoint, nearPoint)));
            return ray;
        }

        Math::Vector3 TransformPoint(const Math::Matrix& matrix, const Math::Vector3& point)
        {
            Math::Vector3 result;
            DirectX::XMStoreFloat3(&result, DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&point), matrix));
            return result;
        }

        bool RaycastWorldAabb(const Physics::Ray& ray, const Math::Vector3& minPoint, const Math::Vector3& maxPoint, float& outDistance)
        {
            float tMin = 0.0f;
            float tMax = FLT_MAX;
            const float origin[3] = { ray.origin.x, ray.origin.y, ray.origin.z };
            const float direction[3] = { ray.direction.x, ray.direction.y, ray.direction.z };
            const float boundsMin[3] = { minPoint.x, minPoint.y, minPoint.z };
            const float boundsMax[3] = { maxPoint.x, maxPoint.y, maxPoint.z };

            for (int axis = 0; axis < 3; ++axis)
            {
                if (std::abs(direction[axis]) < 0.00001f)
                {
                    if (origin[axis] < boundsMin[axis] || origin[axis] > boundsMax[axis])
                    {
                        return false;
                    }
                    continue;
                }

                const float invDirection = 1.0f / direction[axis];
                float t0 = (boundsMin[axis] - origin[axis]) * invDirection;
                float t1 = (boundsMax[axis] - origin[axis]) * invDirection;
                if (t0 > t1)
                {
                    std::swap(t0, t1);
                }
                tMin = std::max(tMin, t0);
                tMax = std::min(tMax, t1);
                if (tMax < tMin)
                {
                    return false;
                }
            }

            outDistance = tMin;
            return true;
        }

        Math::Vector3 GetAxisVector(GizmoAxis axis)
        {
            switch (axis)
            {
            case GizmoAxis::X:
                return { 1.0f, 0.0f, 0.0f };
            case GizmoAxis::Y:
                return { 0.0f, 1.0f, 0.0f };
            case GizmoAxis::Z:
                return { 0.0f, 0.0f, 1.0f };
            case GizmoAxis::None:
            default:
                return { 0.0f, 0.0f, 0.0f };
            }
        }

        Renderer::GizmoAxis ToRendererGizmoAxis(GizmoAxis axis)
        {
            switch (axis)
            {
            case GizmoAxis::X:
                return Renderer::GizmoAxis::X;
            case GizmoAxis::Y:
                return Renderer::GizmoAxis::Y;
            case GizmoAxis::Z:
                return Renderer::GizmoAxis::Z;
            case GizmoAxis::None:
            default:
                return Renderer::GizmoAxis::None;
            }
        }

        Math::Vector3 RotateAxis(const Math::Vector3& rotationValue, const Math::Vector3& axis)
        {
            const DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(rotationValue.x, rotationValue.y, rotationValue.z);
            Math::Vector3 result;
            DirectX::XMStoreFloat3(&result, DirectX::XMVector3Normalize(DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&axis), rotation)));
            return result;
        }

        Math::Vector3 GetLocalAxisVector(const Math::Transform& transform, GizmoAxis axis)
        {
            return RotateAxis(transform.rotation, GetAxisVector(axis));
        }

        float GetAxisComponent(const Math::Vector3& value, GizmoAxis axis)
        {
            switch (axis)
            {
            case GizmoAxis::X:
                return value.x;
            case GizmoAxis::Y:
                return value.y;
            case GizmoAxis::Z:
                return value.z;
            case GizmoAxis::None:
            default:
                return 0.0f;
            }
        }

        float GetAxisInputSign(GizmoAxis axis)
        {
            return axis == GizmoAxis::Z ? -1.0f : 1.0f;
        }

        int GetAxisIndex(GizmoAxis axis)
        {
            switch (axis)
            {
            case GizmoAxis::X:
                return 0;
            case GizmoAxis::Y:
                return 1;
            case GizmoAxis::Z:
                return 2;
            case GizmoAxis::None:
            default:
                return 0;
            }
        }

        void AddAxisComponent(Math::Vector3& value, GizmoAxis axis, float amount)
        {
            switch (axis)
            {
            case GizmoAxis::X:
                value.x += amount;
                break;
            case GizmoAxis::Y:
                value.y += amount;
                break;
            case GizmoAxis::Z:
                value.z += amount;
                break;
            case GizmoAxis::None:
            default:
                break;
            }
        }

        void SetAxisComponent(Math::Vector3& value, GizmoAxis axis, float component)
        {
            switch (axis)
            {
            case GizmoAxis::X:
                value.x = component;
                break;
            case GizmoAxis::Y:
                value.y = component;
                break;
            case GizmoAxis::Z:
                value.z = component;
                break;
            case GizmoAxis::None:
            default:
                break;
            }
        }

        ImVec2 ProjectWorldToScreen(const Scene::Camera& camera, const Math::Vector3& point, float width, float height)
        {
            const float aspectRatio = height <= 0.0f ? 1.0f : width / height;
            const DirectX::XMVECTOR projected = DirectX::XMVector3Project(
                DirectX::XMLoadFloat3(&point),
                0.0f,
                0.0f,
                width,
                height,
                0.0f,
                1.0f,
                camera.GetProjectionMatrix(aspectRatio),
                camera.GetViewMatrix(),
                DirectX::XMMatrixIdentity());

            DirectX::XMFLOAT3 result;
            DirectX::XMStoreFloat3(&result, projected);
            return { result.x, result.y };
        }

        float DistancePointToSegment(ImVec2 point, ImVec2 a, ImVec2 b)
        {
            const float abX = b.x - a.x;
            const float abY = b.y - a.y;
            const float lengthSquared = abX * abX + abY * abY;
            if (lengthSquared <= 0.0001f)
            {
                const float dx = point.x - a.x;
                const float dy = point.y - a.y;
                return std::sqrt(dx * dx + dy * dy);
            }

            const float apX = point.x - a.x;
            const float apY = point.y - a.y;
            const float t = std::clamp((apX * abX + apY * abY) / lengthSquared, 0.0f, 1.0f);
            const float closestX = a.x + abX * t;
            const float closestY = a.y + abY * t;
            const float dx = point.x - closestX;
            const float dy = point.y - closestY;
            return std::sqrt(dx * dx + dy * dy);
        }

        float DistancePointToProjectedCircle(const Scene::Camera& camera, ImVec2 mouse, const Math::Vector3& center, float radius, const Math::Vector3& axisA, const Math::Vector3& axisB, float width, float height)
        {
            constexpr int segments = 64;
            float bestDistance = 1000000.0f;
            ImVec2 previous = {};
            for (int i = 0; i <= segments; ++i)
            {
                const float t = DirectX::XM_2PI * static_cast<float>(i) / static_cast<float>(segments);
                const Math::Vector3 point =
                {
                    center.x + (axisA.x * std::cos(t) + axisB.x * std::sin(t)) * radius,
                    center.y + (axisA.y * std::cos(t) + axisB.y * std::sin(t)) * radius,
                    center.z + (axisA.z * std::cos(t) + axisB.z * std::sin(t)) * radius
                };
                const ImVec2 projected = ProjectWorldToScreen(camera, point, width, height);
                if (i > 0)
                {
                    bestDistance = std::min(bestDistance, DistancePointToSegment(mouse, previous, projected));
                }
                previous = projected;
            }
            return bestDistance;
        }

        bool GetRingMouseAngle(const Scene::Camera& camera, int mouseX, int mouseY, const Math::Vector3& center, const Math::Vector3& normal, const Math::Vector3& axisA, const Math::Vector3& axisB, float width, float height, float& outAngle)
        {
            const Physics::Ray ray = MakeMouseRayFromCamera(camera, mouseX, mouseY, width, height);
            const DirectX::XMVECTOR rayOrigin = DirectX::XMLoadFloat3(&ray.origin);
            const DirectX::XMVECTOR rayDirection = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&ray.direction));
            const DirectX::XMVECTOR planeCenter = DirectX::XMLoadFloat3(&center);
            const DirectX::XMVECTOR planeNormal = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&normal));
            const float denominator = DirectX::XMVectorGetX(DirectX::XMVector3Dot(rayDirection, planeNormal));
            if (std::abs(denominator) < 0.0001f)
            {
                return false;
            }

            const float t = DirectX::XMVectorGetX(DirectX::XMVector3Dot(DirectX::XMVectorSubtract(planeCenter, rayOrigin), planeNormal)) / denominator;
            if (t < 0.0f)
            {
                return false;
            }

            const DirectX::XMVECTOR hitPoint = DirectX::XMVectorAdd(rayOrigin, DirectX::XMVectorScale(rayDirection, t));
            const DirectX::XMVECTOR fromCenter = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(hitPoint, planeCenter));
            const DirectX::XMVECTOR basisA = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&axisA));
            const DirectX::XMVECTOR basisB = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&axisB));
            const float x = DirectX::XMVectorGetX(DirectX::XMVector3Dot(fromCenter, basisA));
            const float y = DirectX::XMVectorGetX(DirectX::XMVector3Dot(fromCenter, basisB));
            outAngle = std::atan2(y, x);
            return true;
        }

        float Dot(const Math::Vector3& a, const Math::Vector3& b)
        {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }

        Math::Vector3 Cross(const Math::Vector3& a, const Math::Vector3& b)
        {
            Math::Vector3 result;
            DirectX::XMStoreFloat3(&result, DirectX::XMVector3Cross(DirectX::XMLoadFloat3(&a), DirectX::XMLoadFloat3(&b)));
            return result;
        }

        bool NormalizeSafe(const Math::Vector3& value, Math::Vector3& outNormalized)
        {
            const DirectX::XMVECTOR vector = DirectX::XMLoadFloat3(&value);
            const float lengthSq = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(vector));
            if (lengthSq < 0.000001f)
            {
                return false;
            }

            DirectX::XMStoreFloat3(&outNormalized, DirectX::XMVector3Normalize(vector));
            return std::isfinite(outNormalized.x) && std::isfinite(outNormalized.y) && std::isfinite(outNormalized.z);
        }

        bool IntersectRayPlane(const Physics::Ray& ray, const Math::Vector3& planePoint, const Math::Vector3& planeNormal, Math::Vector3& outPoint)
        {
            Math::Vector3 normal;
            if (!NormalizeSafe(planeNormal, normal))
            {
                return false;
            }

            const float denominator = Dot(ray.direction, normal);
            if (std::abs(denominator) < 0.0001f)
            {
                return false;
            }

            const Math::Vector3 delta =
            {
                planePoint.x - ray.origin.x,
                planePoint.y - ray.origin.y,
                planePoint.z - ray.origin.z
            };
            const float t = Dot(delta, normal) / denominator;
            if (t < 0.0f || !std::isfinite(t))
            {
                return false;
            }

            outPoint =
            {
                ray.origin.x + ray.direction.x * t,
                ray.origin.y + ray.direction.y * t,
                ray.origin.z + ray.direction.z * t
            };
            return std::isfinite(outPoint.x) && std::isfinite(outPoint.y) && std::isfinite(outPoint.z);
        }

        float Distance(const Math::Vector3& a, const Math::Vector3& b)
        {
            const float x = a.x - b.x;
            const float y = a.y - b.y;
            const float z = a.z - b.z;
            return std::sqrt(x * x + y * y + z * z);
        }

        float GetGizmoRadiusForCamera(const Scene::Camera& camera, const Math::Vector3& center)
        {
            const float distance = Distance(camera.position, center);
            return std::clamp(distance * 0.18f, 0.35f, 20.0f);
        }

        bool IsFiniteRotation(const Math::Vector3& rotation)
        {
            return std::isfinite(rotation.x) && std::isfinite(rotation.y) && std::isfinite(rotation.z);
        }

        bool GetScreenCircleAngle(float centerX, float centerY, float mouseX, float mouseY, float& outAngle)
        {
            const float x = mouseX - centerX;
            const float y = mouseY - centerY;
            if ((x * x + y * y) < 9.0f)
            {
                return false;
            }

            outAngle = std::atan2(y, x);
            return std::isfinite(outAngle);
        }

        float NormalizeSignedAngle(float angle)
        {
            while (angle > DirectX::XM_PI)
            {
                angle -= DirectX::XM_2PI;
            }
            while (angle < -DirectX::XM_PI)
            {
                angle += DirectX::XM_2PI;
            }
            return angle;
        }

        Math::Vector3 QuaternionToPitchYawRoll(DirectX::FXMVECTOR quaternion)
        {
            DirectX::XMFLOAT4 q;
            DirectX::XMStoreFloat4(&q, DirectX::XMQuaternionNormalize(quaternion));

            const float pitchSin = std::clamp(2.0f * (q.w * q.x - q.y * q.z), -1.0f, 1.0f);
            Math::Vector3 result;
            result.x = std::asin(pitchSin);
            result.y = std::atan2(2.0f * (q.w * q.y + q.z * q.x), 1.0f - 2.0f * (q.x * q.x + q.y * q.y));
            result.z = std::atan2(2.0f * (q.w * q.z + q.x * q.y), 1.0f - 2.0f * (q.x * q.x + q.z * q.z));
            return result;
        }

        Math::Vector3 ApplyRotationAroundDisplayedRing(const Math::Vector3& startEuler, const Math::Vector3& ringAxis, float deltaAngle)
        {
            Math::Vector3 normalizedAxis;
            if (!NormalizeSafe(ringAxis, normalizedAxis) || !std::isfinite(deltaAngle))
            {
                return startEuler;
            }

            const DirectX::XMMATRIX startRotation = DirectX::XMMatrixRotationRollPitchYaw(startEuler.x, startEuler.y, startEuler.z);
            const DirectX::XMMATRIX deltaRotation = DirectX::XMMatrixRotationAxis(DirectX::XMLoadFloat3(&normalizedAxis), deltaAngle);
            const DirectX::XMMATRIX combinedRotation = startRotation * deltaRotation;
            return QuaternionToPitchYawRoll(DirectX::XMQuaternionRotationMatrix(combinedRotation));
        }

        bool DragAndInputFloat(const char* label, float* value, float speed, float minValue = 0.0f, float maxValue = 0.0f, const char* format = "%.3f")
        {
            bool changed = ImGui::DragFloat(label, value, speed, minValue, maxValue, format);
            std::string inputLabel = "Input##";
            inputLabel += label;
            changed = ImGui::InputFloat(inputLabel.c_str(), value, 0.0f, 0.0f, format) || changed;
            return changed;
        }

        bool DragAndInputFloat3(const char* label, float* values, float speed, float minValue = 0.0f, float maxValue = 0.0f, const char* format = "%.3f")
        {
            bool changed = ImGui::DragFloat3(label, values, speed, minValue, maxValue, format);
            std::string inputLabel = "Input##";
            inputLabel += label;
            changed = ImGui::InputFloat3(inputLabel.c_str(), values, format) || changed;
            return changed;
        }

        bool SliderAndInputFloat(const char* label, float* value, float minValue, float maxValue, const char* format = "%.3f")
        {
            bool changed = ImGui::SliderFloat(label, value, minValue, maxValue, format);
            std::string inputLabel = "Input##";
            inputLabel += label;
            changed = ImGui::InputFloat(inputLabel.c_str(), value, 0.0f, 0.0f, format) || changed;
            *value = std::clamp(*value, minValue, maxValue);
            return changed;
        }

        struct TrackedWidgetState
        {
            bool changed = false;
            bool activated = false;
            bool deactivated = false;
            bool deactivatedAfterEdit = false;
        };

        void TrackLastItem(TrackedWidgetState& state)
        {
            state.activated = ImGui::IsItemActivated() || state.activated;
            state.deactivated = ImGui::IsItemDeactivated() || state.deactivated;
            state.deactivatedAfterEdit = ImGui::IsItemDeactivatedAfterEdit() || state.deactivatedAfterEdit;
        }

        TrackedWidgetState DragAndInputFloat3Tracked(const char* label, float* values, float speed, float minValue = 0.0f, float maxValue = 0.0f, const char* format = "%.3f")
        {
            TrackedWidgetState state;
            state.changed = ImGui::DragFloat3(label, values, speed, minValue, maxValue, format);
            TrackLastItem(state);
            std::string inputLabel = "Input##";
            inputLabel += label;
            state.changed = ImGui::InputFloat3(inputLabel.c_str(), values, format) || state.changed;
            TrackLastItem(state);
            return state;
        }

        TrackedWidgetState SliderAndInputFloatTracked(const char* label, float* value, float minValue, float maxValue, const char* format = "%.3f")
        {
            TrackedWidgetState state;
            state.changed = ImGui::SliderFloat(label, value, minValue, maxValue, format);
            TrackLastItem(state);
            std::string inputLabel = "Input##";
            inputLabel += label;
            state.changed = ImGui::InputFloat(inputLabel.c_str(), value, 0.0f, 0.0f, format) || state.changed;
            TrackLastItem(state);
            *value = std::clamp(*value, minValue, maxValue);
            return state;
        }

        TrackedWidgetState ColorEdit4Tracked(const char* label, float* value)
        {
            TrackedWidgetState state;
            state.changed = ImGui::ColorEdit4(label, value);
            TrackLastItem(state);
            return state;
        }

        std::string StripTrailingDigits(const std::string& name)
        {
            std::size_t end = name.size();
            while (end > 0 && std::isdigit(static_cast<unsigned char>(name[end - 1])))
            {
                --end;
            }

            return end == 0 ? name : name.substr(0, end);
        }

        std::string MakeNumberedDuplicateName(const Scene::Scene& scene, const std::string& sourceName)
        {
            const std::string baseName = StripTrailingDigits(sourceName);
            int highestSuffix = 0;

            for (const std::unique_ptr<Scene::GameObject>& object : scene.GetGameObjects())
            {
                const std::string& objectName = object->GetName();
                if (objectName == baseName)
                {
                    highestSuffix = std::max(highestSuffix, 0);
                    continue;
                }

                if (objectName.rfind(baseName, 0) != 0)
                {
                    continue;
                }

                const std::string suffix = objectName.substr(baseName.size());
                if (suffix.empty() || !std::all_of(suffix.begin(), suffix.end(), [](unsigned char character)
                {
                    return std::isdigit(character) != 0;
                }))
                {
                    continue;
                }

                highestSuffix = std::max(highestSuffix, std::atoi(suffix.c_str()));
            }

            return baseName + std::to_string(highestSuffix + 1);
        }

        std::string SanitizeAssetName(std::string name, const char* fallback)
        {
            name = Trim(name);
            if (name.empty())
            {
                name = fallback;
            }

            for (char& character : name)
            {
                const unsigned char value = static_cast<unsigned char>(character);
                if (!std::isalnum(value) && character != '_' && character != '-')
                {
                    character = '_';
                }
            }

            return name;
        }

        std::wstring MakeMaterialAssetPath(const std::string& name)
        {
            std::string assetName = Trim(name);
            if (assetName.size() > 4 && _stricmp(assetName.substr(assetName.size() - 4).c_str(), ".mat") == 0)
            {
                assetName.resize(assetName.size() - 4);
            }

            return L"Assets/Materials/" + ToWideAscii(SanitizeAssetName(assetName, "Material")) + L".mat";
        }

        std::wstring MakeAssetPathInFolder(const std::wstring& folder, const std::string& name, const char* extension, const char* fallback)
        {
            std::string assetName = Trim(name);
            const std::size_t extensionLength = std::strlen(extension);
            if (assetName.size() > extensionLength
                && _stricmp(assetName.substr(assetName.size() - extensionLength).c_str(), extension) == 0)
            {
                assetName.resize(assetName.size() - extensionLength);
            }

            std::filesystem::path targetFolder = folder.empty() ? std::filesystem::path("Assets") : std::filesystem::path(folder);
            if (!std::filesystem::exists(targetFolder) || !std::filesystem::is_directory(targetFolder))
            {
                targetFolder = "Assets";
            }

            return (targetFolder / (SanitizeAssetName(assetName, fallback) + extension)).generic_wstring();
        }

        std::filesystem::path MakeUniquePath(const std::filesystem::path& desiredPath)
        {
            if (!std::filesystem::exists(desiredPath))
            {
                return desiredPath;
            }

            const std::filesystem::path parent = desiredPath.parent_path();
            const std::wstring stem = desiredPath.stem().wstring();
            const std::wstring extension = desiredPath.extension().wstring();
            for (int index = 1; index < 10000; ++index)
            {
                std::filesystem::path candidate = parent / (stem + L"_" + std::to_wstring(index) + extension);
                if (!std::filesystem::exists(candidate))
                {
                    return candidate;
                }
            }

            return desiredPath;
        }

        Resource::MaterialDesc BuildMaterialDesc(const Renderer::Material& material, const std::string& fallbackName)
        {
            Resource::MaterialDesc desc;
            desc.name = material.GetName().empty() ? fallbackName : material.GetName();
            desc.vertexShaderPath = material.GetVertexShaderPath().empty() ? L"Shaders/BasicVertex.hlsl" : material.GetVertexShaderPath();
            desc.pixelShaderPath = material.GetPixelShaderPath().empty() ? L"Shaders/BasicPixel.hlsl" : material.GetPixelShaderPath();
            desc.texturePath = material.GetBaseColorTexturePath();
            desc.roughnessTexturePath = material.GetRoughnessTexturePath();
            desc.metallicTexturePath = material.GetMetallicTexturePath();
            desc.normalTexturePath = material.GetNormalTexturePath();
            desc.baseColor = material.GetBaseColor();
            desc.roughness = material.GetRoughness();
            desc.metallic = material.GetMetallic();
            desc.normalStrength = material.GetNormalStrength();
            return desc;
        }

        std::shared_ptr<Renderer::Material> CloneMaterialInstance(const std::shared_ptr<Renderer::Material>& material, const std::string& fallbackName)
        {
            if (!material)
            {
                return nullptr;
            }

            std::shared_ptr<Renderer::Material> clone = material->Clone();
            if (clone && clone->GetName().empty())
            {
                clone->SetName(fallbackName);
            }
            return clone;
        }

        void CopyMaterialState(const Renderer::Material& source, Renderer::Material& destination)
        {
            destination.SetName(source.GetName());
            destination.SetVertexShader(source.GetVertexShader());
            destination.SetPixelShader(source.GetPixelShader());
            destination.SetVertexShaderPath(source.GetVertexShaderPath());
            destination.SetPixelShaderPath(source.GetPixelShaderPath());
            destination.SetBaseColor(source.GetBaseColor());
            destination.SetRoughness(source.GetRoughness());
            destination.SetMetallic(source.GetMetallic());
            destination.SetNormalStrength(source.GetNormalStrength());
            destination.SetBaseColorTexture(source.GetBaseColorTexture());
            destination.SetRoughnessTexture(source.GetRoughnessTexture());
            destination.SetMetallicTexture(source.GetMetallicTexture());
            destination.SetNormalTexture(source.GetNormalTexture());
            destination.SetBaseColorTexturePath(source.GetBaseColorTexturePath());
            destination.SetRoughnessTexturePath(source.GetRoughnessTexturePath());
            destination.SetMetallicTexturePath(source.GetMetallicTexturePath());
            destination.SetNormalTexturePath(source.GetNormalTexturePath());
        }

        Renderer::Material* EnsureEditableMaterialInstance(Scene::MeshRendererComponent& meshRenderer, const std::string& fallbackName)
        {
            std::shared_ptr<Renderer::Material> material = meshRenderer.GetMaterial();
            if (!material)
            {
                return nullptr;
            }

            const bool hasAssetReference = !meshRenderer.GetMaterialGuid().empty()
                || (!meshRenderer.GetMaterialPath().empty()
                    && HasExtension(std::filesystem::path(meshRenderer.GetMaterialPath()), { L".mat" }));
            if (material.use_count() > 1)
            {
                material = CloneMaterialInstance(material, fallbackName);
                meshRenderer.SetMaterial(material);
            }

            if (hasAssetReference)
            {
                meshRenderer.SetMaterialPath({});
                meshRenderer.SetMaterialGuid({});
            }

            return material.get();
        }

        const char* ToMaterialTextureSlotName(MaterialTextureSlot slot)
        {
            switch (slot)
            {
            case MaterialTextureSlot::Roughness:
                return "Roughness";
            case MaterialTextureSlot::Metallic:
                return "Metallic";
            case MaterialTextureSlot::Normal:
                return "Normal";
            case MaterialTextureSlot::BaseColor:
            default:
                return "Base";
            }
        }

        Resource::TextureLoadOptions MakeTextureLoadOptions(MaterialTextureSlot slot)
        {
            Resource::TextureLoadOptions options;
            options.sRGB = slot == MaterialTextureSlot::BaseColor;
            return options;
        }

        const std::wstring& GetMaterialTextureSlotPath(const Renderer::Material& material, MaterialTextureSlot slot)
        {
            switch (slot)
            {
            case MaterialTextureSlot::Roughness:
                return material.GetRoughnessTexturePath();
            case MaterialTextureSlot::Metallic:
                return material.GetMetallicTexturePath();
            case MaterialTextureSlot::Normal:
                return material.GetNormalTexturePath();
            case MaterialTextureSlot::BaseColor:
            default:
                return material.GetBaseColorTexturePath();
            }
        }

        enum class PrimitiveShape
        {
            Cube,
            Sphere,
            Cylinder,
            Cone,
            Plane
        };

        const char* ToPrimitiveShapeName(PrimitiveShape shape)
        {
            switch (shape)
            {
            case PrimitiveShape::Sphere:
                return "Sphere";
            case PrimitiveShape::Cylinder:
                return "Cylinder";
            case PrimitiveShape::Cone:
                return "Cone";
            case PrimitiveShape::Plane:
                return "Plane";
            case PrimitiveShape::Cube:
            default:
                return "Cube";
            }
        }

        const char* ToPrimitiveShapeKey(PrimitiveShape shape)
        {
            switch (shape)
            {
            case PrimitiveShape::Sphere:
                return "sphere";
            case PrimitiveShape::Cylinder:
                return "cylinder";
            case PrimitiveShape::Cone:
                return "cone";
            case PrimitiveShape::Plane:
                return "plane";
            case PrimitiveShape::Cube:
            default:
                return "cube";
            }
        }

        std::wstring GetPrimitiveMeshPath(PrimitiveShape shape)
        {
            switch (shape)
            {
            case PrimitiveShape::Sphere:
                return L"builtin:mesh:sphere";
            case PrimitiveShape::Cylinder:
                return L"builtin:mesh:cylinder";
            case PrimitiveShape::Cone:
                return L"builtin:mesh:cone";
            case PrimitiveShape::Plane:
                return L"builtin:mesh:plane";
            case PrimitiveShape::Cube:
            default:
                return L"builtin:mesh:cube";
            }
        }

        Math::Vector4 GetPrimitiveMaterialColor(PrimitiveShape shape)
        {
            switch (shape)
            {
            case PrimitiveShape::Sphere:
                return { 0.68f, 0.84f, 1.0f, 1.0f };
            case PrimitiveShape::Cylinder:
                return { 0.78f, 0.95f, 0.74f, 1.0f };
            case PrimitiveShape::Cone:
                return { 1.0f, 0.78f, 0.56f, 1.0f };
            case PrimitiveShape::Plane:
                return { 0.86f, 0.86f, 0.86f, 1.0f };
            case PrimitiveShape::Cube:
            default:
                return { 1.0f, 1.0f, 1.0f, 1.0f };
            }
        }

        std::shared_ptr<Renderer::Mesh> CreatePrimitiveMesh(Resource::ResourceManager& resources, PrimitiveShape shape)
        {
            switch (shape)
            {
            case PrimitiveShape::Sphere:
                return resources.CreateSphereMesh();
            case PrimitiveShape::Cylinder:
                return resources.CreateCylinderMesh();
            case PrimitiveShape::Cone:
                return resources.CreateConeMesh();
            case PrimitiveShape::Plane:
                return resources.CreatePlaneMesh();
            case PrimitiveShape::Cube:
            default:
                return resources.CreateCubeMesh();
            }
        }

        ColliderSnapshot CreatePrimitiveCollider(PrimitiveShape shape)
        {
            ColliderSnapshot collider;
            if (shape == PrimitiveShape::Sphere)
            {
                collider.type = Physics::ColliderType::Sphere;
                collider.radius = 0.5f;
                collider.size = { 1.0f, 1.0f, 1.0f };
                return collider;
            }
            if (shape == PrimitiveShape::Plane)
            {
                collider.size = { 1.0f, 0.02f, 1.0f };
                return collider;
            }

            collider.size = { 1.0f, 1.0f, 1.0f };
            return collider;
        }

        void ConfigurePrimitiveSnapshot(GameObjectSnapshot& snapshot, Resource::ResourceManager& resources, PrimitiveShape shape)
        {
            const std::string shapeKey = ToPrimitiveShapeKey(shape);
            const Math::Vector4 materialColor = GetPrimitiveMaterialColor(shape);
            snapshot.name = ToPrimitiveShapeName(shape);
            snapshot.meshRenderer.enabled = true;
            snapshot.meshRenderer.mesh = CreatePrimitiveMesh(resources, shape);
            snapshot.meshRenderer.material = resources.CreateMaterial("editor:material:" + shapeKey, materialColor);
            snapshot.meshRenderer.meshPath = GetPrimitiveMeshPath(shape);
            snapshot.meshRenderer.materialPath = ToWideAscii("editor:material:" + shapeKey);
            snapshot.meshRenderer.hasMaterialState = true;
            snapshot.meshRenderer.materialBaseColor = materialColor;
            snapshot.meshRenderer.materialRoughness = 0.5f;
            snapshot.meshRenderer.materialMetallic = 0.0f;
            snapshot.meshRenderer.materialNormalStrength = 1.0f;
            snapshot.colliders.push_back(CreatePrimitiveCollider(shape));
        }

        const char* GetAssetTypeLabel(const std::wstring& asset)
        {
            const std::filesystem::path path(asset);
            if (HasExtension(path, { L".obj" }))
            {
                return "MODEL";
            }
            if (HasExtension(path, { L".mat" }))
            {
                return "MAT";
            }
            if (HasExtension(path, { L".png", L".jpg", L".jpeg", L".bmp", L".tga" }))
            {
                return "TEX";
            }
            if (HasExtension(path, { L".scene" }))
            {
                return "SCENE";
            }
            return "FILE";
        }

        std::string GetAssetFileName(const std::wstring& asset)
        {
            return ToNarrowAscii(std::filesystem::path(asset).filename().wstring());
        }

        bool IsAssetInFolder(const std::wstring& asset, const std::wstring& folder)
        {
            return std::filesystem::path(asset).parent_path().generic_wstring() == folder;
        }

        bool IsKnownAssetPath(const std::wstring& asset)
        {
            const std::filesystem::path path(asset);
            return HasExtension(path, { L".obj", L".mat", L".scene", L".png", L".jpg", L".jpeg", L".bmp", L".tga" });
        }

        ImTextureRef MakeExternalTextureRef(void* nativeTexture)
        {
            return ImTextureRef(static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(nativeTexture)));
        }

        const char* GetAssetDragPayloadType(const std::filesystem::path& path)
        {
            if (HasExtension(path, { L".png", L".jpg", L".jpeg", L".bmp", L".tga" }))
            {
                return "ASSET_TEXTURE";
            }
            if (HasExtension(path, { L".mat" }))
            {
                return "ASSET_MATERIAL";
            }
            return nullptr;
        }

        void EmitAssetDragDropSource(const char* payloadType, const std::wstring& asset)
        {
            if (!payloadType || !ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                return;
            }

            ImGui::SetDragDropPayload(payloadType, asset.c_str(), (asset.size() + 1) * sizeof(wchar_t));
            const std::string display = ToNarrowAscii(asset);
            ImGui::TextUnformatted(display.c_str());
            ImGui::EndDragDropSource();
        }

        std::wstring ReadWideAssetPayload(const ImGuiPayload* payload)
        {
            if (!payload || !payload->Data || payload->DataSize <= 0)
            {
                return {};
            }

            const int wcharCount = payload->DataSize / static_cast<int>(sizeof(wchar_t));
            if (wcharCount <= 0)
            {
                return {};
            }

            const wchar_t* data = static_cast<const wchar_t*>(payload->Data);
            int length = wcharCount;
            while (length > 0 && data[length - 1] == L'\0')
            {
                --length;
            }

            return length > 0 ? std::wstring(data, data + length) : std::wstring();
        }

        std::wstring NormalizeAssetPath(const std::wstring& path)
        {
            return std::filesystem::path(path).lexically_normal().generic_wstring();
        }

        std::shared_ptr<Renderer::Material> CloneMaterialInstance(const Renderer::Material& source)
        {
            std::shared_ptr<Renderer::Material> clone = std::make_shared<Renderer::Material>();
            clone->SetName(source.GetName().empty() ? "MaterialInstance" : source.GetName() + "_Instance");
            clone->SetVertexShader(source.GetVertexShader());
            clone->SetPixelShader(source.GetPixelShader());
            clone->SetVertexShaderPath(source.GetVertexShaderPath());
            clone->SetPixelShaderPath(source.GetPixelShaderPath());
            clone->SetBaseColor(source.GetBaseColor());
            clone->SetRoughness(source.GetRoughness());
            clone->SetMetallic(source.GetMetallic());
            clone->SetNormalStrength(source.GetNormalStrength());
            clone->SetBaseColorTexture(source.GetBaseColorTexture());
            clone->SetRoughnessTexture(source.GetRoughnessTexture());
            clone->SetMetallicTexture(source.GetMetallicTexture());
            clone->SetNormalTexture(source.GetNormalTexture());
            clone->SetBaseColorTexturePath(source.GetBaseColorTexturePath());
            clone->SetRoughnessTexturePath(source.GetRoughnessTexturePath());
            clone->SetMetallicTexturePath(source.GetMetallicTexturePath());
            clone->SetNormalTexturePath(source.GetNormalTexturePath());
            return clone;
        }
    }

    EditorLayer::EditorLayer() = default;

    EditorLayer::~EditorLayer() = default;

    bool EditorLayer::Initialize()
    {
        Core::Log::Info("EditorLayer initialized.");
        m_dirtyManager.SetAssetDatabase(&m_assetDatabase);
        strncpy_s(m_newMaterialNameBuffer.data(), m_newMaterialNameBuffer.size(), "NewMaterial", _TRUNCATE);
        strncpy_s(m_newSceneNameBuffer.data(), m_newSceneNameBuffer.size(), "NewScene", _TRUNCATE);
        strncpy_s(m_saveMaterialAsBuffer.data(), m_saveMaterialAsBuffer.size(), "SavedMaterial", _TRUNCATE);
        RefreshAssets();
        return true;
    }

    void EditorLayer::Shutdown()
    {
        m_selectedObject = nullptr;
        m_selectedObjects.clear();
        m_selectedAssets.clear();
        if (m_commandManager)
        {
            m_commandManager->Clear();
        }
    }

    void EditorLayer::EnsureCommandManager(Scene::Scene& scene)
    {
        if (!m_commandManager)
        {
            m_commandManager = std::make_unique<CommandManager>(scene);
        }
    }

    void EditorLayer::ShowNotification(const std::string& text, float seconds)
    {
        m_notificationText = text;
        m_notificationTimer = std::max(0.0f, seconds);
    }

    void EditorLayer::RequestExit(Scene::Scene& scene)
    {
        if (CheckSaveBeforeDestructiveAction(scene, PendingEditorAction::Exit))
        {
            m_exitRequested = true;
        }
    }

    bool EditorLayer::ConsumeExitRequested()
    {
        const bool requested = m_exitRequested;
        m_exitRequested = false;
        return requested;
    }

    std::wstring EditorLayer::BuildWindowTitle(const std::wstring& baseTitle, Scene::Scene& scene)
    {
        m_dirtyManager.SetCurrentScene(&scene);
        m_dirtyManager.SetAssetDatabase(&m_assetDatabase);
        return m_dirtyManager.BuildWindowTitle(baseTitle);
    }

    void EditorLayer::Update(Scene::Scene& scene, const Debug::DebugStats&, Input::Input& input, float deltaTime)
    {
        EnsureCommandManager(scene);
        m_staticMeshEditor.OnUpdate(deltaTime);
        ValidateSelection(scene);
        if (m_notificationTimer > 0.0f)
        {
            m_notificationTimer = std::max(0.0f, m_notificationTimer - deltaTime);
        }

        const ImGuiIO& io = ImGui::GetIO();
        const bool mouseOverEditorUi = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
        m_wantsInputCapture = io.WantTextInput;

        if (input.GetKeyDown('P'))
        {
            m_playMode = !m_playMode;
            Core::Log::Info(m_playMode ? "Play Mode enabled." : "Play Mode disabled.");
        }

        const bool controlPressed = input.GetKeyRaw(Input::KeyControl) || io.KeyCtrl;
        const bool shiftPressed = input.GetKeyRaw(Input::KeyShift) || io.KeyShift;
        if (!io.WantTextInput && controlPressed && input.GetKeyDown('Z'))
        {
            if (shiftPressed && m_commandManager)
            {
                m_commandManager->Redo();
            }
            else if (m_commandManager)
            {
                m_commandManager->Undo();
            }
            return;
        }
        if (!io.WantTextInput && controlPressed && input.GetKeyDown('Y'))
        {
            if (m_commandManager)
            {
                m_commandManager->Redo();
            }
            return;
        }
        if (!io.WantTextInput && controlPressed && input.GetKeyDown(Input::KeyS))
        {
            if (shiftPressed)
            {
                SaveCurrentSceneAs(scene, MakeAssetPathInFolder(m_selectedAssetFolder, m_newSceneNameBuffer.data(), ".scene", "Scene"));
            }
            else
            {
                SaveCurrentScene(scene);
            }
            return;
        }
        if (!io.WantTextInput && controlPressed && input.GetKeyDown('N'))
        {
            RequestNewScene(scene);
            return;
        }
        if (!io.WantTextInput && controlPressed && input.GetKeyDown('O'))
        {
            if (!m_selectedSceneAsset.empty())
            {
                CheckSaveBeforeDestructiveAction(scene, PendingEditorAction::OpenScene, m_selectedSceneAsset);
            }
            return;
        }

        if (!io.WantTextInput && m_selectedObject && input.GetKeyDown(Input::KeyDelete))
        {
            DeleteSelectedObjects(scene);
            return;
        }

        if (m_activeGizmoAxis != GizmoAxis::None)
        {
            m_hoveredGizmoAxis = m_activeGizmoAxis;
            if (input.GetMouseButtonRaw(Input::MouseButtonLeft) && m_selectedObject && scene.GetActiveCamera())
            {
                ApplyGizmoDrag(*scene.GetActiveCamera(), input, io.DisplaySize.x, io.DisplaySize.y);
            }
            else
            {
                EndGizmoDrag();
            }
            return;
        }

        const bool altPressed = input.GetKeyRaw(Input::KeyAlt) || io.KeyAlt;
        const bool rightMouseHeld = input.GetMouseButtonRaw(Input::MouseButtonRight);
        if (!io.WantTextInput && !controlPressed && !shiftPressed && !altPressed && !rightMouseHeld)
        {
            if (input.GetKeyDown(Input::KeyW))
            {
                m_gizmoMode = GizmoMode::Move;
                m_showGizmoTools = true;
            }
            else if (input.GetKeyDown(Input::KeyR))
            {
                m_gizmoMode = GizmoMode::Rotate;
                m_showGizmoTools = true;
            }
            else if (input.GetKeyDown(Input::KeyS))
            {
                m_gizmoMode = GizmoMode::Scale;
                m_showGizmoTools = true;
            }
        }

        m_hoveredGizmoAxis = GizmoAxis::None;
        if (!mouseOverEditorUi && m_showGizmoTools && m_selectedObject && scene.GetActiveCamera())
        {
            m_hoveredGizmoAxis = PickGizmoAxis(*scene.GetActiveCamera(), input.GetMouseX(), input.GetMouseY(), io.DisplaySize.x, io.DisplaySize.y);
        }

        if (!mouseOverEditorUi && m_showGizmoTools && m_selectedObject && input.GetMouseButtonDownRaw(Input::MouseButtonLeft) && scene.GetActiveCamera())
        {
            m_activeGizmoAxis = m_hoveredGizmoAxis != GizmoAxis::None
                ? m_hoveredGizmoAxis
                : PickGizmoAxis(*scene.GetActiveCamera(), input.GetMouseX(), input.GetMouseY(), io.DisplaySize.x, io.DisplaySize.y);
            if (m_activeGizmoAxis != GizmoAxis::None)
            {
                if (altPressed)
                {
                    DuplicateSelectedObject(scene);
                }

                BeginGizmoDrag(*scene.GetActiveCamera(), input, io.DisplaySize.x, io.DisplaySize.y);
                return;
            }
        }

        if (!mouseOverEditorUi && input.GetMouseButtonDownRaw(Input::MouseButtonLeft) && scene.GetActiveCamera())
        {
            Physics::RaycastHit hit;
            const Physics::Ray ray = MakeMouseRayFromCamera(*scene.GetActiveCamera(), input.GetMouseX(), input.GetMouseY(), io.DisplaySize.x, io.DisplaySize.y);
            bool hasHit = RaycastSceneMeshes(scene, ray, hit);
            if (!hasHit)
            {
                Physics::PhysicsWorld physicsWorld;
                physicsWorld.BuildFromScene(scene);
                hasHit = physicsWorld.Raycast(ray, hit);
            }
            if (hasHit)
            {
                if (io.KeyShift)
                {
                    ToggleSelectedObject(hit.object);
                }
                else
                {
                    SetSingleSelectedObject(hit.object);
                }
            }
            else if (!io.KeyShift)
            {
                SetSingleSelectedObject(nullptr);
                m_outlinerSelectionAnchor = nullptr;
                m_activeGizmoAxis = GizmoAxis::None;
            }
        }
    }

    void EditorLayer::Render(Scene::Scene& scene, const Debug::DebugStats& stats, Renderer::Renderer& renderer)
    {
        EnsureCommandManager(scene);
        m_dirtyManager.SetCurrentScene(&scene);
        m_dirtyManager.SetAssetDatabase(&m_assetDatabase);
        ValidateSelection(scene);

        ImGuiIO& io = ImGui::GetIO();
        m_wantsInputCapture = io.WantTextInput;
        const float menuHeight = ImGui::GetFrameHeight();
        const float viewportToolbarHeight = 34.0f;
        const float statusBarHeight = 26.0f;
        m_toolbarHeight = std::clamp(m_toolbarHeight, 44.0f, 150.0f);
        m_rightPanelWidth = std::clamp(m_rightPanelWidth, 300.0f, std::max(300.0f, io.DisplaySize.x - 420.0f));
        m_bottomPanelHeight = std::clamp(m_bottomPanelHeight, 150.0f, std::max(150.0f, io.DisplaySize.y - 260.0f));
        m_hierarchyPanelRatio = std::clamp(m_hierarchyPanelRatio, 0.18f, 0.72f);
        const float bottomPanelHeight = m_showAssetBrowser ? m_bottomPanelHeight : statusBarHeight;
        const float toolbarHeight = m_toolbarHeight;
        const float rightPanelWidth = m_rightPanelWidth;
        const float rightPanelX = io.DisplaySize.x - rightPanelWidth;
        const float sideTop = menuHeight + toolbarHeight;
        const float sideHeight = std::max(120.0f, io.DisplaySize.y - sideTop - bottomPanelHeight);
        const float hierarchyHeight = std::clamp(sideHeight * m_hierarchyPanelRatio, 100.0f, std::max(100.0f, sideHeight - 120.0f));
        const float inspectorY = sideTop + hierarchyHeight;
        const float inspectorHeight = std::max(120.0f, sideHeight - hierarchyHeight);
        const float viewportWidth = std::max(100.0f, io.DisplaySize.x - rightPanelWidth);
        constexpr float splitterThickness = 12.0f;
        const ImGuiWindowFlags fixedPanelFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
        const ImGuiWindowFlags splitterFlags = ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoScrollbar
            | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoBringToFrontOnFocus;

        auto drawSplitter = [&](const char* id, const ImVec2& pos, const ImVec2& size, ImGuiMouseCursor cursor, auto&& onDrag)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(size, ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.0f);
            ImGui::Begin(id, nullptr, splitterFlags);
            ImGui::InvisibleButton("drag", size);
            const bool active = ImGui::IsItemActive();
            const bool hovered = ImGui::IsItemHovered();
            if (hovered || active)
            {
                ImGui::SetMouseCursor(cursor);
                const ImVec2 min = ImGui::GetItemRectMin();
                const ImVec2 max = ImGui::GetItemRectMax();
                ImGui::GetWindowDrawList()->AddRectFilled(min, max, IM_COL32(80, 140, 220, active ? 150 : 90));
            }
            if (active)
            {
                m_wantsInputCapture = true;
                onDrag(io.MouseDelta);
            }
            ImGui::End();
            ImGui::PopStyleVar(2);
        };

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Scene"))
            {
                if (ImGui::MenuItem("New Scene", "Ctrl+N"))
                {
                    RequestNewScene(scene);
                }
                if (ImGui::MenuItem("Open Selected Scene", "Ctrl+O", false, !m_selectedSceneAsset.empty()))
                {
                    RequestOpenScene(scene, renderer, m_selectedSceneAsset);
                }
                if (ImGui::MenuItem("Save", "Ctrl+S"))
                {
                    SaveCurrentScene(scene);
                }
                if (ImGui::MenuItem("Save As", "Ctrl+Shift+S"))
                {
                    SaveCurrentSceneAs(scene, MakeAssetPathInFolder(m_selectedAssetFolder, m_newSceneNameBuffer.data(), ".scene", "Scene"));
                }

                ImGui::Separator();
                if (ImGui::MenuItem("Exit"))
                {
                    RequestExit(scene);
                }
                ImGui::Separator();
                ImGui::MenuItem("Play Mode", "P", &m_playMode);
                ImGui::MenuItem("Show Colliders", nullptr, &m_showColliders);
                ImGui::MenuItem("Asset Browser", nullptr, &m_showAssetBrowser);
                ImGui::MenuItem("Gizmo Tools", nullptr, &m_showGizmoTools);
                if (ImGui::BeginMenu("Selection Outline"))
                {
                    ImGui::MenuItem("Screen Space", nullptr, &m_screenSelectionOutline);
                    ImGui::MenuItem("Debug Lines", nullptr, &m_debugSelectionOutline);
                    ImGui::ColorEdit4("Color", m_selectionOutlineColor.data(), ImGuiColorEditFlags_NoInputs);
                    ImGui::SliderFloat("Width", &m_selectionOutlineWidth, 1.0f, 16.0f, "%.1f px");
                    ImGui::SliderFloat("Opacity", &m_selectionOutlineOpacity, 0.0f, 1.0f, "%.2f");
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                const bool canUndo = m_commandManager && m_commandManager->CanUndo();
                const bool canRedo = m_commandManager && m_commandManager->CanRedo();
                const std::string undoLabel = std::string("Undo ") + (canUndo ? m_commandManager->GetUndoName() : "");
                const std::string redoLabel = std::string("Redo ") + (canRedo ? m_commandManager->GetRedoName() : "");
                if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, canUndo))
                {
                    m_commandManager->Undo();
                }
                if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y / Ctrl+Shift+Z", false, canRedo))
                {
                    m_commandManager->Redo();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Window"))
            {
                if (ImGui::MenuItem("Static Mesh Editor"))
                {
                    if (!m_selectedAsset.empty() && HasExtension(std::filesystem::path(m_selectedAsset), { L".obj", L".gltf", L".glb", L".fbx" }))
                    {
                        m_staticMeshEditor.OpenByPath(m_selectedAsset, m_assetDatabase, renderer.GetResourceManager());
                    }
                    else
                    {
                        m_staticMeshEditor.OpenEmpty();
                    }
                }
                if (ImGui::MenuItem("Reset Layout"))
                {
                    m_rightPanelWidth = 430.0f;
                    m_bottomPanelHeight = 280.0f;
                    m_hierarchyPanelRatio = 0.36f;
                    m_toolbarHeight = 78.0f;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        ImGui::SetNextWindowPos(ImVec2(0.0f, menuHeight), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, toolbarHeight), ImGuiCond_Always);
        RenderObjectTools(scene, renderer);

        drawSplitter("##ToolbarSplitter",
            ImVec2(0.0f, menuHeight + toolbarHeight - splitterThickness * 0.5f),
            ImVec2(io.DisplaySize.x, splitterThickness),
            ImGuiMouseCursor_ResizeNS,
            [&](const ImVec2& delta)
            {
                m_toolbarHeight += delta.y;
            });

        ImGui::SetNextWindowPos(ImVec2(0.0f, menuHeight + toolbarHeight), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(viewportWidth, viewportToolbarHeight), ImGuiCond_Always);
        if (ImGui::Begin("Viewport Toolbar", nullptr, fixedPanelFlags | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar))
        {
            ImGui::Text("Scene: %s", scene.GetDirtyDisplayName().c_str());
            ImGui::SameLine();
            const char* gizmoModeText = m_gizmoMode == GizmoMode::Move ? "Move(W)" : (m_gizmoMode == GizmoMode::Rotate ? "Rotate(R)" : "Scale(S)");
            ImGui::Text("Gizmo %s", gizmoModeText);
            ImGui::SameLine();
            ImGui::TextUnformatted("Perspective");
            ImGui::SameLine();
            ImGui::TextUnformatted("Lit");
            ImGui::SameLine();
            ImGui::Text("Snap %.1f / %.1f / %.1f", m_moveSnapStep, m_rotateSnapStep, m_scaleSnapStep);
            ImGui::SameLine();
            ImGui::Text("Cam %.1f %.1f %.1f", stats.cameraPosition.x, stats.cameraPosition.y, stats.cameraPosition.z);
        }
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(rightPanelX, sideTop), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(rightPanelWidth, hierarchyHeight), ImGuiCond_Always);
        ImGui::Begin("Outliner", nullptr, fixedPanelFlags);
        if (ImGui::Button("+ Folder"))
        {
            CreateOutlinerFolder(scene);
        }
        ImGui::SameLine();
        if (ImGui::Button("Move To Root") && !m_selectedObjects.empty())
        {
            MoveObjectsToFolder(scene, m_selectedObjects, {});
        }

        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_F2))
        {
            if (m_selectedObject)
            {
                BeginRenameSelected();
            }
            else if (!m_selectedOutlinerFolder.empty())
            {
                BeginRenameSelectedFolder();
            }
        }

        std::vector<Scene::GameObject*> outlinerObjectOrder;
        outlinerObjectOrder.reserve(scene.GetGameObjects().size());
        for (const std::unique_ptr<Scene::GameObject>& object : scene.GetGameObjects())
        {
            if (object->GetOutlinerFolder().empty())
            {
                outlinerObjectOrder.push_back(object.get());
            }
        }
        const std::vector<std::string> folders = scene.GetOutlinerFolders();
        for (const std::string& folder : folders)
        {
            for (const std::unique_ptr<Scene::GameObject>& object : scene.GetGameObjects())
            {
                if (object->GetOutlinerFolder() == folder)
                {
                    outlinerObjectOrder.push_back(object.get());
                }
            }
        }

        auto selectOutlinerRange = [&](Scene::GameObject* object)
        {
            if (!object)
            {
                return;
            }

            auto targetIt = std::find(outlinerObjectOrder.begin(), outlinerObjectOrder.end(), object);
            auto anchorIt = std::find(outlinerObjectOrder.begin(), outlinerObjectOrder.end(), m_outlinerSelectionAnchor);
            if (targetIt == outlinerObjectOrder.end() || anchorIt == outlinerObjectOrder.end())
            {
                SetSingleSelectedObject(object);
                m_outlinerSelectionAnchor = object;
                return;
            }

            const std::size_t targetIndex = static_cast<std::size_t>(std::distance(outlinerObjectOrder.begin(), targetIt));
            const std::size_t anchorIndex = static_cast<std::size_t>(std::distance(outlinerObjectOrder.begin(), anchorIt));
            const std::size_t first = std::min(anchorIndex, targetIndex);
            const std::size_t last = std::max(anchorIndex, targetIndex);

            m_selectedObjects.clear();
            for (std::size_t index = first; index <= last && index < outlinerObjectOrder.size(); ++index)
            {
                if (outlinerObjectOrder[index])
                {
                    m_selectedObjects.push_back(outlinerObjectOrder[index]);
                }
            }
            m_selectedObject = object;
        };

        auto drawObjectRow = [&](Scene::GameObject& object)
        {
            const bool selected = IsObjectSelected(&object);
            ImGui::PushID(&object);
            if (m_renamingObject == &object)
            {
                if (m_focusRenameInput)
                {
                    ImGui::SetKeyboardFocusHere();
                    m_focusRenameInput = false;
                }

                const bool enterPressed = ImGui::InputText("##Name", m_renameBuffer.data(), m_renameBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
                const bool escapePressed = ImGui::IsKeyPressed(ImGuiKey_Escape);
                if (enterPressed)
                {
                    CommitRename();
                }
                else if (escapePressed)
                {
                    m_renamingObject = nullptr;
                    m_focusRenameInput = false;
                }
                else if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    CommitRename();
                }
                else if (ImGui::IsItemDeactivated())
                {
                    m_renamingObject = nullptr;
                    m_focusRenameInput = false;
                }
            }
            else if (ImGui::Selectable(object.GetName().c_str(), selected))
            {
                m_selectedOutlinerFolder.clear();
                const ImGuiIO& rowIo = ImGui::GetIO();
                if (rowIo.KeyShift)
                {
                    selectOutlinerRange(&object);
                }
                else if (rowIo.KeyCtrl)
                {
                    ToggleSelectedObject(&object);
                    m_outlinerSelectionAnchor = &object;
                }
                else
                {
                    SetSingleSelectedObject(&object);
                    m_outlinerSelectionAnchor = &object;
                }
            }

            if (ImGui::BeginPopupContextItem("ObjectContext"))
            {
                std::vector<Scene::GameObject*> targets;
                if (IsObjectSelected(&object) && !m_selectedObjects.empty())
                {
                    targets = m_selectedObjects;
                }
                else
                {
                    targets.push_back(&object);
                }

                if (ImGui::MenuItem("Move To Root"))
                {
                    MoveObjectsToFolder(scene, targets, {});
                }
                if (ImGui::BeginMenu("Move To Folder"))
                {
                    for (const std::string& folder : scene.GetOutlinerFolders())
                    {
                        if (ImGui::MenuItem(folder.c_str()))
                        {
                            MoveObjectsToFolder(scene, targets, folder);
                        }
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndPopup();
            }
            ImGui::PopID();
        };

        auto drawFolder = [&](const std::string& folder)
        {
            ImGui::PushID(folder.c_str());
            const bool selected = !folder.empty() && m_selectedOutlinerFolder == folder && !m_selectedObject;
            if (m_renamingOutlinerFolder == folder)
            {
                if (m_focusFolderRenameInput)
                {
                    ImGui::SetKeyboardFocusHere();
                    m_focusFolderRenameInput = false;
                }

                const bool enterPressed = ImGui::InputText("##FolderName", m_folderRenameBuffer.data(), m_folderRenameBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
                const bool escapePressed = ImGui::IsKeyPressed(ImGuiKey_Escape);
                if (enterPressed)
                {
                    CommitFolderRename(scene);
                }
                else if (escapePressed)
                {
                    m_renamingOutlinerFolder.clear();
                    m_focusFolderRenameInput = false;
                }
                else if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    CommitFolderRename(scene);
                }
                else if (ImGui::IsItemDeactivated())
                {
                    m_renamingOutlinerFolder.clear();
                    m_focusFolderRenameInput = false;
                }
            }
            else
            {
                const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
                    | ImGuiTreeNodeFlags_DefaultOpen
                    | (selected ? ImGuiTreeNodeFlags_Selected : 0);
                const bool open = ImGui::TreeNodeEx(folder.c_str(), flags);
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                {
                    m_selectedOutlinerFolder = folder;
                    SetSingleSelectedObject(nullptr);
                    m_outlinerSelectionAnchor = nullptr;
                }
                if (ImGui::BeginPopupContextItem("FolderContext"))
                {
                    const bool hasSelection = !m_selectedObjects.empty();
                    if (ImGui::MenuItem("Move Selection Here", nullptr, false, hasSelection))
                    {
                        MoveObjectsToFolder(scene, m_selectedObjects, folder);
                    }
                    if (ImGui::MenuItem("Rename Folder"))
                    {
                        m_selectedOutlinerFolder = folder;
                        SetSingleSelectedObject(nullptr);
                        m_outlinerSelectionAnchor = nullptr;
                        BeginRenameSelectedFolder();
                    }
                    if (ImGui::MenuItem("Delete Folder"))
                    {
                        scene.RemoveOutlinerFolder(folder);
                        scene.MarkDirty();
                        if (m_selectedOutlinerFolder == folder)
                        {
                            m_selectedOutlinerFolder.clear();
                        }
                    }
                    ImGui::EndPopup();
                }

                if (open)
                {
                    for (const std::unique_ptr<Scene::GameObject>& object : scene.GetGameObjects())
                    {
                        if (object->GetOutlinerFolder() == folder)
                        {
                            drawObjectRow(*object);
                        }
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::PopID();
        };

        if (ImGui::TreeNodeEx("Scene Root", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow))
        {
            for (const std::unique_ptr<Scene::GameObject>& object : scene.GetGameObjects())
            {
                if (object->GetOutlinerFolder().empty())
                {
                    drawObjectRow(*object);
                }
            }
            ImGui::TreePop();
        }

        for (const std::string& folder : folders)
        {
            drawFolder(folder);
        }
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(rightPanelX, inspectorY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(rightPanelWidth, inspectorHeight), ImGuiCond_Always);
        ImGui::Begin("Details", nullptr, fixedPanelFlags);
        if (m_selectedObject)
        {
            if (m_selectedObjects.size() > 1)
            {
                ImGui::Text("Selected: %d objects", static_cast<int>(m_selectedObjects.size()));
            }
            ImGui::TextUnformatted(m_selectedObject->GetName().c_str());

            auto trackPropertyEdit = [&](Scene::ObjectID objectId,
                Scene::ComponentID componentId,
                const std::string& propertyPath,
                const PropertyValue& valueBeforeWidget,
                const PropertyValue& valueAfterWidget,
                const TrackedWidgetState& widgetState)
            {
                if (widgetState.activated)
                {
                    m_hasActivePropertyEdit = true;
                    m_activePropertyObjectId = objectId;
                    m_activePropertyComponentId = componentId;
                    m_activePropertyPath = propertyPath;
                    m_activePropertyStartValue = valueBeforeWidget;
                }

                if (widgetState.changed)
                {
                    scene.MarkDirty();
                }

                const bool sameProperty = m_hasActivePropertyEdit
                    && m_activePropertyObjectId == objectId
                    && m_activePropertyComponentId == componentId
                    && m_activePropertyPath == propertyPath;

                if (widgetState.deactivatedAfterEdit)
                {
                    const PropertyValue oldValue = sameProperty ? m_activePropertyStartValue : valueBeforeWidget;
                    PropertySystem::SetProperty(*m_commandManager, scene, objectId, componentId, propertyPath, oldValue, valueAfterWidget);
                    m_hasActivePropertyEdit = false;
                    m_activePropertyObjectId = Scene::InvalidObjectID;
                    m_activePropertyComponentId = Scene::InvalidComponentID;
                    m_activePropertyPath.clear();
                }
                else if (widgetState.deactivated && sameProperty)
                {
                    m_hasActivePropertyEdit = false;
                    m_activePropertyObjectId = Scene::InvalidObjectID;
                    m_activePropertyComponentId = Scene::InvalidComponentID;
                    m_activePropertyPath.clear();
                }
            };

            Math::Transform& transform = m_selectedObject->GetTransform();
            const Scene::ObjectID selectedObjectId = m_selectedObject->GetID();
            Math::Vector3 position = transform.position;
            const Math::Vector3 positionBefore = position;
            const TrackedWidgetState positionState = DragAndInputFloat3Tracked("Position", &position.x, 0.05f);
            if (positionState.changed)
            {
                transform.position = position;
            }
            trackPropertyEdit(selectedObjectId, Scene::InvalidComponentID, "Transform.Position", positionBefore, transform.position, positionState);

            Math::Vector3 rotationDegrees =
            {
                DirectX::XMConvertToDegrees(transform.rotation.x),
                DirectX::XMConvertToDegrees(transform.rotation.y),
                DirectX::XMConvertToDegrees(transform.rotation.z)
            };
            const Math::Vector3 rotationBefore = transform.rotation;
            const TrackedWidgetState rotationState = DragAndInputFloat3Tracked("Rotation (deg)", &rotationDegrees.x, 1.0f);
            if (rotationState.changed)
            {
                transform.rotation =
                {
                    DirectX::XMConvertToRadians(rotationDegrees.x),
                    DirectX::XMConvertToRadians(rotationDegrees.y),
                    DirectX::XMConvertToRadians(rotationDegrees.z)
                };
            }
            trackPropertyEdit(selectedObjectId, Scene::InvalidComponentID, "Transform.Rotation", rotationBefore, transform.rotation, rotationState);

            Math::Vector3 scale = transform.scale;
            const Math::Vector3 scaleBefore = scale;
            const TrackedWidgetState scaleState = DragAndInputFloat3Tracked("Scale", &scale.x, 0.05f, 0.01f, 100.0f);
            if (scaleState.changed)
            {
                transform.scale.x = std::max(0.01f, scale.x);
                transform.scale.y = std::max(0.01f, scale.y);
                transform.scale.z = std::max(0.01f, scale.z);
            }
            trackPropertyEdit(selectedObjectId, Scene::InvalidComponentID, "Transform.Scale", scaleBefore, transform.scale, scaleState);

            auto applyInspectorSnapshotEdit = [&](const GameObjectSnapshot& before, const GameObjectSnapshot& after, const char* notification)
            {
                if (!m_selectedObject || !GameObjectSnapshot::IsDifferent(before, after))
                {
                    return false;
                }

                m_inspectorEditObject = nullptr;
                m_inspectorTransformObject = nullptr;
                if (m_commandManager)
                {
                    m_commandManager->ExecuteCommand(std::make_unique<InspectorEditCommand>(*m_selectedObject, before, after));
                }
                else
                {
                    after.ApplyTo(*m_selectedObject);
                }
                scene.MarkDirty();
                ShowNotification(notification);
                return true;
            };

            auto addMeshRendererComponent = [&]()
            {
                GameObjectSnapshot before = GameObjectSnapshot::Capture(*m_selectedObject);
                GameObjectSnapshot after = before;
                after.meshRenderer = MeshRendererSnapshot{};
                after.meshRenderer.enabled = true;
                after.meshRenderer.mesh = renderer.GetResourceManager().CreateCubeMesh("builtin:mesh:cube");
                after.meshRenderer.material = renderer.GetResourceManager().CreateMaterial("editor:material:default", { 1.0f, 1.0f, 1.0f, 1.0f });
                after.meshRenderer.meshPath = L"builtin:mesh:cube";
                after.meshRenderer.materialPath = L"editor:material:default";
                after.meshRenderer.hasMaterialState = true;
                after.meshRenderer.materialBaseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
                after.meshRenderer.materialRoughness = 0.5f;
                after.meshRenderer.materialMetallic = 0.0f;
                after.meshRenderer.materialNormalStrength = 1.0f;
                applyInspectorSnapshotEdit(before, after, "Added Mesh Renderer.");
            };

            auto addColliderComponent = [&]()
            {
                GameObjectSnapshot before = GameObjectSnapshot::Capture(*m_selectedObject);
                GameObjectSnapshot after = before;
                after.colliders.push_back(ColliderSnapshot{});
                applyInspectorSnapshotEdit(before, after, "Added Collider.");
            };

            auto addLightComponent = [&]()
            {
                GameObjectSnapshot before = GameObjectSnapshot::Capture(*m_selectedObject);
                GameObjectSnapshot after = before;
                after.light.enabled = true;
                applyInspectorSnapshotEdit(before, after, "Added Light.");
            };

            auto addPostProcessComponent = [&]()
            {
                GameObjectSnapshot before = GameObjectSnapshot::Capture(*m_selectedObject);
                GameObjectSnapshot after = before;
                after.postProcess.enabledComponent = true;
                after.postProcess.enabled = true;
                applyInspectorSnapshotEdit(before, after, "Added Post Process.");
            };

            auto addPlayerStartComponent = [&]()
            {
                GameObjectSnapshot before = GameObjectSnapshot::Capture(*m_selectedObject);
                GameObjectSnapshot after = before;
                after.playerStart.enabled = true;
                applyInspectorSnapshotEdit(before, after, "Added Player Start.");
            };

            auto drawComponentHeader = [](const char* label, const char* id)
            {
                ImGui::Separator();
                ImGui::TextUnformatted(label);
                ImGui::SameLine();
                const std::string buttonId = std::string("Remove##") + id;
                return ImGui::SmallButton(buttonId.c_str());
            };

            ImGui::Separator();
            if (ImGui::Button("Add Component"))
            {
                ImGui::OpenPopup("AddComponentMenu");
            }
            if (ImGui::BeginPopup("AddComponentMenu"))
            {
                if (ImGui::MenuItem("Mesh Renderer", nullptr, false, m_selectedObject->GetComponent<Scene::MeshRendererComponent>() == nullptr))
                {
                    addMeshRendererComponent();
                }
                if (ImGui::MenuItem("Collider", nullptr, false, m_selectedObject->GetComponent<Physics::ColliderComponent>() == nullptr))
                {
                    addColliderComponent();
                }
                if (ImGui::MenuItem("Light", nullptr, false, m_selectedObject->GetComponent<Scene::LightComponent>() == nullptr))
                {
                    addLightComponent();
                }
                if (ImGui::MenuItem("Post Process", nullptr, false, m_selectedObject->GetComponent<Scene::PostProcessComponent>() == nullptr))
                {
                    addPostProcessComponent();
                }
                if (ImGui::MenuItem("Player Start", nullptr, false, m_selectedObject->GetComponent<Scene::PlayerStartComponent>() == nullptr))
                {
                    addPlayerStartComponent();
                }
                ImGui::EndPopup();
            }

            bool inspectorEditChanged = false;
            if (Scene::MeshRendererComponent* meshRenderer = m_selectedObject->GetComponent<Scene::MeshRendererComponent>())
            {
                if (drawComponentHeader("Mesh Renderer", "MeshRenderer"))
                {
                    GameObjectSnapshot before = GameObjectSnapshot::Capture(*m_selectedObject);
                    GameObjectSnapshot after = before;
                    after.meshRenderer = MeshRendererSnapshot{};
                    applyInspectorSnapshotEdit(before, after, "Removed Mesh Renderer.");
                }
                else
                {
                const std::string meshPath = ToNarrowAscii(meshRenderer->GetMeshPath());
                const std::string materialPath = ToNarrowAscii(meshRenderer->GetMaterialPath());
                const Renderer::Material* assignedMaterial = meshRenderer->GetMaterial().get();
                ImGui::Text("Mesh: %s", meshPath.empty() ? "(runtime)" : meshPath.c_str());
                ImGui::TextUnformatted("Material");
                ImGui::SameLine(110.0f);
                std::string materialButtonText = materialPath;
                if (materialButtonText.empty() && assignedMaterial)
                {
                    materialButtonText = assignedMaterial->GetName().empty() ? "(runtime material)" : assignedMaterial->GetName();
                }
                if (materialButtonText.empty())
                {
                    materialButtonText = "(drop material here)";
                }
                ImGui::Button(materialButtonText.c_str(), ImVec2(-80.0f, 0.0f));
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_MATERIAL"))
                    {
                        const std::wstring droppedMaterialPath = ReadWideAssetPayload(payload);
                        if (!droppedMaterialPath.empty() && ApplyMaterialToSelectedObjects(renderer, droppedMaterialPath))
                        {
                            scene.MarkDirty();
                            ShowNotification("Applied material.");
                        }
                        else
                        {
                            ShowNotification("Failed to load material.");
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear", ImVec2(64.0f, 0.0f)))
                {
                    GameObjectSnapshot before = GameObjectSnapshot::Capture(*m_selectedObject);
                    meshRenderer->SetMaterial(nullptr);
                    meshRenderer->SetMaterialPath({});
                    meshRenderer->SetMaterialGuid({});
                    GameObjectSnapshot after = GameObjectSnapshot::Capture(*m_selectedObject);
                    applyInspectorSnapshotEdit(before, after, "Cleared material.");
                }

                if (Renderer::Material* material = meshRenderer->GetMaterial().get())
                {
                    const std::string materialInstanceName = "MaterialInstance:" + m_selectedObject->GetName();
                    const Scene::ComponentID meshRendererId = meshRenderer->GetID();
                    Math::Vector4 baseColor = material->GetBaseColor();
                    const Math::Vector4 baseColorBefore = baseColor;
                    const TrackedWidgetState baseColorState = ColorEdit4Tracked("Base Color", &baseColor.x);
                    if (baseColorState.changed)
                    {
                        if (Renderer::Material* editableMaterial = EnsureEditableMaterialInstance(*meshRenderer, materialInstanceName))
                        {
                            material = editableMaterial;
                            material->SetBaseColor(baseColor);
                        }
                    }
                    trackPropertyEdit(selectedObjectId, meshRendererId, "MeshRenderer.Material.BaseColor", baseColorBefore, material->GetBaseColor(), baseColorState);

                    float roughness = material->GetRoughness();
                    const float roughnessBefore = roughness;
                    const TrackedWidgetState roughnessState = SliderAndInputFloatTracked("Roughness", &roughness, 0.0f, 1.0f);
                    if (roughnessState.changed)
                    {
                        if (Renderer::Material* editableMaterial = EnsureEditableMaterialInstance(*meshRenderer, materialInstanceName))
                        {
                            material = editableMaterial;
                            material->SetRoughness(roughness);
                        }
                    }
                    trackPropertyEdit(selectedObjectId, meshRendererId, "MeshRenderer.Material.Roughness", roughnessBefore, material->GetRoughness(), roughnessState);

                    float metallic = material->GetMetallic();
                    const float metallicBefore = metallic;
                    const TrackedWidgetState metallicState = SliderAndInputFloatTracked("Metallic", &metallic, 0.0f, 1.0f);
                    if (metallicState.changed)
                    {
                        if (Renderer::Material* editableMaterial = EnsureEditableMaterialInstance(*meshRenderer, materialInstanceName))
                        {
                            material = editableMaterial;
                            material->SetMetallic(metallic);
                        }
                    }
                    trackPropertyEdit(selectedObjectId, meshRendererId, "MeshRenderer.Material.Metallic", metallicBefore, material->GetMetallic(), metallicState);

                    const bool hasMaterialAssetPath = !meshRenderer->GetMaterialPath().empty()
                        && HasExtension(std::filesystem::path(meshRenderer->GetMaterialPath()), { L".mat" });
                    if (hasMaterialAssetPath && ImGui::Button("Save Material"))
                    {
                        Resource::MaterialDesc desc = BuildMaterialDesc(*material, material->GetName());
                        Resource::MaterialLoader::SaveToFile(meshRenderer->GetMaterialPath(), desc);
                        RefreshAssets();
                    }

                    ImGui::InputText("Save As", m_saveMaterialAsBuffer.data(), m_saveMaterialAsBuffer.size());
                    ImGui::SameLine();
                    if (ImGui::Button("Save Material As"))
                    {
                        SaveSelectedMaterialAs(MakeMaterialAssetPath(m_saveMaterialAsBuffer.data()));
                    }
                }
                }
            }

            if (Scene::LightComponent* light = m_selectedObject->GetComponent<Scene::LightComponent>())
            {
                if (drawComponentHeader("Light", "Light"))
                {
                    GameObjectSnapshot before = GameObjectSnapshot::Capture(*m_selectedObject);
                    GameObjectSnapshot after = before;
                    after.light = LightSnapshot{};
                    applyInspectorSnapshotEdit(before, after, "Removed Light.");
                }
                else
                {
                const char* items[] = { "Directional", "Point", "Spot" };
                int currentType = static_cast<int>(light->type);
                if (ImGui::Combo("Type", &currentType, items, IM_ARRAYSIZE(items)))
                {
                    light->type = static_cast<Scene::LightType>(currentType);
                    inspectorEditChanged = true;
                }

                inspectorEditChanged = ImGui::ColorEdit3("Color", &light->color.x) || inspectorEditChanged;
                inspectorEditChanged = DragAndInputFloat("Intensity", &light->intensity, 0.05f, 0.0f, 100.0f) || inspectorEditChanged;
                inspectorEditChanged = DragAndInputFloat("Range", &light->range, 0.05f, 0.01f, 1000.0f) || inspectorEditChanged;
                inspectorEditChanged = DragAndInputFloat3("Direction", &light->direction.x, 0.05f, -1.0f, 1.0f) || inspectorEditChanged;
                inspectorEditChanged = DragAndInputFloat("Inner Cone", &light->innerConeAngle, 0.25f, 0.0f, 179.0f) || inspectorEditChanged;
                inspectorEditChanged = DragAndInputFloat("Outer Cone", &light->outerConeAngle, 0.25f, 0.0f, 179.0f) || inspectorEditChanged;
                light->intensity = std::clamp(light->intensity, 0.0f, 100.0f);
                light->range = std::clamp(light->range, 0.01f, 1000.0f);
                light->direction.x = std::clamp(light->direction.x, -1.0f, 1.0f);
                light->direction.y = std::clamp(light->direction.y, -1.0f, 1.0f);
                light->direction.z = std::clamp(light->direction.z, -1.0f, 1.0f);
                light->innerConeAngle = std::clamp(light->innerConeAngle, 0.0f, 179.0f);
                light->outerConeAngle = std::clamp(light->outerConeAngle, 0.0f, 179.0f);
                light->ClampConeAngles();
                ImGui::Text("Current: %s", ToLightTypeName(light->type));
                }
            }

            if (Physics::ColliderComponent* collider = m_selectedObject->GetComponent<Physics::ColliderComponent>())
            {
                if (drawComponentHeader("Collider", "Collider"))
                {
                    GameObjectSnapshot before = GameObjectSnapshot::Capture(*m_selectedObject);
                    GameObjectSnapshot after = before;
                    after.colliders.clear();
                    applyInspectorSnapshotEdit(before, after, "Removed Collider.");
                }
                else
                {
                const char* items[] = { "AABB", "Sphere" };
                int currentType = collider->type == Physics::ColliderType::Sphere ? 1 : 0;
                if (ImGui::Combo("Collider Type", &currentType, items, IM_ARRAYSIZE(items)))
                {
                    collider->type = currentType == 1 ? Physics::ColliderType::Sphere : Physics::ColliderType::AABB;
                    inspectorEditChanged = true;
                }
                inspectorEditChanged = DragAndInputFloat3("Center", &collider->center.x, 0.05f) || inspectorEditChanged;
                Math::Vector3 colliderRotationDegrees =
                {
                    DirectX::XMConvertToDegrees(collider->rotation.x),
                    DirectX::XMConvertToDegrees(collider->rotation.y),
                    DirectX::XMConvertToDegrees(collider->rotation.z)
                };
                if (DragAndInputFloat3("Rotation (deg)", &colliderRotationDegrees.x, 1.0f))
                {
                    collider->rotation =
                    {
                        DirectX::XMConvertToRadians(colliderRotationDegrees.x),
                        DirectX::XMConvertToRadians(colliderRotationDegrees.y),
                        DirectX::XMConvertToRadians(colliderRotationDegrees.z)
                    };
                    inspectorEditChanged = true;
                }
                inspectorEditChanged = DragAndInputFloat3("Size", &collider->size.x, 0.05f, 0.01f, 1000.0f) || inspectorEditChanged;
                inspectorEditChanged = DragAndInputFloat("Radius", &collider->radius, 0.05f, 0.01f, 1000.0f) || inspectorEditChanged;
                collider->size.x = std::clamp(collider->size.x, 0.01f, 1000.0f);
                collider->size.y = std::clamp(collider->size.y, 0.01f, 1000.0f);
                collider->size.z = std::clamp(collider->size.z, 0.01f, 1000.0f);
                collider->radius = std::clamp(collider->radius, 0.01f, 1000.0f);
                inspectorEditChanged = ImGui::Checkbox("Is Trigger", &collider->isTrigger) || inspectorEditChanged;
                ImGui::Text("Current: %s", ToColliderTypeName(collider->type));
                }
            }

            if (Scene::PostProcessComponent* postProcess = m_selectedObject->GetComponent<Scene::PostProcessComponent>())
            {
                if (drawComponentHeader("Post Process", "PostProcess"))
                {
                    GameObjectSnapshot before = GameObjectSnapshot::Capture(*m_selectedObject);
                    GameObjectSnapshot after = before;
                    after.postProcess = PostProcessSnapshot{};
                    applyInspectorSnapshotEdit(before, after, "Removed Post Process.");
                }
                else
                {
                inspectorEditChanged = ImGui::Checkbox("Enabled", &postProcess->enabled) || inspectorEditChanged;
                if (postProcess->effect != Renderer::PostProcessEffect::None)
                {
                    postProcess->grayscaleEnabled = postProcess->grayscaleEnabled || postProcess->effect == Renderer::PostProcessEffect::Grayscale;
                    postProcess->vignetteEnabled = postProcess->vignetteEnabled || postProcess->effect == Renderer::PostProcessEffect::Vignette;
                    postProcess->toneMappingEnabled = postProcess->toneMappingEnabled || postProcess->effect == Renderer::PostProcessEffect::ToneMapping;
                    postProcess->effect = Renderer::PostProcessEffect::None;
                    inspectorEditChanged = true;
                }
                inspectorEditChanged = ImGui::Checkbox("Grayscale", &postProcess->grayscaleEnabled) || inspectorEditChanged;
                if (postProcess->grayscaleEnabled)
                {
                    inspectorEditChanged = SliderAndInputFloat("Grayscale Intensity", &postProcess->grayscaleIntensity, 0.0f, 1.0f) || inspectorEditChanged;
                }
                inspectorEditChanged = ImGui::Checkbox("Vignette", &postProcess->vignetteEnabled) || inspectorEditChanged;
                if (postProcess->vignetteEnabled)
                {
                    inspectorEditChanged = SliderAndInputFloat("Vignette Strength", &postProcess->vignetteStrength, 0.0f, 2.0f) || inspectorEditChanged;
                    inspectorEditChanged = SliderAndInputFloat("Vignette Radius", &postProcess->vignetteRadius, 0.0f, 2.0f) || inspectorEditChanged;
                    inspectorEditChanged = SliderAndInputFloat("Vignette Softness", &postProcess->vignetteSoftness, 0.01f, 2.0f) || inspectorEditChanged;
                }
                inspectorEditChanged = ImGui::Checkbox("Tone Mapping", &postProcess->toneMappingEnabled) || inspectorEditChanged;
                if (postProcess->toneMappingEnabled)
                {
                    inspectorEditChanged = SliderAndInputFloat("Exposure", &postProcess->exposure, 0.1f, 5.0f) || inspectorEditChanged;
                    inspectorEditChanged = SliderAndInputFloat("Gamma", &postProcess->gamma, 0.1f, 4.0f) || inspectorEditChanged;
                }
                inspectorEditChanged = ImGui::Checkbox("Color Adjust", &postProcess->colorAdjustEnabled) || inspectorEditChanged;
                if (postProcess->colorAdjustEnabled)
                {
                    inspectorEditChanged = SliderAndInputFloat("Brightness", &postProcess->brightness, -1.0f, 1.0f) || inspectorEditChanged;
                    inspectorEditChanged = SliderAndInputFloat("Contrast", &postProcess->contrast, 0.0f, 3.0f) || inspectorEditChanged;
                    inspectorEditChanged = SliderAndInputFloat("Saturation", &postProcess->saturation, 0.0f, 3.0f) || inspectorEditChanged;
                }
                ImGui::Text("Current: %s", ToPostProcessStackSummary(*postProcess));
                }
            }

            if (Scene::PlayerStartComponent* playerStart = m_selectedObject->GetComponent<Scene::PlayerStartComponent>())
            {
                if (drawComponentHeader("Player Start", "PlayerStart"))
                {
                    GameObjectSnapshot before = GameObjectSnapshot::Capture(*m_selectedObject);
                    GameObjectSnapshot after = before;
                    after.playerStart = PlayerStartSnapshot{};
                    applyInspectorSnapshotEdit(before, after, "Removed Player Start.");
                }
                else
                {
                inspectorEditChanged = DragAndInputFloat("Player Height", &playerStart->playerHeight, 0.05f, 0.0f, 10.0f) || inspectorEditChanged;
                inspectorEditChanged = DragAndInputFloat("Move Speed", &playerStart->moveSpeed, 0.05f, 0.0f, 100.0f) || inspectorEditChanged;
                inspectorEditChanged = DragAndInputFloat("Fast Multiplier", &playerStart->fastMoveMultiplier, 0.05f, 1.0f, 20.0f) || inspectorEditChanged;
                inspectorEditChanged = DragAndInputFloat("Mouse Sensitivity", &playerStart->mouseSensitivity, 0.0005f, 0.0001f, 0.1f) || inspectorEditChanged;
                playerStart->playerHeight = std::clamp(playerStart->playerHeight, 0.0f, 10.0f);
                playerStart->moveSpeed = std::clamp(playerStart->moveSpeed, 0.0f, 100.0f);
                playerStart->fastMoveMultiplier = std::clamp(playerStart->fastMoveMultiplier, 1.0f, 20.0f);
                playerStart->mouseSensitivity = std::clamp(playerStart->mouseSensitivity, 0.0001f, 0.1f);
                ImGui::TextUnformatted("Play Mode starts the first-person camera here.");
                }
            }

            if (inspectorEditChanged && !m_inspectorEditObject)
            {
                m_inspectorEditObject = m_selectedObject;
                m_inspectorEditStart = GameObjectSnapshot::Capture(*m_selectedObject);
                scene.MarkDirty();
            }
            if (m_inspectorEditObject && !ImGui::IsAnyItemActive())
            {
                const GameObjectSnapshot after = GameObjectSnapshot::Capture(*m_inspectorEditObject);
                if (GameObjectSnapshot::IsDifferent(m_inspectorEditStart, after))
                {
                    m_commandManager->ExecuteCommand(std::make_unique<InspectorEditCommand>(*m_inspectorEditObject, m_inspectorEditStart, after));
                }
                m_inspectorEditObject = nullptr;
            }
        }
        else
        {
            ImGui::TextUnformatted("No object selected.");
        }
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(8.0f, sideTop + viewportToolbarHeight + 8.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(260.0f, 150.0f), ImGuiCond_Always);
        ImGui::Begin("Viewport Stats", nullptr, fixedPanelFlags | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);
        ImGui::Text("FPS: %.1f", stats.fps);
        ImGui::Text("Delta Time: %.4f", stats.deltaTime);
        ImGui::Text("Objects: %u", stats.objectCount);
        ImGui::Text("Draw Calls: %u", stats.drawCallCount);
        ImGui::Text("Triangles: %u", stats.triangleCount);
        ImGui::Text("Camera: %.2f, %.2f, %.2f", stats.cameraPosition.x, stats.cameraPosition.y, stats.cameraPosition.z);
        ImGui::Text("Mode: %s", m_playMode ? "Play" : "Editor");
        ImGui::End();

        if (m_showAssetBrowser)
        {
            ImGui::SetNextWindowPos(ImVec2(0.0f, io.DisplaySize.y - bottomPanelHeight), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, bottomPanelHeight), ImGuiCond_Always);
            RenderAssetBrowser(scene, renderer);
        }
        RenderModelEditor(scene, renderer);
        RenderMaterialEditor(scene, renderer);
        m_staticMeshEditor.OnImGuiRender(renderer);

        drawSplitter("##RightPanelSplitter",
            ImVec2(rightPanelX - splitterThickness * 0.5f, sideTop),
            ImVec2(splitterThickness, sideHeight),
            ImGuiMouseCursor_ResizeEW,
            [&](const ImVec2& delta)
            {
                m_rightPanelWidth -= delta.x;
            });

        drawSplitter("##DetailsSplitter",
            ImVec2(rightPanelX, inspectorY - splitterThickness * 0.5f),
            ImVec2(rightPanelWidth, splitterThickness),
            ImGuiMouseCursor_ResizeNS,
            [&](const ImVec2& delta)
            {
                if (sideHeight > 1.0f)
                {
                    m_hierarchyPanelRatio += delta.y / sideHeight;
                }
            });

        if (m_showAssetBrowser)
        {
            drawSplitter("##ContentBrowserSplitter",
                ImVec2(0.0f, io.DisplaySize.y - bottomPanelHeight - splitterThickness * 0.5f),
                ImVec2(io.DisplaySize.x, splitterThickness),
                ImGuiMouseCursor_ResizeNS,
                [&](const ImVec2& delta)
                {
                    m_bottomPanelHeight -= delta.y;
                });
        }

        ImGui::SetNextWindowPos(ImVec2(0.0f, io.DisplaySize.y - statusBarHeight), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, statusBarHeight), ImGuiCond_Always);
        if (ImGui::Begin("Status Bar", nullptr, fixedPanelFlags | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar))
        {
            ImGui::Text("FPS %.1f", stats.fps);
            ImGui::SameLine(120.0f);
            ImGui::Text("Objects %u", stats.objectCount);
            ImGui::SameLine(240.0f);
            ImGui::Text("Draw %u", stats.drawCallCount);
            ImGui::SameLine(360.0f);
        ImGui::TextUnformatted(m_currentSceneAsset.empty() ? "Unsaved Scene" : ToNarrowAscii(m_currentSceneAsset).c_str());
        if (scene.IsDirty())
        {
            ImGui::SameLine();
            ImGui::TextUnformatted("*");
        }
        if (m_assetDatabase.IsDirty())
        {
            ImGui::SameLine();
            ImGui::TextUnformatted("Assets*");
        }
    }
    ImGui::End();

        if (m_notificationTimer > 0.0f && !m_notificationText.empty())
        {
            const float fade = std::clamp(m_notificationTimer / 0.25f, 0.0f, 1.0f);
            const float notificationBottom = m_showAssetBrowser
                ? io.DisplaySize.y - bottomPanelHeight - 16.0f
                : io.DisplaySize.y - statusBarHeight - 16.0f;

            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, notificationBottom), ImGuiCond_Always, ImVec2(0.5f, 1.0f));
            ImGui::SetNextWindowBgAlpha(0.92f * fade);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, fade);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 10.0f));
            if (ImGui::Begin("##EditorNotification", nullptr,
                ImGuiWindowFlags_NoTitleBar
                | ImGuiWindowFlags_NoResize
                | ImGuiWindowFlags_NoMove
                | ImGuiWindowFlags_NoScrollbar
                | ImGuiWindowFlags_AlwaysAutoResize
                | ImGuiWindowFlags_NoSavedSettings
                | ImGuiWindowFlags_NoInputs
                | ImGuiWindowFlags_NoFocusOnAppearing))
            {
                ImGui::TextUnformatted(m_notificationText.c_str());
            }
            ImGui::End();
            ImGui::PopStyleVar(2);
        }

        RenderSavePromptDialog(scene, renderer);
        (void)renderer;
    }

    void EditorLayer::ApplyRendererSettings(Renderer::Renderer& renderer, const Scene::Scene& scene) const
    {
        Renderer::PostProcessSettings settings;
        for (const std::unique_ptr<Scene::GameObject>& object : scene.GetGameObjects())
        {
            const Scene::PostProcessComponent* postProcess = object->GetComponent<Scene::PostProcessComponent>();
            if (postProcess && postProcess->enabled && postProcess->ToSettings().HasAnyEffectEnabled())
            {
                settings = postProcess->ToSettings();
                break;
            }
        }
        renderer.SetPostProcessSettings(settings);

        Renderer::SelectionOutlineSettings outlineSettings;
        outlineSettings.enabled = m_screenSelectionOutline;
        outlineSettings.debugLineEnabled = m_debugSelectionOutline;
        outlineSettings.color =
        {
            m_selectionOutlineColor[0],
            m_selectionOutlineColor[1],
            m_selectionOutlineColor[2],
            m_selectionOutlineColor[3]
        };
        outlineSettings.width = m_selectionOutlineWidth;
        outlineSettings.opacity = m_selectionOutlineOpacity;
        renderer.SetSelectionOutlineSettings(outlineSettings);
    }

    void EditorLayer::ValidateSelection(const Scene::Scene& scene)
    {
        if (m_renamingObject)
        {
            bool renameTargetExists = false;
            for (const std::unique_ptr<Scene::GameObject>& object : scene.GetGameObjects())
            {
                if (object.get() == m_renamingObject)
                {
                    renameTargetExists = true;
                    break;
                }
            }

            if (!renameTargetExists)
            {
                m_renamingObject = nullptr;
                m_focusRenameInput = false;
            }
        }

        auto objectExists = [&](const Scene::GameObject* candidate)
        {
            return std::any_of(scene.GetGameObjects().begin(), scene.GetGameObjects().end(), [&](const std::unique_ptr<Scene::GameObject>& object)
            {
                return object.get() == candidate;
            });
        };

        m_selectedObjects.erase(std::remove_if(m_selectedObjects.begin(), m_selectedObjects.end(), [&](const Scene::GameObject* object)
        {
            return !objectExists(object);
        }), m_selectedObjects.end());

        if (m_selectedObject && !objectExists(m_selectedObject))
        {
            m_selectedObject = nullptr;
        }

        if (m_outlinerSelectionAnchor && !objectExists(m_outlinerSelectionAnchor))
        {
            m_outlinerSelectionAnchor = nullptr;
        }

        if (!m_selectedObject && !m_selectedObjects.empty())
        {
            m_selectedObject = m_selectedObjects.front();
        }

        if (m_selectedObject && std::find(m_selectedObjects.begin(), m_selectedObjects.end(), m_selectedObject) == m_selectedObjects.end())
        {
            m_selectedObjects.push_back(m_selectedObject);
        }
    }

    bool EditorLayer::RaycastSceneMeshes(const Scene::Scene& scene, const Physics::Ray& ray, Physics::RaycastHit& outHit) const
    {
        float closestDistance = FLT_MAX;
        Scene::GameObject* closestObject = nullptr;
        Math::Vector3 closestPoint = {};

        for (const std::unique_ptr<Scene::GameObject>& object : scene.GetGameObjects())
        {
            if (!object)
            {
                continue;
            }

            const Scene::MeshRendererComponent* meshRenderer = object->GetComponent<Scene::MeshRendererComponent>();
            if (!meshRenderer || !meshRenderer->GetMesh() || !meshRenderer->GetMesh()->GetBounds().IsValid())
            {
                continue;
            }

            const Renderer::BoundingBox& bounds = meshRenderer->GetMesh()->GetBounds();
            const Math::Matrix world = object->GetTransform().GetWorldMatrix();
            const std::array<Math::Vector3, 8> localPoints =
            {
                Math::Vector3{ bounds.min.x, bounds.min.y, bounds.min.z },
                Math::Vector3{ bounds.max.x, bounds.min.y, bounds.min.z },
                Math::Vector3{ bounds.max.x, bounds.max.y, bounds.min.z },
                Math::Vector3{ bounds.min.x, bounds.max.y, bounds.min.z },
                Math::Vector3{ bounds.min.x, bounds.min.y, bounds.max.z },
                Math::Vector3{ bounds.max.x, bounds.min.y, bounds.max.z },
                Math::Vector3{ bounds.max.x, bounds.max.y, bounds.max.z },
                Math::Vector3{ bounds.min.x, bounds.max.y, bounds.max.z }
            };

            Math::Vector3 worldMin = { FLT_MAX, FLT_MAX, FLT_MAX };
            Math::Vector3 worldMax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
            for (const Math::Vector3& localPoint : localPoints)
            {
                const Math::Vector3 point = TransformPoint(world, localPoint);
                worldMin.x = std::min(worldMin.x, point.x);
                worldMin.y = std::min(worldMin.y, point.y);
                worldMin.z = std::min(worldMin.z, point.z);
                worldMax.x = std::max(worldMax.x, point.x);
                worldMax.y = std::max(worldMax.y, point.y);
                worldMax.z = std::max(worldMax.z, point.z);
            }

            float distance = 0.0f;
            if (!RaycastWorldAabb(ray, worldMin, worldMax, distance) || distance >= closestDistance)
            {
                continue;
            }

            closestDistance = distance;
            closestObject = object.get();
            DirectX::XMVECTOR point = DirectX::XMVectorAdd(
                DirectX::XMLoadFloat3(&ray.origin),
                DirectX::XMVectorScale(DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&ray.direction)), distance));
            DirectX::XMStoreFloat3(&closestPoint, point);
        }

        if (!closestObject)
        {
            return false;
        }

        outHit.object = closestObject;
        outHit.point = closestPoint;
        outHit.distance = closestDistance;
        return true;
    }

    void EditorLayer::SetSingleSelectedObject(Scene::GameObject* object)
    {
        m_selectedObject = object;
        m_selectedObjects.clear();
        m_hoveredGizmoAxis = GizmoAxis::None;
        m_activeGizmoAxis = GizmoAxis::None;
        m_hasActivePropertyEdit = false;
        m_activePropertyObjectId = Scene::InvalidObjectID;
        m_activePropertyComponentId = Scene::InvalidComponentID;
        m_activePropertyPath.clear();
        if (object)
        {
            m_selectedObjects.push_back(object);
        }
    }

    void EditorLayer::ToggleSelectedObject(Scene::GameObject* object)
    {
        if (!object)
        {
            return;
        }

        const auto found = std::find(m_selectedObjects.begin(), m_selectedObjects.end(), object);
        if (found != m_selectedObjects.end())
        {
            m_selectedObjects.erase(found);
            if (m_selectedObject == object)
            {
                m_selectedObject = m_selectedObjects.empty() ? nullptr : m_selectedObjects.back();
            }
            return;
        }

        m_selectedObjects.push_back(object);
        m_selectedObject = object;
    }

    bool EditorLayer::IsObjectSelected(const Scene::GameObject* object) const
    {
        return std::find(m_selectedObjects.begin(), m_selectedObjects.end(), object) != m_selectedObjects.end();
    }

    void EditorLayer::SetSingleSelectedAsset(const std::wstring& asset)
    {
        m_selectedAsset = asset;
        m_selectedAssets.clear();
        if (!asset.empty())
        {
            m_selectedAssets.push_back(asset);
        }
    }

    void EditorLayer::ToggleSelectedAsset(const std::wstring& asset)
    {
        if (asset.empty())
        {
            return;
        }

        const auto found = std::find(m_selectedAssets.begin(), m_selectedAssets.end(), asset);
        if (found != m_selectedAssets.end())
        {
            m_selectedAssets.erase(found);
            if (m_selectedAsset == asset)
            {
                m_selectedAsset = m_selectedAssets.empty() ? std::wstring() : m_selectedAssets.back();
            }
            return;
        }

        m_selectedAssets.push_back(asset);
        m_selectedAsset = asset;
    }

    bool EditorLayer::IsAssetSelected(const std::wstring& asset) const
    {
        return std::find(m_selectedAssets.begin(), m_selectedAssets.end(), asset) != m_selectedAssets.end();
    }

    bool EditorLayer::ApplyMaterialToSelectedObjects(Renderer::Renderer& renderer, const std::wstring& materialPath)
    {
        std::vector<Scene::GameObject*> targets = m_selectedObjects;
        if (targets.empty() && m_selectedObject)
        {
            targets.push_back(m_selectedObject);
        }
        if (targets.empty())
        {
            return false;
        }

        Resource::ResourceManager& resources = renderer.GetResourceManager();
        std::shared_ptr<Renderer::Material> sourceMaterial = resources.LoadMaterial(materialPath);
        if (!sourceMaterial)
        {
            return false;
        }
        const std::string materialGuid = m_assetDatabase.GetGuidFromPath(materialPath);

        for (Scene::GameObject* object : targets)
        {
            if (!object)
            {
                continue;
            }

            Scene::MeshRendererComponent* meshRenderer = object->GetComponent<Scene::MeshRendererComponent>();
            if (!meshRenderer)
            {
                meshRenderer = &object->AddComponent<Scene::MeshRendererComponent>();
                meshRenderer->SetMesh(resources.CreateCubeMesh("builtin:mesh:cube"));
                meshRenderer->SetMeshPath(L"builtin:mesh:cube");
            }

            std::shared_ptr<Renderer::Material> material = CloneMaterialInstance(sourceMaterial, "MaterialInstance:" + object->GetName());
            meshRenderer->SetMaterial(material ? material : sourceMaterial);
            meshRenderer->SetMaterialPath(materialPath);
            meshRenderer->SetMaterialGuid(materialGuid);
        }

        return true;
    }

    bool EditorLayer::DeleteSelectedObjects(Scene::Scene& scene)
    {
        EnsureCommandManager(scene);
        std::vector<Scene::GameObject*> objectsToDelete = m_selectedObjects;
        if (objectsToDelete.empty() && m_selectedObject)
        {
            objectsToDelete.push_back(m_selectedObject);
        }
        if (objectsToDelete.empty())
        {
            return false;
        }

        std::sort(objectsToDelete.begin(), objectsToDelete.end());
        objectsToDelete.erase(std::unique(objectsToDelete.begin(), objectsToDelete.end()), objectsToDelete.end());

        SetSingleSelectedObject(nullptr);
        m_renamingObject = nullptr;
        m_focusRenameInput = false;
        m_activeGizmoAxis = GizmoAxis::None;

        if (m_commandManager)
        {
            m_commandManager->ExecuteCommand(std::make_unique<DeleteGameObjectCommand>(scene, objectsToDelete));
        }

        Core::Log::Info("Deleted selected object(s).");
        return true;
    }

    std::string EditorLayer::MakeUniqueOutlinerFolderName(const Scene::Scene& scene, const std::string& baseName) const
    {
        const std::string base = Trim(baseName).empty() ? "New Folder" : Trim(baseName);
        if (!scene.HasOutlinerFolder(base))
        {
            return base;
        }

        for (int index = 1; index < 10000; ++index)
        {
            const std::string candidate = base + " " + std::to_string(index);
            if (!scene.HasOutlinerFolder(candidate))
            {
                return candidate;
            }
        }

        return base + " 10000";
    }

    void EditorLayer::CreateOutlinerFolder(Scene::Scene& scene)
    {
        const std::string folder = MakeUniqueOutlinerFolderName(scene, "New Folder");
        scene.AddOutlinerFolder(folder);
        scene.MarkDirty();
        m_selectedOutlinerFolder = folder;
        SetSingleSelectedObject(nullptr);
        BeginRenameSelectedFolder();
    }

    void EditorLayer::BeginRenameSelectedFolder()
    {
        if (m_selectedOutlinerFolder.empty())
        {
            return;
        }

        m_renamingOutlinerFolder = m_selectedOutlinerFolder;
        m_folderRenameBuffer.fill('\0');
        const std::size_t copyCount = std::min(m_selectedOutlinerFolder.size(), m_folderRenameBuffer.size() - 1);
        std::memcpy(m_folderRenameBuffer.data(), m_selectedOutlinerFolder.data(), copyCount);
        m_focusFolderRenameInput = true;
    }

    void EditorLayer::CommitFolderRename(Scene::Scene& scene)
    {
        if (m_renamingOutlinerFolder.empty())
        {
            return;
        }

        const std::string trimmedName = Trim(m_folderRenameBuffer.data());
        if (!trimmedName.empty() && trimmedName != m_renamingOutlinerFolder && !scene.HasOutlinerFolder(trimmedName))
        {
            const std::string oldFolder = m_renamingOutlinerFolder;
            scene.RenameOutlinerFolder(oldFolder, trimmedName);
            scene.MarkDirty();
            m_selectedOutlinerFolder = trimmedName;
        }

        m_renamingOutlinerFolder.clear();
        m_focusFolderRenameInput = false;
    }

    void EditorLayer::MoveObjectsToFolder(Scene::Scene& scene, const std::vector<Scene::GameObject*>& objects, const std::string& folder)
    {
        if (!folder.empty() && !scene.HasOutlinerFolder(folder))
        {
            scene.AddOutlinerFolder(folder);
        }

        std::vector<Scene::GameObject*> targets;
        targets.reserve(objects.size());
        for (Scene::GameObject* object : objects)
        {
            if (!object || object->GetOutlinerFolder() == folder)
            {
                continue;
            }

            if (std::find(targets.begin(), targets.end(), object) == targets.end())
            {
                targets.push_back(object);
            }
        }

        if (targets.empty())
        {
            return;
        }

        EnsureCommandManager(scene);
        if (m_commandManager)
        {
            m_commandManager->ExecuteCommand(std::make_unique<SetObjectFolderCommand>(targets, folder));
        }
        else
        {
            for (Scene::GameObject* object : targets)
            {
                object->SetOutlinerFolder(folder);
            }
            scene.MarkDirty();
        }
    }

    void EditorLayer::BeginRenameSelected()
    {
        if (!m_selectedObject)
        {
            return;
        }

        m_renamingObject = m_selectedObject;
        m_renameBuffer.fill('\0');
        const std::string& name = m_selectedObject->GetName();
        const std::size_t copyCount = std::min(name.size(), m_renameBuffer.size() - 1);
        std::memcpy(m_renameBuffer.data(), name.data(), copyCount);
        m_focusRenameInput = true;
    }

    void EditorLayer::CommitRename()
    {
        if (!m_renamingObject)
        {
            return;
        }

        std::string newName = m_renameBuffer.data();
        const auto first = std::find_if_not(newName.begin(), newName.end(), [](unsigned char character)
        {
            return std::isspace(character) != 0;
        });
        const auto last = std::find_if_not(newName.rbegin(), newName.rend(), [](unsigned char character)
        {
            return std::isspace(character) != 0;
        }).base();

        if (first < last)
        {
            const std::string trimmedName(first, last);
            if (trimmedName != m_renamingObject->GetName())
            {
                if (m_commandManager)
                {
                    m_commandManager->ExecuteCommand(std::make_unique<RenameGameObjectCommand>(*m_renamingObject, m_renamingObject->GetName(), trimmedName));
                }
                else
                {
                    m_renamingObject->SetName(trimmedName);
                }
            }
        }

        m_renamingObject = nullptr;
        m_focusRenameInput = false;
    }

    void EditorLayer::BeginRenameSelectedAsset()
    {
        if (m_selectedAsset.empty() || !std::filesystem::exists(std::filesystem::path(m_selectedAsset)))
        {
            return;
        }

        m_renamingAsset = m_selectedAsset;
        m_assetRenameBuffer.fill('\0');
        const std::string stem = ToNarrowAscii(std::filesystem::path(m_selectedAsset).stem().wstring());
        const std::size_t copyCount = std::min(stem.size(), m_assetRenameBuffer.size() - 1);
        std::memcpy(m_assetRenameBuffer.data(), stem.data(), copyCount);
        m_focusAssetRenameInput = true;
    }

    void EditorLayer::CommitAssetRename(Scene::Scene& scene, Renderer::Renderer& renderer)
    {
        if (m_renamingAsset.empty())
        {
            return;
        }

        const std::filesystem::path oldPath(m_renamingAsset);
        std::string newStem = SanitizeAssetName(m_assetRenameBuffer.data(), ToNarrowAscii(oldPath.stem().wstring()).c_str());
        if (newStem.empty())
        {
            m_renamingAsset.clear();
            m_focusAssetRenameInput = false;
            return;
        }

        const std::filesystem::path newPath = oldPath.parent_path() / (newStem + ToNarrowAscii(oldPath.extension().wstring()));
        const std::wstring newAssetPath = newPath.generic_wstring();
        if (newAssetPath == m_renamingAsset)
        {
            m_renamingAsset.clear();
            m_focusAssetRenameInput = false;
            return;
        }

        if (std::filesystem::exists(newPath))
        {
            Core::Log::Error("Cannot rename asset because target already exists: " + ToNarrowAscii(newAssetPath));
            m_renamingAsset.clear();
            m_focusAssetRenameInput = false;
            return;
        }

        if (!m_assetDatabase.RenameAsset(oldPath, newPath))
        {
            Core::Log::Error("Failed to rename asset: " + ToNarrowAscii(m_renamingAsset));
            m_renamingAsset.clear();
            m_focusAssetRenameInput = false;
            return;
        }

        if (HasExtension(newPath, { L".mat" }))
        {
            Resource::MaterialDesc desc;
            if (Resource::MaterialLoader::LoadFromFile(newAssetPath, desc))
            {
                desc.name = newStem;
                Resource::MaterialLoader::SaveToFile(newAssetPath, desc);
            }
        }

        for (const std::unique_ptr<Scene::GameObject>& object : scene.GetGameObjects())
        {
            Scene::MeshRendererComponent* meshRenderer = object->GetComponent<Scene::MeshRendererComponent>();
            if (!meshRenderer)
            {
                continue;
            }

            const bool materialPathWasRenamed = meshRenderer->GetMaterialPath() == m_renamingAsset;
            if (meshRenderer->GetMeshPath() == m_renamingAsset)
            {
                meshRenderer->SetMeshPath(newAssetPath);
            }
            if (materialPathWasRenamed)
            {
                meshRenderer->SetMaterialPath(newAssetPath);
            }

            Renderer::Material* material = meshRenderer->GetMaterial().get();
            if (material)
            {
                if (materialPathWasRenamed)
                {
                    material->SetName(newStem);
                }
                if (material->GetBaseColorTexturePath() == m_renamingAsset)
                {
                    material->SetBaseColorTexturePath(newAssetPath);
                }
                if (material->GetRoughnessTexturePath() == m_renamingAsset)
                {
                    material->SetRoughnessTexturePath(newAssetPath);
                }
                if (material->GetMetallicTexturePath() == m_renamingAsset)
                {
                    material->SetMetallicTexturePath(newAssetPath);
                }
                if (material->GetNormalTexturePath() == m_renamingAsset)
                {
                    material->SetNormalTexturePath(newAssetPath);
                }
            }
        }

        if (m_currentSceneAsset == m_renamingAsset)
        {
            m_currentSceneAsset = newAssetPath;
        }
        if (m_selectedSceneAsset == m_renamingAsset)
        {
            m_selectedSceneAsset = newAssetPath;
        }
        if (m_selectedMaterialAsset == m_renamingAsset)
        {
            renderer.GetResourceManager().ForgetMaterial(m_renamingAsset);
            m_selectedMaterialAsset = newAssetPath;
        }

        SetSingleSelectedAsset(newAssetPath);
        m_renamingAsset.clear();
        m_focusAssetRenameInput = false;
        RefreshAssets();
        Core::Log::Info("Renamed asset: " + ToNarrowAscii(newAssetPath));
    }

    bool EditorLayer::CreateMaterialAsset(Renderer::Renderer& renderer)
    {
        const std::string materialName = SanitizeAssetName(m_newMaterialNameBuffer.data(), "NewMaterial");
        const std::wstring materialPath = MakeAssetPathInFolder(m_selectedAssetFolder, materialName, ".mat", "Material");
        std::filesystem::create_directories(std::filesystem::path(materialPath).parent_path());

        Resource::ResourceManager& resources = renderer.GetResourceManager();
        std::shared_ptr<Renderer::Material> material = resources.CreateMaterial("editor:material:" + materialName, { 1.0f, 1.0f, 1.0f, 1.0f });
        if (!material)
        {
            Core::Log::Error("Failed to create material asset.");
            return false;
        }

        material->SetName(materialName);
        Resource::MaterialDesc desc = BuildMaterialDesc(*material, materialName);
        if (!Resource::MaterialLoader::SaveToFile(materialPath, desc))
        {
            return false;
        }

        if (!m_selectedObjects.empty() || m_selectedObject)
        {
            ApplyMaterialToSelectedObjects(renderer, materialPath);
        }

        SetSingleSelectedAsset(materialPath);
        m_selectedMaterialAsset = materialPath;
        RefreshAssets();
        Core::Log::Info("Created material asset: " + ToNarrowAscii(materialPath));
        return true;
    }

    bool EditorLayer::CreateSceneAsset(Scene::Scene& scene)
    {
        const std::string sceneName = SanitizeAssetName(m_newSceneNameBuffer.data(), "NewScene");
        const std::wstring scenePath = MakeAssetPathInFolder(m_selectedAssetFolder, sceneName, ".scene", "Scene");
        std::filesystem::create_directories(std::filesystem::path(scenePath).parent_path());

        scene.Clear();
        scene.EnsureDefaultCameraAndLight();
        scene.MarkDirty();
        if (m_commandManager)
        {
            m_commandManager->Clear();
        }
        SetSingleSelectedObject(nullptr);
        m_selectedObjects.clear();
        m_currentSceneAsset = scenePath;
        m_selectedSceneAsset = scenePath;
        scene.SetFilePath(scenePath);
        SetSingleSelectedAsset(scenePath);

        if (!Scene::SceneSerializer::SaveScene(scenePath, scene))
        {
            Core::Log::Error("Failed to create scene asset: " + ToNarrowAscii(scenePath));
            return false;
        }

        RefreshAssets();
        Core::Log::Info("Created scene asset: " + ToNarrowAscii(scenePath));
        scene.ClearDirty();
        if (m_commandManager)
        {
            m_commandManager->MarkSaved();
        }
        return true;
    }

    bool EditorLayer::RequestNewScene(Scene::Scene& scene)
    {
        if (!CheckSaveBeforeDestructiveAction(scene, PendingEditorAction::NewScene))
        {
            return false;
        }

        ExecuteNewScene(scene);
        return true;
    }

    bool EditorLayer::RequestOpenScene(Scene::Scene& scene, Renderer::Renderer& renderer, const std::wstring& path)
    {
        if (path.empty())
        {
            return false;
        }

        if (!CheckSaveBeforeDestructiveAction(scene, PendingEditorAction::OpenScene, path))
        {
            return false;
        }

        return ExecuteOpenScene(scene, renderer, path);
    }

    bool EditorLayer::CheckSaveBeforeDestructiveAction(Scene::Scene& scene, PendingEditorAction action, const std::wstring& path)
    {
        if (!scene.IsDirty())
        {
            return true;
        }

        m_pendingAction = action;
        m_pendingOpenScenePath = path;
        m_openSavePrompt = true;
        return false;
    }

    void EditorLayer::ExecutePendingAction(Scene::Scene& scene, Renderer::Renderer& renderer, SavePromptResult result)
    {
        if (m_pendingAction == PendingEditorAction::None)
        {
            return;
        }

        if (result == SavePromptResult::Cancel)
        {
            ShowNotification("Action canceled.");
            m_pendingAction = PendingEditorAction::None;
            m_pendingOpenScenePath.clear();
            return;
        }

        if (result == SavePromptResult::Save && !SaveCurrentScene(scene))
        {
            ShowNotification("Save failed. Action canceled.");
            return;
        }

        const PendingEditorAction action = m_pendingAction;
        const std::wstring openPath = m_pendingOpenScenePath;
        m_pendingAction = PendingEditorAction::None;
        m_pendingOpenScenePath.clear();

        switch (action)
        {
        case PendingEditorAction::NewScene:
            ExecuteNewScene(scene);
            break;
        case PendingEditorAction::OpenScene:
            ExecuteOpenScene(scene, renderer, openPath);
            break;
        case PendingEditorAction::Exit:
            m_exitRequested = true;
            break;
        case PendingEditorAction::None:
        default:
            break;
        }
    }

    void EditorLayer::ExecuteNewScene(Scene::Scene& scene)
    {
        scene.Clear();
        scene.EnsureDefaultCameraAndLight();
        scene.SetFilePath({});
        scene.ClearDirty();
        if (m_commandManager)
        {
            m_commandManager->Clear();
            m_commandManager->MarkSaved();
        }
        m_currentSceneAsset.clear();
        m_selectedSceneAsset.clear();
        SetSingleSelectedObject(nullptr);
        ShowNotification("New scene created.");
        Core::Log::Info("Created untitled scene.");
    }

    bool EditorLayer::ExecuteOpenScene(Scene::Scene& scene, Renderer::Renderer& renderer, const std::wstring& path)
    {
        Resource::ResourceManager& resources = renderer.GetResourceManager();
        if (!Scene::SceneSerializer::LoadScene(path, scene, resources))
        {
            Core::Log::Error("Failed to open scene: " + ToNarrowAscii(path));
            ShowNotification("Failed to open scene.");
            return false;
        }

        scene.SetFilePath(path);
        scene.ClearDirty();
        if (m_commandManager)
        {
            m_commandManager->Clear();
            m_commandManager->MarkSaved();
        }
        SetSingleSelectedObject(scene.GetGameObjects().empty() ? nullptr : scene.GetGameObjects().front().get());
        m_currentSceneAsset = path;
        m_selectedSceneAsset = path;
        SetSingleSelectedAsset(path);
        ShowNotification("Scene loaded: " + ToNarrowAscii(path));
        Core::Log::Info("Opened scene: " + ToNarrowAscii(path));
        return true;
    }

    void EditorLayer::RenderSavePromptDialog(Scene::Scene& scene, Renderer::Renderer& renderer)
    {
        if (m_openSavePrompt)
        {
            ImGui::OpenPopup("Unsaved Changes");
            m_openSavePrompt = false;
        }

        if (ImGui::BeginPopupModal("Unsaved Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted("The current scene has unsaved changes.");
            ImGui::Spacing();
            ImGui::Text("Save changes to %s?", scene.GetDisplayName().c_str());
            ImGui::Spacing();

            if (ImGui::Button("Save", ImVec2(96.0f, 0.0f)))
            {
                ImGui::CloseCurrentPopup();
                ExecutePendingAction(scene, renderer, SavePromptResult::Save);
            }
            ImGui::SameLine();
            if (ImGui::Button("Don't Save", ImVec2(112.0f, 0.0f)))
            {
                ImGui::CloseCurrentPopup();
                ExecutePendingAction(scene, renderer, SavePromptResult::DontSave);
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(96.0f, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                ImGui::CloseCurrentPopup();
                ExecutePendingAction(scene, renderer, SavePromptResult::Cancel);
            }
            ImGui::EndPopup();
        }
    }

    bool EditorLayer::SaveCurrentScene(Scene::Scene& scene)
    {
        std::wstring savePath = m_currentSceneAsset;
        if (savePath.empty() && !scene.GetFilePath().empty())
        {
            savePath = scene.GetFilePath().generic_wstring();
        }
        if (savePath.empty())
        {
            savePath = MakeAssetPathInFolder(m_selectedAssetFolder, m_newSceneNameBuffer.data(), ".scene", "Scene");
        }
        return SaveCurrentSceneAs(scene, savePath);
    }

    bool EditorLayer::ImportModelAsset()
    {
        wchar_t fileName[MAX_PATH] = {};
        OPENFILENAMEW openFileName = {};
        openFileName.lStructSize = sizeof(openFileName);
        openFileName.lpstrFile = fileName;
        openFileName.nMaxFile = MAX_PATH;
        openFileName.lpstrFilter = L"Wavefront OBJ (*.obj)\0*.obj\0All Files (*.*)\0*.*\0";
        openFileName.nFilterIndex = 1;
        openFileName.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

        if (!GetOpenFileNameW(&openFileName))
        {
            return false;
        }

        const std::filesystem::path sourcePath(fileName);
        if (!HasExtension(sourcePath, { L".obj" }))
        {
            Core::Log::Warning("Only OBJ model import is supported right now.");
            ShowNotification("Only OBJ model import is supported right now.");
            return false;
        }

        std::filesystem::path targetFolder = "Assets/Models";
        if (!m_selectedAssetFolder.empty())
        {
            const std::filesystem::path selectedFolder(m_selectedAssetFolder);
            if (std::filesystem::exists(selectedFolder) && std::filesystem::is_directory(selectedFolder))
            {
                targetFolder = selectedFolder;
            }
        }

        std::filesystem::create_directories(targetFolder);
        std::filesystem::path targetPath = MakeUniquePath(targetFolder / sourcePath.filename());

        std::error_code error;
        const std::filesystem::path sourceAbsolute = std::filesystem::weakly_canonical(sourcePath, error);
        error.clear();
        const std::filesystem::path targetAbsolute = std::filesystem::weakly_canonical(targetPath.parent_path(), error) / targetPath.filename();
        error.clear();

        if (sourceAbsolute != targetAbsolute)
        {
            std::filesystem::copy_file(sourcePath, targetPath, std::filesystem::copy_options::none, error);
            if (error)
            {
                Core::Log::Error("Failed to import OBJ model: " + error.message());
                ShowNotification("Failed to import OBJ model.");
                return false;
            }
        }
        else
        {
            targetPath = sourcePath;
        }

        const std::filesystem::path sourceMaterialPath = sourcePath.parent_path() / (sourcePath.stem().wstring() + L".mtl");
        if (std::filesystem::exists(sourceMaterialPath))
        {
            const std::filesystem::path targetMaterialPath = targetPath.parent_path() / sourceMaterialPath.filename();
            if (!std::filesystem::exists(targetMaterialPath))
            {
                std::filesystem::copy_file(sourceMaterialPath, targetMaterialPath, std::filesystem::copy_options::none, error);
                error.clear();
            }
        }

        m_selectedAssetFolder = targetPath.parent_path().generic_wstring();
        SetSingleSelectedAsset(targetPath.generic_wstring());
        RefreshAssets();
        Core::Log::Info("Imported OBJ model: " + ToNarrowAscii(targetPath.generic_wstring()));
        ShowNotification("Imported model: " + targetPath.filename().string());
        return true;
    }

    bool EditorLayer::SaveCurrentSceneAs(Scene::Scene& scene, const std::wstring& path)
    {
        if (path.empty())
        {
            return false;
        }

        const int savedMaterialCount = SaveReferencedMaterialAssets(scene);
        std::filesystem::create_directories(std::filesystem::path(path).parent_path());
        if (!Scene::SceneSerializer::SaveScene(path, scene))
        {
            Core::Log::Error("Failed to save scene asset: " + ToNarrowAscii(path));
            return false;
        }

        m_currentSceneAsset = path;
        m_selectedSceneAsset = path;
        scene.SetFilePath(path);
        SetSingleSelectedAsset(path);
        RefreshAssets();
        scene.ClearDirty();
        if (m_commandManager)
        {
            m_commandManager->MarkSaved();
        }
        const std::string savedPath = ToNarrowAscii(path);
        Core::Log::Info("Saved scene asset: " + savedPath);
        ShowNotification(savedMaterialCount > 0
            ? "Scene and materials saved: " + savedPath
            : "Scene saved: " + savedPath);
        return true;
    }

    int EditorLayer::SaveReferencedMaterialAssets(const Scene::Scene& scene)
    {
        std::vector<std::wstring> savedPaths;
        int savedCount = 0;

        for (const std::unique_ptr<Scene::GameObject>& object : scene.GetGameObjects())
        {
            if (!object)
            {
                continue;
            }

            const Scene::MeshRendererComponent* meshRenderer = object->GetComponent<Scene::MeshRendererComponent>();
            if (!meshRenderer || !meshRenderer->GetMaterial())
            {
                continue;
            }

            const std::wstring& materialPath = meshRenderer->GetMaterialPath();
            if (!HasExtension(std::filesystem::path(materialPath), { L".mat" }))
            {
                continue;
            }

            if (std::find(savedPaths.begin(), savedPaths.end(), materialPath) != savedPaths.end())
            {
                continue;
            }

            std::filesystem::create_directories(std::filesystem::path(materialPath).parent_path());
            const std::string fallbackName = std::filesystem::path(materialPath).stem().string();
            Resource::MaterialDesc desc = BuildMaterialDesc(*meshRenderer->GetMaterial(), fallbackName);
            desc.name = fallbackName.empty() ? desc.name : fallbackName;
            if (Resource::MaterialLoader::SaveToFile(materialPath, desc))
            {
                savedPaths.push_back(materialPath);
                ++savedCount;
            }
            else
            {
                Core::Log::Error("Failed to save referenced material asset: " + ToNarrowAscii(materialPath));
            }
        }

        return savedCount;
    }

    bool EditorLayer::SaveSelectedMaterialAs(const std::wstring& path)
    {
        if (!m_selectedObject)
        {
            return false;
        }

        Scene::MeshRendererComponent* meshRenderer = m_selectedObject->GetComponent<Scene::MeshRendererComponent>();
        if (!meshRenderer || !meshRenderer->GetMaterial())
        {
            return false;
        }

        std::filesystem::create_directories(std::filesystem::path(path).parent_path());
        Renderer::Material& material = *meshRenderer->GetMaterial();
        const std::string fallbackName = std::filesystem::path(path).stem().string();
        Resource::MaterialDesc desc = BuildMaterialDesc(material, fallbackName);
        desc.name = fallbackName.empty() ? desc.name : fallbackName;

        if (!Resource::MaterialLoader::SaveToFile(path, desc))
        {
            return false;
        }

        material.SetName(desc.name);
        meshRenderer->SetMaterialPath(path);
        RefreshAssets();
        Core::Log::Info("Saved material as: " + ToNarrowAscii(path));
        return true;
    }

    bool EditorLayer::DeleteSelectedMaterialAsset(Scene::Scene& scene, Renderer::Renderer& renderer)
    {
        if (m_selectedMaterialAsset.empty())
        {
            return false;
        }

        SetSingleSelectedAsset(m_selectedMaterialAsset);
        return DeleteSelectedAssets(scene, renderer);
    }

    bool EditorLayer::DeleteSelectedAssets(Scene::Scene& scene, Renderer::Renderer& renderer)
    {
        std::vector<std::wstring> assetsToDelete = m_selectedAssets;
        if (assetsToDelete.empty() && !m_selectedAsset.empty())
        {
            assetsToDelete.push_back(m_selectedAsset);
        }

        if (assetsToDelete.empty())
        {
            return false;
        }

        std::sort(assetsToDelete.begin(), assetsToDelete.end());
        assetsToDelete.erase(std::unique(assetsToDelete.begin(), assetsToDelete.end()), assetsToDelete.end());

        int deletedCount = 0;
        for (const std::wstring& asset : assetsToDelete)
        {
            if (asset.empty())
            {
                continue;
            }

            const std::filesystem::path path(asset);
            const std::wstring normalizedAsset = NormalizeAssetPath(asset);
            bool sceneReferencesChanged = false;

            for (const std::unique_ptr<Scene::GameObject>& object : scene.GetGameObjects())
            {
                if (!object)
                {
                    continue;
                }

                Scene::MeshRendererComponent* meshRenderer = object->GetComponent<Scene::MeshRendererComponent>();
                if (!meshRenderer)
                {
                    continue;
                }

                if (HasExtension(path, { L".mat" })
                    && NormalizeAssetPath(meshRenderer->GetMaterialPath()) == normalizedAsset)
                {
                    meshRenderer->SetMaterial(nullptr);
                    meshRenderer->SetMaterialPath({});
                    meshRenderer->SetMaterialGuid({});
                    sceneReferencesChanged = true;
                }

                if (HasExtension(path, { L".obj", L".gltf", L".glb", L".fbx" })
                    && NormalizeAssetPath(meshRenderer->GetMeshPath()) == normalizedAsset)
                {
                    meshRenderer->SetMesh(nullptr);
                    meshRenderer->SetMeshPath({});
                    meshRenderer->SetMeshGuid({});
                    sceneReferencesChanged = true;
                }

                if (HasExtension(path, { L".png", L".jpg", L".jpeg", L".bmp", L".tga" })
                    && meshRenderer->GetMaterial())
                {
                    Renderer::Material& material = *meshRenderer->GetMaterial();
                    const MaterialTextureSlot slots[] =
                    {
                        MaterialTextureSlot::BaseColor,
                        MaterialTextureSlot::Roughness,
                        MaterialTextureSlot::Metallic,
                        MaterialTextureSlot::Normal
                    };
                    for (MaterialTextureSlot slot : slots)
                    {
                        if (NormalizeAssetPath(GetMaterialTextureSlotPath(material, slot)) == normalizedAsset)
                        {
                            sceneReferencesChanged = ClearMaterialTextureSlot(material, slot) || sceneReferencesChanged;
                        }
                    }
                }
            }

            if (m_showMaterialEditor && NormalizeAssetPath(m_materialEditorAsset) == normalizedAsset)
            {
                m_showMaterialEditor = false;
                m_materialEditorAsset.clear();
                m_materialEditorMaterial.reset();
                m_materialEditorDirty = false;
            }
            else if (m_materialEditorMaterial && HasExtension(path, { L".png", L".jpg", L".jpeg", L".bmp", L".tga" }))
            {
                const MaterialTextureSlot slots[] =
                {
                    MaterialTextureSlot::BaseColor,
                    MaterialTextureSlot::Roughness,
                    MaterialTextureSlot::Metallic,
                    MaterialTextureSlot::Normal
                };
                for (MaterialTextureSlot slot : slots)
                {
                    if (NormalizeAssetPath(GetMaterialTextureSlotPath(*m_materialEditorMaterial, slot)) == normalizedAsset)
                    {
                        m_materialEditorDirty = ClearMaterialTextureSlot(*m_materialEditorMaterial, slot) || m_materialEditorDirty;
                    }
                }
            }

            if (HasExtension(path, { L".mat" }))
            {
                renderer.GetResourceManager().ForgetMaterial(asset);
            }

            if (m_currentSceneAsset == asset)
            {
                m_currentSceneAsset.clear();
                scene.SetFilePath({});
                sceneReferencesChanged = true;
            }
            if (m_selectedSceneAsset == asset)
            {
                m_selectedSceneAsset.clear();
            }
            if (m_selectedMaterialAsset == asset)
            {
                m_selectedMaterialAsset.clear();
            }

            if (!m_assetDatabase.DeleteAsset(path))
            {
                Core::Log::Error("Failed to delete asset: " + ToNarrowAscii(asset));
                continue;
            }

            if (sceneReferencesChanged)
            {
                scene.MarkDirty();
            }
            ++deletedCount;
        }

        m_selectedAsset.clear();
        m_selectedAssets.clear();
        RefreshAssets();
        if (deletedCount > 0)
        {
            ShowNotification("Deleted asset(s): " + std::to_string(deletedCount));
        }
        return deletedCount > 0;
    }

    bool EditorLayer::ApplyTextureToSelectedMaterial(Renderer::Renderer& renderer, const std::wstring& texturePath)
    {
        return ApplyTextureToSelectedMaterial(renderer, texturePath, MaterialTextureSlot::BaseColor);
    }

    bool EditorLayer::ApplyTextureToSelectedMaterial(Renderer::Renderer& renderer, const std::wstring& texturePath, MaterialTextureSlot slot)
    {
        std::vector<Scene::GameObject*> targets = m_selectedObjects;
        if (targets.empty() && m_selectedObject)
        {
            targets.push_back(m_selectedObject);
        }
        if (targets.empty())
        {
            return false;
        }

        bool applied = false;
        Resource::ResourceManager& resources = renderer.GetResourceManager();
        for (Scene::GameObject* object : targets)
        {
            if (!object)
            {
                continue;
            }

            Scene::MeshRendererComponent* meshRenderer = object->GetComponent<Scene::MeshRendererComponent>();
            if (!meshRenderer)
            {
                meshRenderer = &object->AddComponent<Scene::MeshRendererComponent>();
                meshRenderer->SetMesh(resources.CreateCubeMesh("builtin:mesh:cube"));
                meshRenderer->SetMeshPath(L"builtin:mesh:cube");
            }

            if (!meshRenderer->GetMaterial())
            {
                meshRenderer->SetMaterial(resources.CreateMaterial("editor:material:auto_texture:" + object->GetName(), { 1.0f, 1.0f, 1.0f, 1.0f }));
                meshRenderer->SetMaterialPath({});
                meshRenderer->SetMaterialGuid({});
            }

            Renderer::Material* material = EnsureEditableMaterialInstance(*meshRenderer, "MaterialInstance:" + object->GetName());
            if (!material)
            {
                continue;
            }

            applied = ApplyTextureToMaterial(renderer, *material, texturePath, slot) || applied;
        }

        if (applied)
        {
            Core::Log::Info(std::string("Applied ") + ToMaterialTextureSlotName(slot) + " texture: " + ToNarrowAscii(texturePath));
        }
        return applied;
    }

    bool EditorLayer::ApplyTextureToMaterial(Renderer::Renderer& renderer, Renderer::Material& material, const std::wstring& texturePath, MaterialTextureSlot slot)
    {
        std::shared_ptr<RHI::RHITexture> texture = renderer.GetResourceManager().LoadTexture(texturePath, MakeTextureLoadOptions(slot));
        if (!texture)
        {
            return false;
        }

        switch (slot)
        {
        case MaterialTextureSlot::Roughness:
            material.SetRoughnessTexture(texture);
            material.SetRoughnessTexturePath(texturePath);
            break;
        case MaterialTextureSlot::Metallic:
            material.SetMetallicTexture(texture);
            material.SetMetallicTexturePath(texturePath);
            break;
        case MaterialTextureSlot::Normal:
            material.SetNormalTexture(texture);
            material.SetNormalTexturePath(texturePath);
            break;
        case MaterialTextureSlot::BaseColor:
        default:
            material.SetBaseColorTexture(texture);
            material.SetBaseColorTexturePath(texturePath);
            break;
        }

        return true;
    }

    bool EditorLayer::ClearSelectedMaterialTextureSlot(MaterialTextureSlot slot)
    {
        std::vector<Scene::GameObject*> targets = m_selectedObjects;
        if (targets.empty() && m_selectedObject)
        {
            targets.push_back(m_selectedObject);
        }
        if (targets.empty())
        {
            return false;
        }

        bool cleared = false;
        for (Scene::GameObject* object : targets)
        {
            if (!object)
            {
                continue;
            }

            Scene::MeshRendererComponent* meshRenderer = object->GetComponent<Scene::MeshRendererComponent>();
            if (!meshRenderer || !meshRenderer->GetMaterial())
            {
                continue;
            }

            Renderer::Material* material = EnsureEditableMaterialInstance(*meshRenderer, "MaterialInstance:" + object->GetName());
            if (!material)
            {
                continue;
            }

            cleared = ClearMaterialTextureSlot(*material, slot) || cleared;
        }

        if (cleared)
        {
            Core::Log::Info(std::string("Cleared ") + ToMaterialTextureSlotName(slot) + " texture slot.");
        }
        return cleared;
    }

    bool EditorLayer::ClearMaterialTextureSlot(Renderer::Material& material, MaterialTextureSlot slot)
    {
        switch (slot)
        {
        case MaterialTextureSlot::Roughness:
            if (!material.GetRoughnessTexture() && material.GetRoughnessTexturePath().empty())
            {
                return false;
            }
            material.SetRoughnessTexture(nullptr);
            material.SetRoughnessTexturePath({});
            return true;
        case MaterialTextureSlot::Metallic:
            if (!material.GetMetallicTexture() && material.GetMetallicTexturePath().empty())
            {
                return false;
            }
            material.SetMetallicTexture(nullptr);
            material.SetMetallicTexturePath({});
            return true;
        case MaterialTextureSlot::Normal:
            if (!material.GetNormalTexture() && material.GetNormalTexturePath().empty())
            {
                return false;
            }
            material.SetNormalTexture(nullptr);
            material.SetNormalTexturePath({});
            return true;
        case MaterialTextureSlot::BaseColor:
        default:
            if (!material.GetBaseColorTexture() && material.GetBaseColorTexturePath().empty())
            {
                return false;
            }
            material.SetBaseColorTexture(nullptr);
            material.SetBaseColorTexturePath({});
            return true;
        }
    }

    void EditorLayer::OpenMaterialEditor(Renderer::Renderer& renderer, const std::wstring& materialAsset)
    {
        if (!HasExtension(std::filesystem::path(materialAsset), { L".mat" }))
        {
            return;
        }

        std::shared_ptr<Renderer::Material> material = renderer.GetResourceManager().LoadMaterial(materialAsset);
        if (!material)
        {
            Core::Log::Error("Failed to open material editor: " + ToNarrowAscii(materialAsset));
            ShowNotification("Failed to open material.");
            return;
        }

        m_materialEditorAsset = materialAsset;
        m_materialEditorMaterial = material;
        m_materialEditorDirty = false;
        m_showMaterialEditor = true;
        m_selectedMaterialAsset = materialAsset;
        SetSingleSelectedAsset(materialAsset);
        Core::Log::Info("Opened material editor: " + ToNarrowAscii(materialAsset));
    }

    bool EditorLayer::SaveMaterialEditor()
    {
        if (m_materialEditorAsset.empty() || !m_materialEditorMaterial)
        {
            return false;
        }

        std::filesystem::create_directories(std::filesystem::path(m_materialEditorAsset).parent_path());
        const std::string fallbackName = std::filesystem::path(m_materialEditorAsset).stem().string();
        Resource::MaterialDesc desc = BuildMaterialDesc(*m_materialEditorMaterial, fallbackName);
        desc.name = fallbackName.empty() ? desc.name : fallbackName;
        if (!Resource::MaterialLoader::SaveToFile(m_materialEditorAsset, desc))
        {
            Core::Log::Error("Failed to save material: " + ToNarrowAscii(m_materialEditorAsset));
            ShowNotification("Failed to save material.");
            return false;
        }

        m_materialEditorMaterial->SetName(desc.name);
        m_materialEditorDirty = false;
        RefreshAssets();
        Core::Log::Info("Saved material: " + ToNarrowAscii(m_materialEditorAsset));
        ShowNotification("Material saved.");
        return true;
    }

    void EditorLayer::SyncMaterialEditorMaterialToScene(Scene::Scene& scene)
    {
        if (m_materialEditorAsset.empty() || !m_materialEditorMaterial)
        {
            return;
        }

        const std::wstring normalizedEditorAsset = NormalizeAssetPath(m_materialEditorAsset);
        const std::string editorMaterialGuid = m_assetDatabase.GetGuidFromPath(m_materialEditorAsset);
        bool changedAnyReference = false;

        for (const std::unique_ptr<Scene::GameObject>& object : scene.GetGameObjects())
        {
            if (!object)
            {
                continue;
            }

            Scene::MeshRendererComponent* meshRenderer = object->GetComponent<Scene::MeshRendererComponent>();
            if (!meshRenderer || !meshRenderer->GetMaterial())
            {
                continue;
            }

            const bool pathMatches = !meshRenderer->GetMaterialPath().empty()
                && NormalizeAssetPath(meshRenderer->GetMaterialPath()) == normalizedEditorAsset;
            const bool guidMatches = !editorMaterialGuid.empty()
                && !meshRenderer->GetMaterialGuid().empty()
                && meshRenderer->GetMaterialGuid() == editorMaterialGuid;
            if (!pathMatches && !guidMatches)
            {
                continue;
            }

            CopyMaterialState(*m_materialEditorMaterial, *meshRenderer->GetMaterial());
            meshRenderer->SetMaterialPath(m_materialEditorAsset);
            meshRenderer->SetMaterialGuid(editorMaterialGuid);
            changedAnyReference = true;
        }

        if (changedAnyReference)
        {
            scene.MarkDirty();
        }
    }

    void EditorLayer::RenderMaterialEditor(Scene::Scene& scene, Renderer::Renderer& renderer)
    {
        if (!m_showMaterialEditor)
        {
            return;
        }

        if (!m_materialEditorMaterial)
        {
            m_showMaterialEditor = false;
            m_materialEditorAsset.clear();
            m_materialEditorDirty = false;
            return;
        }

        const std::string title = "Material Editor - " + ToNarrowAscii(std::filesystem::path(m_materialEditorAsset).filename().wstring());
        ImGui::SetNextWindowSize(ImVec2(460.0f, 560.0f), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin(title.c_str(), &m_showMaterialEditor))
        {
            ImGui::End();
            return;
        }

        Renderer::Material& material = *m_materialEditorMaterial;
        ImGui::Text("Asset: %s", ToNarrowAscii(m_materialEditorAsset).c_str());
        ImGui::Separator();
        bool materialChangedThisFrame = false;

        Math::Vector4 baseColor = material.GetBaseColor();
        if (ImGui::ColorEdit4("Base Color", &baseColor.x))
        {
            material.SetBaseColor(baseColor);
            m_materialEditorDirty = true;
            materialChangedThisFrame = true;
        }

        float roughness = material.GetRoughness();
        if (SliderAndInputFloat("Roughness", &roughness, 0.0f, 1.0f))
        {
            material.SetRoughness(roughness);
            m_materialEditorDirty = true;
            materialChangedThisFrame = true;
        }

        float metallic = material.GetMetallic();
        if (SliderAndInputFloat("Metallic", &metallic, 0.0f, 1.0f))
        {
            material.SetMetallic(metallic);
            m_materialEditorDirty = true;
            materialChangedThisFrame = true;
        }

        float normalStrength = material.GetNormalStrength();
        if (SliderAndInputFloat("Normal Strength", &normalStrength, 0.0f, 4.0f))
        {
            material.SetNormalStrength(normalStrength);
            m_materialEditorDirty = true;
            materialChangedThisFrame = true;
        }

        ImGui::SeparatorText("Texture Slots");
        auto drawTextureSlot = [&](MaterialTextureSlot slot)
        {
            const std::wstring& path = GetMaterialTextureSlotPath(material, slot);
            const std::string pathText = ToNarrowAscii(path);
            ImGui::PushID(ToMaterialTextureSlotName(slot));
            ImGui::Text("%s Map", ToMaterialTextureSlotName(slot));
            ImGui::SameLine(120.0f);
            const std::string buttonText = pathText.empty() ? "(drop texture here)" : pathText;
            ImGui::Button(buttonText.c_str(), ImVec2(-72.0f, 0.0f));
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_TEXTURE"))
                {
                    const std::wstring texturePath = ReadWideAssetPayload(payload);
                    if (!texturePath.empty())
                    {
                        const bool changed = ApplyTextureToMaterial(renderer, material, texturePath, slot);
                        m_materialEditorDirty = changed || m_materialEditorDirty;
                        materialChangedThisFrame = changed || materialChangedThisFrame;
                    }
                }
                ImGui::EndDragDropTarget();
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear", ImVec2(60.0f, 0.0f)))
            {
                const bool changed = ClearMaterialTextureSlot(material, slot);
                m_materialEditorDirty = changed || m_materialEditorDirty;
                materialChangedThisFrame = changed || materialChangedThisFrame;
            }

            const std::shared_ptr<RHI::RHITexture>* texture = nullptr;
            switch (slot)
            {
            case MaterialTextureSlot::Roughness:
                texture = &material.GetRoughnessTexture();
                break;
            case MaterialTextureSlot::Metallic:
                texture = &material.GetMetallicTexture();
                break;
            case MaterialTextureSlot::Normal:
                texture = &material.GetNormalTexture();
                break;
            case MaterialTextureSlot::BaseColor:
            default:
                texture = &material.GetBaseColorTexture();
                break;
            }

            if (texture && *texture && (*texture)->GetNativeShaderResourceHandleForUI())
            {
                ImGui::Image(MakeExternalTextureRef((*texture)->GetNativeShaderResourceHandleForUI()), ImVec2(72.0f, 72.0f));
            }
            ImGui::PopID();
        };

        drawTextureSlot(MaterialTextureSlot::BaseColor);
        drawTextureSlot(MaterialTextureSlot::Roughness);
        drawTextureSlot(MaterialTextureSlot::Metallic);
        drawTextureSlot(MaterialTextureSlot::Normal);

        if (materialChangedThisFrame)
        {
            SyncMaterialEditorMaterialToScene(scene);
        }

        ImGui::Separator();
        if (ImGui::Button("Save", ImVec2(96.0f, 0.0f)))
        {
            if (SaveMaterialEditor())
            {
                SyncMaterialEditorMaterialToScene(scene);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Reload", ImVec2(96.0f, 0.0f)))
        {
            renderer.GetResourceManager().ForgetMaterial(m_materialEditorAsset);
            OpenMaterialEditor(renderer, m_materialEditorAsset);
            SyncMaterialEditorMaterialToScene(scene);
        }
        ImGui::SameLine();
        if (m_materialEditorDirty)
        {
            ImGui::TextUnformatted("* modified");
        }

        ImGui::End();
    }

    void EditorLayer::OpenModelEditor(const std::wstring& modelAsset)
    {
        if (!HasExtension(std::filesystem::path(modelAsset), { L".obj" }))
        {
            return;
        }

        m_modelEditorAsset = modelAsset;
        LoadModelColliderAsset(modelAsset);
        m_showModelEditor = true;
        if (m_modelEditorColliders.empty())
        {
            ModelColliderEntry collider;
            collider.type = Physics::ColliderType::AABB;
            collider.size = { 1.0f, 1.0f, 1.0f };
            m_modelEditorColliders.push_back(collider);
        }
        m_selectedModelColliderIndex = 0;
        ApplyModelCollidersToSelectedObjects();
    }

    bool EditorLayer::LoadModelColliderAsset(const std::wstring& modelAsset)
    {
        m_modelEditorColliders.clear();
        m_selectedModelColliderIndex = -1;

        const std::filesystem::path colliderPath = std::filesystem::path(modelAsset).concat(L".colliders");
        std::ifstream file(colliderPath);
        if (!file.is_open())
        {
            return false;
        }

        ModelColliderEntry current;
        bool hasCurrent = false;
        std::string line;
        while (std::getline(file, line))
        {
            line = Trim(line);
            if (line.empty() || line[0] == '#')
            {
                continue;
            }

            if (line == "[Collider]")
            {
                if (hasCurrent)
                {
                    m_modelEditorColliders.push_back(current);
                }
                current = {};
                hasCurrent = true;
                continue;
            }

            const std::size_t separator = line.find('=');
            if (separator == std::string::npos || !hasCurrent)
            {
                continue;
            }

            const std::string key = Trim(line.substr(0, separator));
            const std::string value = Trim(line.substr(separator + 1));
            try
            {
                if (key == "type")
                {
                    current.type = value == "Sphere" ? Physics::ColliderType::Sphere : Physics::ColliderType::AABB;
                }
                else if (key == "center" || key == "size")
                {
                    std::stringstream stream(value);
                    std::string part;
                    float values[3] = {};
                    for (int i = 0; i < 3 && std::getline(stream, part, ','); ++i)
                    {
                        values[i] = std::stof(Trim(part));
                    }
                    if (key == "center")
                    {
                        current.center = { values[0], values[1], values[2] };
                    }
                    else
                    {
                        current.size = { values[0], values[1], values[2] };
                    }
                }
                else if (key == "rotation")
                {
                    std::stringstream stream(value);
                    std::string part;
                    float values[3] = {};
                    for (int i = 0; i < 3 && std::getline(stream, part, ','); ++i)
                    {
                        values[i] = std::stof(Trim(part));
                    }
                    current.rotation =
                    {
                        DirectX::XMConvertToRadians(values[0]),
                        DirectX::XMConvertToRadians(values[1]),
                        DirectX::XMConvertToRadians(values[2])
                    };
                }
                else if (key == "radius")
                {
                    current.radius = std::stof(value);
                }
                else if (key == "is_trigger")
                {
                    current.isTrigger = value == "true" || value == "1" || value == "yes";
                }
            }
            catch (const std::exception&)
            {
                Core::Log::Warning("Skipped malformed model collider property: " + line);
            }
        }

        if (hasCurrent)
        {
            m_modelEditorColliders.push_back(current);
        }
        if (!m_modelEditorColliders.empty())
        {
            m_selectedModelColliderIndex = 0;
        }
        return true;
    }

    bool EditorLayer::SaveModelColliderAsset() const
    {
        if (m_modelEditorAsset.empty())
        {
            return false;
        }

        const std::filesystem::path colliderPath = std::filesystem::path(m_modelEditorAsset).concat(L".colliders");
        std::ofstream file(colliderPath);
        if (!file.is_open())
        {
            Core::Log::Error("Failed to save model colliders: " + ToNarrowAscii(colliderPath.generic_wstring()));
            return false;
        }

        for (const ModelColliderEntry& collider : m_modelEditorColliders)
        {
            file << "[Collider]\n";
            file << "type=" << ToColliderTypeName(collider.type) << "\n";
            file << "center=" << collider.center.x << "," << collider.center.y << "," << collider.center.z << "\n";
            file << "rotation=" << DirectX::XMConvertToDegrees(collider.rotation.x) << "," << DirectX::XMConvertToDegrees(collider.rotation.y) << "," << DirectX::XMConvertToDegrees(collider.rotation.z) << "\n";
            file << "size=" << collider.size.x << "," << collider.size.y << "," << collider.size.z << "\n";
            file << "radius=" << collider.radius << "\n";
            file << "is_trigger=" << (collider.isTrigger ? "true" : "false") << "\n\n";
        }

        return true;
    }

    bool EditorLayer::ApplyModelCollidersToSelectedObjects()
    {
        std::vector<Scene::GameObject*> targets = m_selectedObjects;
        if (targets.empty() && m_selectedObject)
        {
            targets.push_back(m_selectedObject);
        }
        if (targets.empty())
        {
            return false;
        }

        for (Scene::GameObject* object : targets)
        {
            if (!object)
            {
                continue;
            }

            object->RemoveComponents<Physics::ColliderComponent>();
            for (const ModelColliderEntry& source : m_modelEditorColliders)
            {
                Physics::ColliderComponent& collider = object->AddComponent<Physics::ColliderComponent>();
                collider.type = source.type;
                collider.center = source.center;
                collider.rotation = source.rotation;
                collider.size = source.size;
                collider.radius = source.radius;
                collider.isTrigger = source.isTrigger;
            }
        }

        return true;
    }

    void EditorLayer::RenderModelEditor(Scene::Scene&, Renderer::Renderer& renderer)
    {
        if (!m_showModelEditor)
        {
            return;
        }

        ImGui::SetNextWindowSize(ImVec2(640.0f, 520.0f), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Model Asset Editor", &m_showModelEditor))
        {
            ImGui::End();
            return;
        }

        const std::string assetPath = ToNarrowAscii(m_modelEditorAsset);
        ImGui::Text("Model: %s", assetPath.c_str());
        ImGui::Separator();

        if (ImGui::Button("Place Model In Scene"))
        {
            if (!m_modelEditorAsset.empty())
            {
                Resource::ResourceManager& resources = renderer.GetResourceManager();
                Scene::GameObject* object = m_selectedObject;
                if (!object)
                {
                    ImGui::TextDisabled("Select or place an object from the Asset Browser first.");
                }
                else
                {
                    Scene::MeshRendererComponent* meshRenderer = object->GetComponent<Scene::MeshRendererComponent>();
                    if (!meshRenderer)
                    {
                        meshRenderer = &object->AddComponent<Scene::MeshRendererComponent>();
                    }
                    meshRenderer->SetMesh(resources.LoadMesh(m_modelEditorAsset));
                    meshRenderer->SetMeshPath(m_modelEditorAsset);
                    meshRenderer->SetMeshGuid(m_assetDatabase.GetGuidFromPath(m_modelEditorAsset));
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Apply Colliders To Selected"))
        {
            if (ApplyModelCollidersToSelectedObjects())
            {
                ShowNotification("Applied model colliders to selected object(s).");
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Save Colliders"))
        {
            if (SaveModelColliderAsset())
            {
                ShowNotification("Saved model colliders.");
                RefreshAssets();
            }
        }

        ImGui::SeparatorText("Collider List");
        bool colliderSetChanged = false;
        if (ImGui::Button("Add Box"))
        {
            ModelColliderEntry collider;
            collider.type = Physics::ColliderType::AABB;
            m_modelEditorColliders.push_back(collider);
            m_selectedModelColliderIndex = static_cast<int>(m_modelEditorColliders.size()) - 1;
            colliderSetChanged = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Sphere"))
        {
            ModelColliderEntry collider;
            collider.type = Physics::ColliderType::Sphere;
            m_modelEditorColliders.push_back(collider);
            m_selectedModelColliderIndex = static_cast<int>(m_modelEditorColliders.size()) - 1;
            colliderSetChanged = true;
        }
        ImGui::SameLine();
        if (m_selectedModelColliderIndex >= 0 && ImGui::Button("Delete Collider"))
        {
            const std::size_t index = static_cast<std::size_t>(m_selectedModelColliderIndex);
            if (index < m_modelEditorColliders.size())
            {
                m_modelEditorColliders.erase(m_modelEditorColliders.begin() + index);
                m_selectedModelColliderIndex = m_modelEditorColliders.empty()
                    ? -1
                    : std::clamp(m_selectedModelColliderIndex, 0, static_cast<int>(m_modelEditorColliders.size()) - 1);
                colliderSetChanged = true;
            }
        }

        if (ImGui::BeginTable("ModelColliderEditorLayout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("Colliders", ImGuiTableColumnFlags_WidthFixed, 190.0f);
            ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            for (std::size_t i = 0; i < m_modelEditorColliders.size(); ++i)
            {
                ImGui::PushID(static_cast<int>(i));
                const ModelColliderEntry& collider = m_modelEditorColliders[i];
                const std::string label = std::string(ToColliderTypeName(collider.type)) + " " + std::to_string(i);
                if (ImGui::Selectable(label.c_str(), m_selectedModelColliderIndex == static_cast<int>(i)))
                {
                    m_selectedModelColliderIndex = static_cast<int>(i);
                }
                ImGui::PopID();
            }

            ImGui::TableSetColumnIndex(1);
            if (m_selectedModelColliderIndex >= 0
                && static_cast<std::size_t>(m_selectedModelColliderIndex) < m_modelEditorColliders.size())
            {
                ModelColliderEntry& collider = m_modelEditorColliders[static_cast<std::size_t>(m_selectedModelColliderIndex)];
                const char* colliderTypes[] = { "AABB", "Sphere" };
                int currentType = collider.type == Physics::ColliderType::Sphere ? 1 : 0;
                if (ImGui::Combo("Type", &currentType, colliderTypes, IM_ARRAYSIZE(colliderTypes)))
                {
                    collider.type = currentType == 1 ? Physics::ColliderType::Sphere : Physics::ColliderType::AABB;
                    colliderSetChanged = true;
                }
                colliderSetChanged = DragAndInputFloat3("Center", &collider.center.x, 0.05f) || colliderSetChanged;
                Math::Vector3 rotationDegrees =
                {
                    DirectX::XMConvertToDegrees(collider.rotation.x),
                    DirectX::XMConvertToDegrees(collider.rotation.y),
                    DirectX::XMConvertToDegrees(collider.rotation.z)
                };
                if (DragAndInputFloat3("Rotation (deg)", &rotationDegrees.x, 1.0f))
                {
                    collider.rotation =
                    {
                        DirectX::XMConvertToRadians(rotationDegrees.x),
                        DirectX::XMConvertToRadians(rotationDegrees.y),
                        DirectX::XMConvertToRadians(rotationDegrees.z)
                    };
                    colliderSetChanged = true;
                }
                colliderSetChanged = DragAndInputFloat3("Size", &collider.size.x, 0.05f, 0.01f, 1000.0f) || colliderSetChanged;
                colliderSetChanged = DragAndInputFloat("Radius", &collider.radius, 0.05f, 0.01f, 1000.0f) || colliderSetChanged;
                collider.size.x = std::clamp(collider.size.x, 0.01f, 1000.0f);
                collider.size.y = std::clamp(collider.size.y, 0.01f, 1000.0f);
                collider.size.z = std::clamp(collider.size.z, 0.01f, 1000.0f);
                collider.radius = std::clamp(collider.radius, 0.01f, 1000.0f);
                colliderSetChanged = ImGui::Checkbox("Is Trigger", &collider.isTrigger) || colliderSetChanged;
            }
            else
            {
                ImGui::TextDisabled("Add or select a collider.");
            }

            ImGui::EndTable();
        }

        if (colliderSetChanged)
        {
            ApplyModelCollidersToSelectedObjects();
        }

        ImGui::SeparatorText("Workflow");
        ImGui::TextWrapped("Double-click an OBJ asset to edit its reusable collider setup. Edits are previewed on the selected scene object immediately. Save writes a .colliders sidecar next to the model.");
        ImGui::End();
    }

    Scene::GameObject* EditorLayer::DuplicateSelectedObject(Scene::Scene& scene)
    {
        EnsureCommandManager(scene);
        if (!m_selectedObject)
        {
            return nullptr;
        }

        std::vector<Scene::GameObject*> sources = m_selectedObjects;
        if (sources.empty())
        {
            sources.push_back(m_selectedObject);
        }

        std::sort(sources.begin(), sources.end());
        sources.erase(std::unique(sources.begin(), sources.end()), sources.end());

        auto command = std::make_unique<DuplicateGameObjectCommand>(scene, sources);
        DuplicateGameObjectCommand* commandPtr = command.get();
        if (m_commandManager)
        {
            m_commandManager->ExecuteCommand(std::move(command));
            m_selectedObjects = commandPtr->GetDuplicates();
            m_selectedObject = m_selectedObjects.empty() ? nullptr : m_selectedObjects.back();
        }

        Core::Log::Info("Duplicated selected object(s) with Alt + gizmo drag.");
        return m_selectedObject;
    }

    Scene::GameObject* EditorLayer::GetSelectedObject() const
    {
        return m_selectedObject;
    }

    const std::vector<Scene::GameObject*>& EditorLayer::GetSelectedObjects() const
    {
        return m_selectedObjects;
    }

    bool EditorLayer::IsPlayMode() const
    {
        return m_playMode;
    }

    bool EditorLayer::WantsInputCapture() const
    {
        return m_wantsInputCapture;
    }

    bool EditorLayer::ShouldShowColliders() const
    {
        return m_showColliders;
    }

    bool EditorLayer::ShouldShowGizmo() const
    {
        return m_showGizmoTools && m_selectedObject != nullptr;
    }

    Renderer::GizmoVisualMode EditorLayer::GetGizmoVisualMode() const
    {
        switch (m_gizmoMode)
        {
        case GizmoMode::Rotate:
            return Renderer::GizmoVisualMode::Rotate;
        case GizmoMode::Scale:
            return Renderer::GizmoVisualMode::Scale;
        case GizmoMode::Move:
        default:
            return Renderer::GizmoVisualMode::Move;
        }
    }

    Renderer::GizmoAxis EditorLayer::GetHoveredGizmoAxis() const
    {
        return ToRendererGizmoAxis(m_hoveredGizmoAxis);
    }

    Renderer::GizmoAxis EditorLayer::GetActiveGizmoAxis() const
    {
        return ToRendererGizmoAxis(m_activeGizmoAxis);
    }

    GizmoAxis EditorLayer::PickGizmoAxis(const Scene::Camera& camera, int mouseX, int mouseY, float width, float height) const
    {
        if (!m_selectedObject || width <= 0.0f || height <= 0.0f)
        {
            return GizmoAxis::None;
        }

        const Math::Transform& transform = m_selectedObject->GetTransform();
        const Math::Vector3 origin = transform.position;
        const ImVec2 mouse = { static_cast<float>(mouseX), static_cast<float>(mouseY) };
        const ImVec2 originScreen = ProjectWorldToScreen(camera, origin, width, height);
        constexpr float pickThresholdPixels = 14.0f;
        const float ringRadius = GetGizmoRadiusForCamera(camera, origin);
        const float axisLength = ringRadius * 1.25f;
        const Math::Vector3 localAxes[] =
        {
            GetLocalAxisVector(transform, GizmoAxis::X),
            GetLocalAxisVector(transform, GizmoAxis::Y),
            GetLocalAxisVector(transform, GizmoAxis::Z)
        };

        GizmoAxis bestAxis = GizmoAxis::None;
        float bestDistance = pickThresholdPixels;
        const GizmoAxis axes[] = { GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z };

        if (m_gizmoMode == GizmoMode::Rotate)
        {
            const Physics::Ray ray = MakeMouseRayFromCamera(camera, mouseX, mouseY, width, height);
            const Math::Vector3 worldAxes[] =
            {
                GetAxisVector(GizmoAxis::X),
                GetAxisVector(GizmoAxis::Y),
                GetAxisVector(GizmoAxis::Z)
            };
            const float worldTolerance = std::max(0.05f, ringRadius * 0.12f);

            for (int i = 0; i < 3; ++i)
            {
                Math::Vector3 hitPoint;
                if (!IntersectRayPlane(ray, origin, worldAxes[i], hitPoint))
                {
                    continue;
                }

                const float radiusDistance = std::abs(Distance(hitPoint, origin) - ringRadius);
                if (radiusDistance > worldTolerance)
                {
                    continue;
                }

                const Math::Vector3 basisA = i == 0 ? GetAxisVector(GizmoAxis::Y) : GetAxisVector(GizmoAxis::X);
                const Math::Vector3 basisB = i == 2 ? GetAxisVector(GizmoAxis::Y) : GetAxisVector(GizmoAxis::Z);
                const float screenDistance = DistancePointToProjectedCircle(camera, mouse, origin, ringRadius, basisA, basisB, width, height);
                if (screenDistance < bestDistance)
                {
                    bestDistance = screenDistance;
                    bestAxis = axes[i];
                }
            }
        }
        else
        {
            for (int i = 0; i < 3; ++i)
            {
                const Math::Vector3 axisVector = localAxes[i];
                const Math::Vector3 end =
                {
                    origin.x + axisVector.x * axisLength,
                    origin.y + axisVector.y * axisLength,
                    origin.z + axisVector.z * axisLength
                };
                const ImVec2 endScreen = ProjectWorldToScreen(camera, end, width, height);
                const float distance = DistancePointToSegment(mouse, originScreen, endScreen);
                if (distance < bestDistance)
                {
                    bestDistance = distance;
                    bestAxis = axes[i];
                }
            }
        }

        return bestAxis;
    }

    void EditorLayer::BeginGizmoDrag(const Scene::Camera& camera, const Input::Input& input, float width, float height)
    {
        m_moveSnapRemainder = 0.0f;
        m_rotateSnapRemainder = 0.0f;
        m_scaleSnapRemainder = 0.0f;
        m_gizmoRotateDragValid = false;
        m_gizmoDragStartTransforms.clear();

        if (!m_selectedObject)
        {
            return;
        }

        const Math::Transform& transform = m_selectedObject->GetTransform();
        m_gizmoDragObject = m_selectedObject;
        m_gizmoDragStartTransform = transform;
        std::vector<Scene::GameObject*> targets = m_selectedObjects;
        if (targets.empty())
        {
            targets.push_back(m_selectedObject);
        }
        std::sort(targets.begin(), targets.end());
        targets.erase(std::unique(targets.begin(), targets.end()), targets.end());
        for (Scene::GameObject* object : targets)
        {
            if (object)
            {
                m_gizmoDragStartTransforms.push_back({ object, object->GetTransform() });
            }
        }

        m_dragStartLocalAxes[0] = GetLocalAxisVector(transform, GizmoAxis::X);
        m_dragStartLocalAxes[1] = GetLocalAxisVector(transform, GizmoAxis::Y);
        m_dragStartLocalAxes[2] = GetLocalAxisVector(transform, GizmoAxis::Z);

        if (m_gizmoMode != GizmoMode::Rotate || width <= 0.0f || height <= 0.0f)
        {
            return;
        }

        m_gizmoDragCenter = transform.position;
        m_gizmoDragAxis = GetAxisVector(m_activeGizmoAxis);
        const ImVec2 screenCenter = ProjectWorldToScreen(camera, m_gizmoDragCenter, width, height);
        m_gizmoDragScreenCenterX = screenCenter.x;
        m_gizmoDragScreenCenterY = screenCenter.y;
        if (!GetScreenCircleAngle(
            m_gizmoDragScreenCenterX,
            m_gizmoDragScreenCenterY,
            static_cast<float>(input.GetMouseX()),
            static_cast<float>(input.GetMouseY()),
            m_gizmoDragStartScreenAngle))
        {
            m_activeGizmoAxis = GizmoAxis::None;
            return;
        }

        m_gizmoRotateDragValid = true;
        if (m_debugRotationGizmo)
        {
            Core::Log::Info("Rotate gizmo drag started. Axis=" + std::to_string(GetAxisIndex(m_activeGizmoAxis)));
        }
    }

    void EditorLayer::EndGizmoDrag()
    {
        if (m_gizmoDragObject && TransformCommand::IsDifferent(m_gizmoDragStartTransform, m_gizmoDragObject->GetTransform()))
        {
            if (m_commandManager)
            {
                m_commandManager->ExecuteCommand(std::make_unique<TransformCommand>(*m_gizmoDragObject, m_gizmoDragStartTransform, m_gizmoDragObject->GetTransform()));
            }
        }
        m_gizmoDragObject = nullptr;
        m_gizmoDragStartTransforms.clear();
        m_gizmoRotateDragValid = false;
        m_activeGizmoAxis = GizmoAxis::None;
        m_hoveredGizmoAxis = GizmoAxis::None;
        m_moveSnapRemainder = 0.0f;
        m_rotateSnapRemainder = 0.0f;
        m_scaleSnapRemainder = 0.0f;
    }

    void EditorLayer::ApplyGizmoDrag(const Scene::Camera& camera, const Input::Input& input, float width, float height)
    {
        if (!m_selectedObject || m_activeGizmoAxis == GizmoAxis::None || width <= 0.0f || height <= 0.0f)
        {
            return;
        }

        const int mouseDeltaX = input.GetMouseDeltaX();
        const int mouseDeltaY = input.GetMouseDeltaY();
        if (mouseDeltaX == 0 && mouseDeltaY == 0)
        {
            return;
        }

        std::vector<Scene::GameObject*> targets = m_selectedObjects;
        if (targets.empty())
        {
            targets.push_back(m_selectedObject);
        }
        std::sort(targets.begin(), targets.end());
        targets.erase(std::unique(targets.begin(), targets.end()), targets.end());

        const Math::Transform& primaryTransform = m_selectedObject->GetTransform();
        const Math::Vector3 origin = primaryTransform.position;
        const Math::Vector3 axisVector = m_dragStartLocalAxes[GetAxisIndex(m_activeGizmoAxis)];
        const Math::Vector3 end =
        {
            origin.x + axisVector.x,
            origin.y + axisVector.y,
            origin.z + axisVector.z
        };

        const ImVec2 originScreen = ProjectWorldToScreen(camera, origin, width, height);
        const ImVec2 endScreen = ProjectWorldToScreen(camera, end, width, height);
        float axisScreenX = endScreen.x - originScreen.x;
        float axisScreenY = endScreen.y - originScreen.y;
        const float axisLength = std::sqrt(axisScreenX * axisScreenX + axisScreenY * axisScreenY);
        float projectedPixels = 0.0f;
        if (axisLength > 0.001f)
        {
            axisScreenX /= axisLength;
            axisScreenY /= axisLength;
            projectedPixels = static_cast<float>(mouseDeltaX) * axisScreenX + static_cast<float>(mouseDeltaY) * axisScreenY;
        }
        else if (m_gizmoMode != GizmoMode::Rotate)
        {
            return;
        }

        const DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat3(&camera.position);
        const DirectX::XMVECTOR objectPosition = DirectX::XMLoadFloat3(&origin);
        const float cameraDistance = std::max(0.1f, DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSubtract(objectPosition, cameraPosition))));
        const float worldUnitsPerPixel = (2.0f * cameraDistance * std::tan(camera.fov * 0.5f)) / height;

        switch (m_gizmoMode)
        {
        case GizmoMode::Move:
        {
            m_moveSnapRemainder += projectedPixels * worldUnitsPerPixel;
            const float steps = std::trunc(m_moveSnapRemainder / m_moveSnapStep);
            if (steps == 0.0f)
            {
                break;
            }

            const float amount = steps * m_moveSnapStep;
            m_moveSnapRemainder -= amount;
            for (Scene::GameObject* object : targets)
            {
                if (!object)
                {
                    continue;
                }

                Math::Transform& transform = object->GetTransform();
                transform.position.x += axisVector.x * amount;
                transform.position.y += axisVector.y * amount;
                transform.position.z += axisVector.z * amount;
            }
            break;
        }
        case GizmoMode::Rotate:
        {
            if (!m_gizmoRotateDragValid)
            {
                break;
            }

            float currentScreenAngle = 0.0f;
            if (!GetScreenCircleAngle(
                m_gizmoDragScreenCenterX,
                m_gizmoDragScreenCenterY,
                static_cast<float>(input.GetMouseX()),
                static_cast<float>(input.GetMouseY()),
                currentScreenAngle))
            {
                break;
            }

            float deltaAngle = NormalizeSignedAngle(currentScreenAngle - m_gizmoDragStartScreenAngle);
            if (!std::isfinite(deltaAngle))
            {
                break;
            }

            if (m_rotateSnapStep > 0.0001f)
            {
                deltaAngle = std::round(deltaAngle / m_rotateSnapStep) * m_rotateSnapStep;
            }

            const int axisIndex = GetAxisIndex(m_activeGizmoAxis);
            for (const std::pair<Scene::GameObject*, Math::Transform>& entry : m_gizmoDragStartTransforms)
            {
                Scene::GameObject* object = entry.first;
                if (!object)
                {
                    continue;
                }

                Math::Transform transform = entry.second;
                transform.rotation = ApplyRotationAroundDisplayedRing(entry.second.rotation, m_gizmoDragAxis, deltaAngle);
                if (!IsFiniteRotation(transform.rotation))
                {
                    continue;
                }

                object->GetTransform().rotation = transform.rotation;
            }

            if (m_debugRotationGizmo)
            {
                Core::Log::Info("Rotate gizmo delta(rad)=" + std::to_string(deltaAngle) + " axis=" + std::to_string(axisIndex));
            }
            break;
        }
        case GizmoMode::Scale:
        {
            m_scaleSnapRemainder += projectedPixels * 0.01f;
            const float steps = std::trunc(m_scaleSnapRemainder / m_scaleSnapStep);
            if (steps == 0.0f)
            {
                break;
            }

            const float amount = steps * m_scaleSnapStep;
            m_scaleSnapRemainder -= amount;
            for (Scene::GameObject* object : targets)
            {
                if (!object)
                {
                    continue;
                }

                Math::Transform& transform = object->GetTransform();
                const float currentValue = GetAxisComponent(transform.scale, m_activeGizmoAxis);
                SetAxisComponent(transform.scale, m_activeGizmoAxis, std::max(0.01f, currentValue + amount));
            }
            break;
        }
        }
    }

    void EditorLayer::RefreshAssets()
    {
        m_modelAssets.clear();
        m_textureAssets.clear();
        m_materialAssets.clear();
        m_sceneAssets.clear();
        m_assetDatabase.ScanAssets("Assets");

        for (const auto& [pathKey, meta] : m_assetDatabase.GetAssets())
        {
            const std::wstring assetPath = meta.sourcePath.generic_wstring();
            switch (meta.type)
            {
            case Asset::AssetType::Mesh:
                m_modelAssets.push_back(assetPath);
                break;
            case Asset::AssetType::Texture:
                m_textureAssets.push_back(assetPath);
                break;
            case Asset::AssetType::Material:
                m_materialAssets.push_back(assetPath);
                break;
            case Asset::AssetType::Scene:
                m_sceneAssets.push_back(assetPath);
                break;
            default:
                break;
            }
        }

        if (!m_selectedMaterialAsset.empty()
            && std::find(m_materialAssets.begin(), m_materialAssets.end(), m_selectedMaterialAsset) == m_materialAssets.end())
        {
            m_selectedMaterialAsset.clear();
        }

        if (!m_selectedSceneAsset.empty()
            && std::find(m_sceneAssets.begin(), m_sceneAssets.end(), m_selectedSceneAsset) == m_sceneAssets.end())
        {
            m_selectedSceneAsset.clear();
        }

        const bool selectedAssetExists = std::find(m_modelAssets.begin(), m_modelAssets.end(), m_selectedAsset) != m_modelAssets.end()
            || std::find(m_textureAssets.begin(), m_textureAssets.end(), m_selectedAsset) != m_textureAssets.end()
            || std::find(m_materialAssets.begin(), m_materialAssets.end(), m_selectedAsset) != m_materialAssets.end()
            || std::find(m_sceneAssets.begin(), m_sceneAssets.end(), m_selectedAsset) != m_sceneAssets.end();
        if (!m_selectedAsset.empty() && !selectedAssetExists)
        {
            m_selectedAsset.clear();
        }
        m_selectedAssets.erase(std::remove_if(m_selectedAssets.begin(), m_selectedAssets.end(), [&](const std::wstring& asset)
        {
            return std::find(m_modelAssets.begin(), m_modelAssets.end(), asset) == m_modelAssets.end()
                && std::find(m_textureAssets.begin(), m_textureAssets.end(), asset) == m_textureAssets.end()
                && std::find(m_materialAssets.begin(), m_materialAssets.end(), asset) == m_materialAssets.end()
                && std::find(m_sceneAssets.begin(), m_sceneAssets.end(), asset) == m_sceneAssets.end();
        }), m_selectedAssets.end());

        if (!m_renamingAsset.empty() && !std::filesystem::exists(std::filesystem::path(m_renamingAsset)))
        {
            m_renamingAsset.clear();
            m_focusAssetRenameInput = false;
        }

        if (m_selectedAssetFolder.empty() || !std::filesystem::exists(std::filesystem::path(m_selectedAssetFolder)))
        {
            m_selectedAssetFolder = L"Assets";
        }
    }

    void EditorLayer::RenderObjectTools(Scene::Scene& scene, Renderer::Renderer& renderer)
    {
        constexpr ImGuiWindowFlags toolbarFlags = ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoScrollbar;
        ImGui::Begin("Main Toolbar", nullptr, toolbarFlags);
        if (ImGui::Button("Create Empty"))
        {
            GameObjectSnapshot snapshot;
            snapshot.name = "GameObject";
            auto command = std::make_unique<CreateGameObjectCommand>(scene, snapshot);
            CreateGameObjectCommand* commandPtr = command.get();
            m_commandManager->ExecuteCommand(std::move(command));
            SetSingleSelectedObject(commandPtr->GetCreatedObject());
        }
        ImGui::SameLine();
        auto createPrimitiveObject = [&](PrimitiveShape shape)
        {
            GameObjectSnapshot snapshot;
            ConfigurePrimitiveSnapshot(snapshot, renderer.GetResourceManager(), shape);
            auto command = std::make_unique<CreateGameObjectCommand>(scene, snapshot);
            CreateGameObjectCommand* commandPtr = command.get();
            m_commandManager->ExecuteCommand(std::move(command));
            SetSingleSelectedObject(commandPtr->GetCreatedObject());
        };

        if (ImGui::Button("Shapes"))
        {
            ImGui::OpenPopup("CreateShapeMenu");
        }
        if (ImGui::BeginPopup("CreateShapeMenu"))
        {
            if (ImGui::MenuItem("Cube"))
            {
                createPrimitiveObject(PrimitiveShape::Cube);
            }
            if (ImGui::MenuItem("Sphere"))
            {
                createPrimitiveObject(PrimitiveShape::Sphere);
            }
            if (ImGui::MenuItem("Cylinder"))
            {
                createPrimitiveObject(PrimitiveShape::Cylinder);
            }
            if (ImGui::MenuItem("Cone"))
            {
                createPrimitiveObject(PrimitiveShape::Cone);
            }
            if (ImGui::MenuItem("Plane"))
            {
                createPrimitiveObject(PrimitiveShape::Plane);
            }
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Create Post Process"))
        {
            GameObjectSnapshot snapshot;
            snapshot.name = "PostProcess";
            snapshot.postProcess.enabledComponent = true;
            snapshot.postProcess.enabled = true;
            snapshot.postProcess.vignetteEnabled = true;
            auto command = std::make_unique<CreateGameObjectCommand>(scene, snapshot);
            CreateGameObjectCommand* commandPtr = command.get();
            m_commandManager->ExecuteCommand(std::move(command));
            SetSingleSelectedObject(commandPtr->GetCreatedObject());
        }
        ImGui::SameLine();
        if (ImGui::Button("Create Player Start"))
        {
            GameObjectSnapshot snapshot;
            snapshot.name = "PlayerStart";
            snapshot.transform.position = { 0.0f, 0.0f, -4.0f };
            snapshot.playerStart.enabled = true;
            auto command = std::make_unique<CreateGameObjectCommand>(scene, snapshot);
            CreateGameObjectCommand* commandPtr = command.get();
            m_commandManager->ExecuteCommand(std::move(command));
            SetSingleSelectedObject(commandPtr->GetCreatedObject());
        }
        if (m_selectedObject)
        {
            ImGui::SameLine();
            if (ImGui::Button("Delete Selected"))
            {
                DeleteSelectedObjects(scene);
            }
        }

        if (m_showGizmoTools)
        {
            ImGui::SeparatorText("Gizmo Prep");
            ImGui::TextUnformatted("Mode (W Move / R Rotate / S Scale)");
            ImGui::SameLine();
            if (ImGui::RadioButton("Move##GizmoMove", m_gizmoMode == GizmoMode::Move))
            {
                m_gizmoMode = GizmoMode::Move;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Rotate##GizmoRotate", m_gizmoMode == GizmoMode::Rotate))
            {
                m_gizmoMode = GizmoMode::Rotate;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Scale##GizmoScale", m_gizmoMode == GizmoMode::Scale))
            {
                m_gizmoMode = GizmoMode::Scale;
            }
            ImGui::TextUnformatted("Drag the visible X/Y/Z axis in the scene view.");
        }
        ImGui::End();
    }

    void EditorLayer::RenderAssetBrowser(Scene::Scene& scene, Renderer::Renderer& renderer)
    {
        if (!m_showAssetBrowser)
        {
            return;
        }

        renderer.GetResourceManager().SetAssetDatabase(&m_assetDatabase);
        constexpr ImGuiWindowFlags assetBrowserFlags = ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoCollapse;
        ImGui::Begin("Content Browser", &m_showAssetBrowser, assetBrowserFlags);
        if (ImGui::Button("Refresh"))
        {
            RefreshAssets();
        }
        ImGui::SameLine();
        if (ImGui::Button("Import Model"))
        {
            ImportModelAsset();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save Scene"))
        {
            SaveCurrentSceneAs(scene, m_currentSceneAsset.empty() ? MakeAssetPathInFolder(m_selectedAssetFolder, m_newSceneNameBuffer.data(), ".scene", "Scene") : m_currentSceneAsset);
        }

        Resource::ResourceManager& resources = renderer.GetResourceManager();
        const std::string currentSceneText = m_currentSceneAsset.empty() ? "(unsaved scene)" : ToNarrowAscii(m_currentSceneAsset);
        ImGui::Text("Current Scene: %s", currentSceneText.c_str());

        ImGui::InputText("Scene Name", m_newSceneNameBuffer.data(), m_newSceneNameBuffer.size());
        ImGui::SameLine();
        if (ImGui::Button("Create Scene"))
        {
            CreateSceneAsset(scene);
        }

        ImGui::InputText("Material Name", m_newMaterialNameBuffer.data(), m_newMaterialNameBuffer.size());
        ImGui::SameLine();
        if (ImGui::Button("Create Material"))
        {
            CreateMaterialAsset(renderer);
        }
        const bool hasSelectedAsset = !m_selectedAsset.empty() || !m_selectedAssets.empty();
        if (hasSelectedAsset)
        {
            ImGui::SameLine();
            const bool deleteKeyPressed = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)
                && !ImGui::GetIO().WantTextInput
                && ImGui::IsKeyPressed(ImGuiKey_Delete);
            if (ImGui::Button("Delete Asset") || deleteKeyPressed)
            {
                DeleteSelectedAssets(scene, renderer);
            }
        }
        if (!m_selectedAsset.empty()
            && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)
            && !ImGui::GetIO().WantTextInput
            && ImGui::IsKeyPressed(ImGuiKey_F2))
        {
            BeginRenameSelectedAsset();
        }
        ImGui::Separator();

        std::vector<std::wstring> allAssets;
        allAssets.reserve(m_modelAssets.size() + m_textureAssets.size() + m_materialAssets.size() + m_sceneAssets.size());
        allAssets.insert(allAssets.end(), m_modelAssets.begin(), m_modelAssets.end());
        allAssets.insert(allAssets.end(), m_textureAssets.begin(), m_textureAssets.end());
        allAssets.insert(allAssets.end(), m_materialAssets.begin(), m_materialAssets.end());
        allAssets.insert(allAssets.end(), m_sceneAssets.begin(), m_sceneAssets.end());
        std::sort(allAssets.begin(), allAssets.end());

        auto applyAsset = [&](const std::wstring& asset)
        {
            if (IsKnownAssetPath(asset))
            {
                SetSingleSelectedAsset(asset);
            }

            const std::filesystem::path path(asset);
            if (HasExtension(path, { L".obj" }))
            {
                std::vector<Scene::GameObject*> targets = m_selectedObjects;
                if (targets.empty() && m_selectedObject)
                {
                    targets.push_back(m_selectedObject);
                }
                if (targets.empty())
                {
                    Scene::GameObject& object = scene.CreateGameObject(path.stem().string());
                    targets.push_back(&object);
                    SetSingleSelectedObject(&object);
                }

                std::shared_ptr<Renderer::Mesh> mesh = resources.LoadMesh(asset);
                if (!mesh)
                {
                    Core::Log::Error("Failed to load model asset: " + ToNarrowAscii(asset));
                    ShowNotification("Failed to load model asset.");
                    return;
                }
                const std::string meshGuid = m_assetDatabase.GetGuidFromPath(asset);

                for (Scene::GameObject* object : targets)
                {
                    if (!object)
                    {
                        continue;
                    }

                    Scene::MeshRendererComponent* meshRenderer = object->GetComponent<Scene::MeshRendererComponent>();
                    if (!meshRenderer)
                    {
                        meshRenderer = &object->AddComponent<Scene::MeshRendererComponent>();
                    }
                    meshRenderer->SetMesh(mesh);
                    meshRenderer->SetMeshPath(asset);
                    meshRenderer->SetMeshGuid(meshGuid);
                    if (!meshRenderer->GetMaterial())
                    {
                        meshRenderer->SetMaterial(resources.CreateMaterial("editor:material:asset_default:" + object->GetName(), { 1.0f, 1.0f, 1.0f, 1.0f }));
                        meshRenderer->SetMaterialPath(L"editor:material:asset_default");
                    }
                }
                ShowNotification("Placed model: " + path.filename().string());
                return;
            }

            if (HasExtension(path, { L".mat" }))
            {
                m_selectedMaterialAsset = asset;
                ApplyMaterialToSelectedObjects(renderer, asset);
                return;
            }

            if (HasExtension(path, { L".png", L".jpg", L".jpeg", L".bmp", L".tga" }))
            {
                ApplyTextureToSelectedMaterial(renderer, asset);
                return;
            }

            if (HasExtension(path, { L".scene" }))
            {
                RequestOpenScene(scene, renderer, asset);
            }
        };

        auto drawFolderNode = [&](auto&& self, const std::filesystem::path& folder) -> void
        {
            const std::wstring folderPath = folder.generic_wstring();
            const bool selected = m_selectedAssetFolder == folderPath;
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            if (selected)
            {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            bool hasChildDirectory = false;
            if (std::filesystem::exists(folder))
            {
                for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(folder))
                {
                    if (entry.is_directory())
                    {
                        hasChildDirectory = true;
                        break;
                    }
                }
            }
            if (!hasChildDirectory)
            {
                flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            }

            const std::string label = ToNarrowAscii(folder.filename().empty() ? folder.generic_wstring() : folder.filename().wstring());
            const bool open = ImGui::TreeNodeEx(label.c_str(), flags);
            if (ImGui::IsItemClicked())
            {
                m_selectedAssetFolder = folderPath;
            }

            if (open && hasChildDirectory)
            {
                std::vector<std::filesystem::path> childFolders;
                for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(folder))
                {
                    if (entry.is_directory())
                    {
                        childFolders.push_back(entry.path());
                    }
                }
                std::sort(childFolders.begin(), childFolders.end());
                for (const std::filesystem::path& child : childFolders)
                {
                    self(self, child);
                }
                ImGui::TreePop();
            }
        };

        if (ImGui::BeginTable("AssetBrowserLayout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("Folders", ImGuiTableColumnFlags_WidthFixed, 220.0f);
            ImGui::TableSetupColumn("Assets", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            const float browserChildHeight = std::max(80.0f, ImGui::GetContentRegionAvail().y);
            ImGui::BeginChild("AssetFolders", ImVec2(0.0f, browserChildHeight), false);
            drawFolderNode(drawFolderNode, std::filesystem::path("Assets"));
            ImGui::EndChild();

            ImGui::TableSetColumnIndex(1);
            ImGui::BeginChild("AssetGrid", ImVec2(0.0f, browserChildHeight), false);
            const std::string folderLabel = ToNarrowAscii(m_selectedAssetFolder);
            ImGui::TextUnformatted(folderLabel.c_str());
            ImGui::Separator();
            if (ImGui::BeginPopupContextWindow("AssetBrowserCreateMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
            {
                ImGui::TextDisabled("Create in selected folder");
                ImGui::Separator();
                if (ImGui::MenuItem("Scene"))
                {
                    CreateSceneAsset(scene);
                }
                if (ImGui::MenuItem("Material"))
                {
                    CreateMaterialAsset(renderer);
                }
                if (ImGui::MenuItem("Import Model"))
                {
                    ImportModelAsset();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Save Current Scene Here"))
                {
                    SaveCurrentSceneAs(scene, MakeAssetPathInFolder(m_selectedAssetFolder, m_newSceneNameBuffer.data(), ".scene", "Scene"));
                }
                ImGui::EndPopup();
            }

            std::vector<std::wstring> visibleAssets;
            for (const std::wstring& asset : allAssets)
            {
                if (IsAssetInFolder(asset, m_selectedAssetFolder))
                {
                    visibleAssets.push_back(asset);
                }
            }

            constexpr float tileWidth = 118.0f;
            constexpr float tileHeight = 132.0f;
            const float availableWidth = std::max(tileWidth, ImGui::GetContentRegionAvail().x);
            const int columnCount = std::max(1, static_cast<int>(availableWidth / tileWidth));
            if (ImGui::BeginTable("AssetGridTable", columnCount, ImGuiTableFlags_SizingFixedFit))
            {
                for (std::size_t i = 0; i < visibleAssets.size(); ++i)
                {
                    const std::wstring& asset = visibleAssets[i];
                    const std::filesystem::path path(asset);
                    ImGui::TableNextColumn();
                    ImGui::PushID(static_cast<int>(i));
                    const bool selected = IsAssetSelected(asset);
                    if (selected)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Header));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
                    }

                    if (m_renamingAsset == asset)
                    {
                        if (m_focusAssetRenameInput)
                        {
                            ImGui::SetKeyboardFocusHere();
                            m_focusAssetRenameInput = false;
                        }

                        ImGui::TextUnformatted(GetAssetTypeLabel(asset));
                        const bool enterPressed = ImGui::InputText("##AssetRename", m_assetRenameBuffer.data(), m_assetRenameBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
                        const bool escapePressed = ImGui::IsKeyPressed(ImGuiKey_Escape);
                        if (enterPressed)
                        {
                            CommitAssetRename(scene, renderer);
                        }
                        else if (escapePressed)
                        {
                            m_renamingAsset.clear();
                            m_focusAssetRenameInput = false;
                        }
                        else if (ImGui::IsItemDeactivatedAfterEdit())
                        {
                            CommitAssetRename(scene, renderer);
                        }
                        else if (ImGui::IsItemDeactivated())
                        {
                            m_renamingAsset.clear();
                            m_focusAssetRenameInput = false;
                        }
                    }
                    else
                    {
                        const Asset::AssetMeta* meta = m_assetDatabase.GetMetaFromPath(path);
                        const std::string guidPrefix = meta && !meta->guid.empty()
                            ? meta->guid.substr(0, std::min<std::size_t>(8, meta->guid.size()))
                            : "no-guid";
                        const std::string typeLabel = meta ? Asset::AssetTypeToString(meta->type) : GetAssetTypeLabel(asset);
                        const bool isTextureAsset = HasExtension(path, { L".png", L".jpg", L".jpeg", L".bmp", L".tga" });
                        const char* assetPayloadType = GetAssetDragPayloadType(path);
                        auto handleAssetClick = [&]()
                        {
                            if (ImGui::GetIO().KeyShift)
                            {
                                ToggleSelectedAsset(asset);
                                if (HasExtension(path, { L".mat" }))
                                {
                                    m_selectedMaterialAsset = asset;
                                }
                                if (HasExtension(path, { L".scene" }))
                                {
                                    m_selectedSceneAsset = asset;
                                }
                            }
                            else
                            {
                                applyAsset(asset);
                            }
                        };

                        bool assetItemHovered = false;
                        if (isTextureAsset)
                        {
                            std::shared_ptr<RHI::RHITexture> texture = resources.LoadTexture(asset);
                            void* textureHandle = texture ? texture->GetNativeShaderResourceHandleForUI() : nullptr;
                            if (textureHandle)
                            {
                                const ImVec2 imageSize(72.0f, 72.0f);
                                ImGui::Image(MakeExternalTextureRef(textureHandle), imageSize);
                                assetItemHovered = assetItemHovered || ImGui::IsItemHovered();
                                if (selected)
                                {
                                    const ImVec2 min = ImGui::GetItemRectMin();
                                    const ImVec2 max = ImGui::GetItemRectMax();
                                    ImGui::GetWindowDrawList()->AddRect(min, max, ImGui::GetColorU32(ImGuiCol_HeaderActive), 0.0f, 0, 2.0f);
                                }
                                if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                                {
                                    handleAssetClick();
                                }
                                EmitAssetDragDropSource(assetPayloadType, asset);
                            }
                            else
                            {
                                if (ImGui::Button("TEX", ImVec2(72.0f, 72.0f)))
                                {
                                    handleAssetClick();
                                }
                                assetItemHovered = assetItemHovered || ImGui::IsItemHovered();
                                EmitAssetDragDropSource(assetPayloadType, asset);
                            }

                            std::string buttonText = typeLabel + "\n" + GetAssetFileName(asset);
                            if (ImGui::Button(buttonText.c_str(), ImVec2(tileWidth - 10.0f, 48.0f)))
                            {
                                handleAssetClick();
                            }
                            assetItemHovered = assetItemHovered || ImGui::IsItemHovered();
                            EmitAssetDragDropSource(assetPayloadType, asset);
                        }
                        else
                        {
                            std::string buttonText = typeLabel + "\n" + GetAssetFileName(asset) + "\n" + guidPrefix;
                            if (ImGui::Button(buttonText.c_str(), ImVec2(tileWidth - 10.0f, tileHeight)))
                            {
                                handleAssetClick();
                            }
                            assetItemHovered = ImGui::IsItemHovered();
                            EmitAssetDragDropSource(assetPayloadType, asset);
                        }
                        if (HasExtension(path, { L".obj", L".gltf", L".glb", L".fbx" })
                            && assetItemHovered
                            && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                        {
                            const Asset::AssetMeta* meta = m_assetDatabase.GetMetaFromPath(path);
                            if (meta && meta->type == Asset::AssetType::Mesh)
                            {
                                m_staticMeshEditor.OpenByGuid(meta->guid, m_assetDatabase, renderer.GetResourceManager());
                            }
                            else
                            {
                                m_staticMeshEditor.OpenByPath(asset, m_assetDatabase, renderer.GetResourceManager());
                            }
                        }
                        if (HasExtension(path, { L".mat" })
                            && assetItemHovered
                            && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                        {
                            OpenMaterialEditor(renderer, asset);
                        }
                        if (assetItemHovered && meta)
                        {
                            ImGui::BeginTooltip();
                            ImGui::Text("Type: %s", Asset::AssetTypeToString(meta->type));
                            ImGui::Text("GUID: %s", meta->guid.c_str());
                            const std::string metaPathText = ToNarrowAscii(meta->sourcePath.generic_wstring());
                            ImGui::Text("Path: %s", metaPathText.c_str());
                            ImGui::EndTooltip();
                        }
                        if (ImGui::BeginPopupContextItem("AssetGuidContext"))
                        {
                            if (ImGui::MenuItem("Rename"))
                            {
                                SetSingleSelectedAsset(asset);
                                BeginRenameSelectedAsset();
                            }
                            if (HasExtension(path, { L".mat" }) && ImGui::MenuItem("Open Material Editor"))
                            {
                                OpenMaterialEditor(renderer, asset);
                            }
                            if (ImGui::MenuItem("Delete"))
                            {
                                SetSingleSelectedAsset(asset);
                                DeleteSelectedAssets(scene, renderer);
                            }
                            ImGui::Separator();
                            if (meta)
                            {
                                ImGui::Text("Type: %s", Asset::AssetTypeToString(meta->type));
                                ImGui::Text("GUID: %s", meta->guid.c_str());
                                if (ImGui::MenuItem("Copy GUID"))
                                {
                                    ImGui::SetClipboardText(meta->guid.c_str());
                                    ShowNotification("Copied asset GUID.");
                                }
                            }
                            else
                            {
                                ImGui::TextDisabled("No meta data.");
                            }
                            ImGui::EndPopup();
                        }
                    }

                    if (selected)
                    {
                        ImGui::PopStyleColor(2);
                    }

                    ImGui::PopID();
                }
                ImGui::EndTable();
            }

            if (visibleAssets.empty())
            {
                ImGui::TextDisabled("No assets in this folder.");
            }
            ImGui::EndChild();
            ImGui::EndTable();
        }

        ImGui::End();
    }
}
