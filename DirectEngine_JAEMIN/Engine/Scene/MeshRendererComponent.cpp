#include "MeshRendererComponent.h"

#include <utility>

#include <utility>

namespace Engine::Scene
{
    void MeshRendererComponent::SetMesh(std::shared_ptr<Renderer::Mesh> mesh)
    {
        m_mesh = std::move(mesh);
    }

    void MeshRendererComponent::SetMaterial(std::shared_ptr<Renderer::Material> material)
    {
        m_material = std::move(material);
    }

    void MeshRendererComponent::SetMeshPath(std::wstring path)
    {
        m_meshPath = std::move(path);
    }

    void MeshRendererComponent::SetMaterialPath(std::wstring path)
    {
        m_materialPath = std::move(path);
    }

    void MeshRendererComponent::SetMeshGuid(std::string guid)
    {
        m_meshGuid = std::move(guid);
    }

    void MeshRendererComponent::SetMaterialGuid(std::string guid)
    {
        m_materialGuid = std::move(guid);
    }

    const std::shared_ptr<Renderer::Mesh>& MeshRendererComponent::GetMesh() const
    {
        return m_mesh;
    }

    const std::shared_ptr<Renderer::Material>& MeshRendererComponent::GetMaterial() const
    {
        return m_material;
    }

    const std::wstring& MeshRendererComponent::GetMeshPath() const
    {
        return m_meshPath;
    }

    const std::wstring& MeshRendererComponent::GetMaterialPath() const
    {
        return m_materialPath;
    }

    const std::string& MeshRendererComponent::GetMeshGuid() const
    {
        return m_meshGuid;
    }

    const std::string& MeshRendererComponent::GetMaterialGuid() const
    {
        return m_materialGuid;
    }
}
