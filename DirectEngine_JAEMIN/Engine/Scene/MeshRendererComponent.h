#pragma once

#include "Component.h"

#include <memory>
#include <string>

namespace Engine::Renderer
{
    class Material;
    class Mesh;
}

namespace Engine::Scene
{
    class MeshRendererComponent : public Component
    {
    public:
        static constexpr const char* StaticTypeName = "MeshRenderer";
        const char* GetTypeName() const override { return StaticTypeName; }

        void SetMesh(std::shared_ptr<Renderer::Mesh> mesh);
        void SetMaterial(std::shared_ptr<Renderer::Material> material);
        void SetMeshPath(std::wstring path);
        void SetMaterialPath(std::wstring path);
        void SetMeshGuid(std::string guid);
        void SetMaterialGuid(std::string guid);

        const std::shared_ptr<Renderer::Mesh>& GetMesh() const;
        const std::shared_ptr<Renderer::Material>& GetMaterial() const;
        const std::wstring& GetMeshPath() const;
        const std::wstring& GetMaterialPath() const;
        const std::string& GetMeshGuid() const;
        const std::string& GetMaterialGuid() const;

    private:
        std::shared_ptr<Renderer::Mesh> m_mesh;
        std::shared_ptr<Renderer::Material> m_material;
        std::wstring m_meshPath;
        std::wstring m_materialPath;
        std::string m_meshGuid;
        std::string m_materialGuid;
    };
}
