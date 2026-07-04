#pragma once

namespace Engine::Renderer
{
    class Material;
    class Mesh;
}

namespace Engine::Scene
{
    class MeshRenderer
    {
    public:
        void SetMesh(Renderer::Mesh* mesh);
        void SetMaterial(Renderer::Material* material);

        Renderer::Mesh* GetMesh() const;
        Renderer::Material* GetMaterial() const;

    private:
        Renderer::Mesh* m_mesh = nullptr;
        Renderer::Material* m_material = nullptr;
    };
}
