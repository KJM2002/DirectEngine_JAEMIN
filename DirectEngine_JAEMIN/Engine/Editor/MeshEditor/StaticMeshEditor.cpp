#include "StaticMeshEditor.h"

#include "Engine/Asset/AssetDatabase.h"
#include "Engine/Asset/AssetType.h"
#include "Engine/Core/Log.h"
#include "Engine/Resource/ResourceManager.h"
#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace Engine::Editor
{
    namespace
    {
        std::string ToNarrowAscii(const std::filesystem::path& path)
        {
            const std::wstring value = path.generic_wstring();
            std::string result;
            result.reserve(value.size());
            for (wchar_t c : value)
            {
                result.push_back(c <= 0x7f ? static_cast<char>(c) : '?');
            }
            return result;
        }

        void DrawVector3(const char* label, const Math::Vector3& value)
        {
            ImGui::Text("%s: %.3f, %.3f, %.3f", label, value.x, value.y, value.z);
        }

        std::string Trim(const std::string& value)
        {
            const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c) != 0; });
            const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) { return std::isspace(c) != 0; }).base();
            return first >= last ? std::string() : std::string(first, last);
        }

        const char* ToColliderTypeName(Physics::ColliderType type)
        {
            return type == Physics::ColliderType::Sphere ? "Sphere" : "AABB";
        }

        bool DragFloat3(const char* label, Math::Vector3& value, float speed = 0.05f, float minValue = -1000.0f, float maxValue = 1000.0f)
        {
            float values[3] = { value.x, value.y, value.z };
            const bool changed = ImGui::DragFloat3(label, values, speed, minValue, maxValue, "%.3f");
            if (changed)
            {
                value = { values[0], values[1], values[2] };
            }
            return changed;
        }
    }

    void StaticMeshEditor::OpenEmpty()
    {
        m_isOpen = true;
        m_mesh.reset();
        m_meshGuid.clear();
        m_meshPath.clear();
        m_colliders.clear();
        m_selectedColliderIndex = -1;
        m_errorText.clear();
        m_previewViewport.SetMesh(nullptr);
        SyncCollidersToPreview();
    }

    bool StaticMeshEditor::OpenByGuid(const std::string& meshGuid, const Asset::AssetDatabase& assetDatabase, Resource::ResourceManager& resourceManager)
    {
        m_isOpen = true;
        m_errorText.clear();
        m_meshGuid = meshGuid;
        m_meshPath = assetDatabase.GetPathFromGuid(meshGuid);
        if (m_meshPath.empty())
        {
            SetError("Failed to open Static Mesh. GUID path lookup failed.");
            return false;
        }
        if (assetDatabase.GetAssetTypeFromGuid(meshGuid) != Asset::AssetType::Mesh)
        {
            SetError("Failed to open Static Mesh. Asset type is not Mesh.");
            return false;
        }

        m_mesh = resourceManager.LoadMeshByGuid(meshGuid);
        if (!m_mesh)
        {
            SetError("Failed to open Static Mesh. Mesh load failed.");
            return false;
        }

        m_previewViewport.SetMesh(m_mesh);
        LoadColliderAsset();
        SyncCollidersToPreview();
        Core::Log::Info("Opened Static Mesh Editor: " + ToNarrowAscii(m_meshPath));
        return true;
    }

    bool StaticMeshEditor::OpenByPath(const std::filesystem::path& meshPath, const Asset::AssetDatabase& assetDatabase, Resource::ResourceManager& resourceManager)
    {
        m_isOpen = true;
        m_errorText.clear();
        m_meshPath = meshPath;
        m_meshGuid = assetDatabase.GetGuidFromPath(meshPath);

        if (!m_meshGuid.empty())
        {
            return OpenByGuid(m_meshGuid, assetDatabase, resourceManager);
        }

        m_mesh = resourceManager.LoadMesh(meshPath.generic_wstring());
        if (!m_mesh)
        {
            SetError("Failed to open Static Mesh. Mesh load failed.");
            return false;
        }

        m_previewViewport.SetMesh(m_mesh);
        LoadColliderAsset();
        SyncCollidersToPreview();
        Core::Log::Info("Opened Static Mesh Editor by path: " + ToNarrowAscii(m_meshPath));
        return true;
    }

    void StaticMeshEditor::Close()
    {
        m_isOpen = false;
    }

    bool StaticMeshEditor::IsOpen() const
    {
        return m_isOpen;
    }

    void StaticMeshEditor::OnUpdate(float deltaTime)
    {
        m_previewViewport.Update(deltaTime);
    }

    void StaticMeshEditor::OnImGuiRender()
    {
        if (!m_isOpen)
        {
            return;
        }

        ImGui::SetNextWindowSize(ImVec2(980.0f, 680.0f), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Static Mesh Editor", &m_isOpen))
        {
            ImGui::End();
            return;
        }

        DrawToolbar();
        ImGui::Separator();

        if (!m_errorText.empty())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.25f, 1.0f), "%s", m_errorText.c_str());
            ImGui::Separator();
        }

        ImGui::Columns(2, "StaticMeshEditorColumns", true);
        ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() - 330.0f);
        m_previewViewport.DrawImGuiViewport();
        ImGui::NextColumn();
        DrawDetailsPanel();
        ImGui::Separator();
        DrawMaterialSlotsPanel();
        ImGui::Separator();
        DrawColliderPanel();
        ImGui::Columns(1);
        ImGui::End();
    }

    void StaticMeshEditor::DrawToolbar()
    {
        ImGui::BeginDisabled();
        ImGui::Button("Save");
        ImGui::SameLine();
        ImGui::Button("Reimport");
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Frame Mesh"))
        {
            m_previewViewport.FrameMesh();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Camera"))
        {
            m_previewViewport.ResetCamera();
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Grid", &m_showGrid))
        {
            m_previewViewport.SetShowGrid(m_showGrid);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Axis", &m_showAxis))
        {
            m_previewViewport.SetShowAxis(m_showAxis);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Bounds", &m_showBounds))
        {
            m_previewViewport.SetShowBounds(m_showBounds);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Collider", &m_showCollider))
        {
            m_previewViewport.SetShowCollider(m_showCollider);
        }
    }

    void StaticMeshEditor::DrawDetailsPanel()
    {
        ImGui::SeparatorText("Asset");
        ImGui::Text("Static Mesh: %s", m_meshPath.empty() ? "(none)" : m_meshPath.filename().string().c_str());
        ImGui::Text("GUID: %s", m_meshGuid.empty() ? "(none)" : m_meshGuid.c_str());
        ImGui::TextWrapped("Path: %s", m_meshPath.empty() ? "(none)" : ToNarrowAscii(m_meshPath).c_str());

        ImGui::SeparatorText("Mesh Info");
        if (!m_mesh)
        {
            ImGui::TextUnformatted("No Static Mesh selected.");
            ImGui::TextUnformatted("Select a mesh asset from the Asset Browser.");
            return;
        }

        ImGui::Text("Vertices: %u", m_mesh->GetVertexCount());
        ImGui::Text("Indices: %u", m_mesh->GetIndexCount());
        ImGui::Text("Triangles: %u", m_mesh->GetTriangleCount());
        ImGui::Text("SubMeshes: %u", m_mesh->GetSubMeshCount());
        ImGui::Text("Material Slots: %u", static_cast<unsigned>(m_mesh->GetMaterialSlots().size()));

        ImGui::SeparatorText("Bounds");
        const Renderer::BoundingBox& bounds = m_mesh->GetBounds();
        if (bounds.IsValid())
        {
            DrawVector3("Min", bounds.min);
            DrawVector3("Max", bounds.max);
            DrawVector3("Center", bounds.GetCenter());
            DrawVector3("Extents", bounds.GetExtents());
            DrawVector3("Size", bounds.GetSize());
        }
        else
        {
            ImGui::TextUnformatted("Bounds unavailable.");
        }

        ImGui::SeparatorText("Import");
        ImGui::BeginDisabled();
        float importScale = 1.0f;
        ImGui::DragFloat("Import Scale", &importScale, 0.01f, 0.001f, 100.0f);
        ImGui::EndDisabled();
    }

    void StaticMeshEditor::DrawMaterialSlotsPanel()
    {
        ImGui::SeparatorText("Material Slots");
        if (!m_mesh)
        {
            ImGui::TextUnformatted("No material slots.");
            return;
        }

        const std::vector<Renderer::MaterialSlot>& slots = m_mesh->GetMaterialSlots();
        for (std::size_t i = 0; i < slots.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            ImGui::Text("Slot %u", static_cast<unsigned>(i));
            ImGui::Text("Name: %s", slots[i].name.c_str());
            ImGui::Text("Material: %s", slots[i].materialGuid.empty() ? "None" : slots[i].materialGuid.c_str());
            ImGui::PopID();
        }

        if (m_showCollider)
        {
            ImGui::SeparatorText("Collider Preview");
            ImGui::Text("Collider Count: %u", static_cast<unsigned>(m_colliders.size()));
        }
    }

    void StaticMeshEditor::DrawColliderPanel()
    {
        ImGui::SeparatorText("Colliders");
        bool changed = false;
        if (ImGui::Button("Add Box"))
        {
            StaticMeshPreviewViewport::ColliderPreview collider;
            collider.type = Physics::ColliderType::AABB;
            if (m_mesh && m_mesh->GetBounds().IsValid())
            {
                collider.center = m_mesh->GetBounds().GetCenter();
                collider.size = m_mesh->GetBounds().GetSize();
            }
            m_colliders.push_back(collider);
            m_selectedColliderIndex = static_cast<int>(m_colliders.size()) - 1;
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Sphere"))
        {
            StaticMeshPreviewViewport::ColliderPreview collider;
            collider.type = Physics::ColliderType::Sphere;
            if (m_mesh && m_mesh->GetBounds().IsValid())
            {
                const Math::Vector3 extents = m_mesh->GetBounds().GetExtents();
                collider.center = m_mesh->GetBounds().GetCenter();
                collider.radius = std::max({ extents.x, extents.y, extents.z, 0.5f });
            }
            m_colliders.push_back(collider);
            m_selectedColliderIndex = static_cast<int>(m_colliders.size()) - 1;
            changed = true;
        }
        ImGui::SameLine();
        if (m_selectedColliderIndex >= 0 && ImGui::Button("Delete Collider"))
        {
            const std::size_t index = static_cast<std::size_t>(m_selectedColliderIndex);
            if (index < m_colliders.size())
            {
                m_colliders.erase(m_colliders.begin() + index);
                m_selectedColliderIndex = m_colliders.empty()
                    ? -1
                    : std::clamp(m_selectedColliderIndex, 0, static_cast<int>(m_colliders.size()) - 1);
                changed = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Save Colliders"))
        {
            if (SaveColliderAsset())
            {
                Core::Log::Info("Saved static mesh colliders: " + ToNarrowAscii(m_meshPath));
            }
        }

        if (m_colliders.empty())
        {
            ImGui::TextDisabled("No collider data. Add Box or Sphere to create a reusable setup.");
            return;
        }

        if (ImGui::BeginTable("StaticMeshColliderTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("List", ImGuiTableColumnFlags_WidthFixed, 110.0f);
            ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            for (std::size_t i = 0; i < m_colliders.size(); ++i)
            {
                ImGui::PushID(static_cast<int>(i));
                const std::string label = std::string(ToColliderTypeName(m_colliders[i].type)) + " " + std::to_string(i);
                if (ImGui::Selectable(label.c_str(), m_selectedColliderIndex == static_cast<int>(i)))
                {
                    m_selectedColliderIndex = static_cast<int>(i);
                }
                ImGui::PopID();
            }

            ImGui::TableSetColumnIndex(1);
            if (m_selectedColliderIndex >= 0 && static_cast<std::size_t>(m_selectedColliderIndex) < m_colliders.size())
            {
                StaticMeshPreviewViewport::ColliderPreview& collider = m_colliders[static_cast<std::size_t>(m_selectedColliderIndex)];
                const char* colliderTypes[] = { "AABB", "Sphere" };
                int currentType = collider.type == Physics::ColliderType::Sphere ? 1 : 0;
                if (ImGui::Combo("Type", &currentType, colliderTypes, IM_ARRAYSIZE(colliderTypes)))
                {
                    collider.type = currentType == 1 ? Physics::ColliderType::Sphere : Physics::ColliderType::AABB;
                    changed = true;
                }
                changed = DragFloat3("Center", collider.center) || changed;
                changed = DragFloat3("Size", collider.size, 0.05f, 0.01f, 1000.0f) || changed;
                changed = ImGui::DragFloat("Radius", &collider.radius, 0.05f, 0.01f, 1000.0f, "%.3f") || changed;
                changed = ImGui::Checkbox("Is Trigger", &collider.isTrigger) || changed;
                collider.size.x = std::clamp(collider.size.x, 0.01f, 1000.0f);
                collider.size.y = std::clamp(collider.size.y, 0.01f, 1000.0f);
                collider.size.z = std::clamp(collider.size.z, 0.01f, 1000.0f);
                collider.radius = std::clamp(collider.radius, 0.01f, 1000.0f);
            }
            ImGui::EndTable();
        }

        if (changed)
        {
            m_showCollider = true;
            m_previewViewport.SetShowCollider(true);
            SyncCollidersToPreview();
        }
    }

    bool StaticMeshEditor::LoadColliderAsset()
    {
        m_colliders.clear();
        m_selectedColliderIndex = -1;
        if (m_meshPath.empty())
        {
            return false;
        }

        const std::filesystem::path colliderPath = std::filesystem::path(m_meshPath).concat(L".colliders");
        std::ifstream file(colliderPath);
        if (!file.is_open())
        {
            return false;
        }

        StaticMeshPreviewViewport::ColliderPreview current;
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
                    m_colliders.push_back(current);
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
                Core::Log::Warning("Skipped malformed static mesh collider property: " + line);
            }
        }

        if (hasCurrent)
        {
            m_colliders.push_back(current);
        }
        m_selectedColliderIndex = m_colliders.empty() ? -1 : 0;
        return !m_colliders.empty();
    }

    bool StaticMeshEditor::SaveColliderAsset() const
    {
        if (m_meshPath.empty())
        {
            return false;
        }

        const std::filesystem::path colliderPath = std::filesystem::path(m_meshPath).concat(L".colliders");
        std::ofstream file(colliderPath);
        if (!file.is_open())
        {
            Core::Log::Error("Failed to save static mesh colliders: " + ToNarrowAscii(colliderPath));
            return false;
        }

        for (const StaticMeshPreviewViewport::ColliderPreview& collider : m_colliders)
        {
            file << "[Collider]\n";
            file << "type=" << ToColliderTypeName(collider.type) << "\n";
            file << "center=" << collider.center.x << "," << collider.center.y << "," << collider.center.z << "\n";
            file << "size=" << collider.size.x << "," << collider.size.y << "," << collider.size.z << "\n";
            file << "radius=" << collider.radius << "\n";
            file << "is_trigger=" << (collider.isTrigger ? "true" : "false") << "\n\n";
        }
        return true;
    }

    void StaticMeshEditor::SyncCollidersToPreview()
    {
        m_previewViewport.SetColliders(m_colliders);
    }

    void StaticMeshEditor::SetError(const std::string& error)
    {
        m_errorText = error;
        m_mesh.reset();
        m_previewViewport.SetMesh(nullptr);
        Core::Log::Error(error);
    }
}
