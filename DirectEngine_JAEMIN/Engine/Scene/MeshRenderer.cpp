#include "MeshRenderer.h"

namespace Engine::Scene
{
    void MeshRenderer::SetMesh(Renderer::Mesh* mesh)
    {
        m_mesh = mesh;
    }

    void MeshRenderer::SetMaterial(Renderer::Material* material)
    {
        m_material = material;
    }

    Renderer::Mesh* MeshRenderer::GetMesh() const
    {
        return m_mesh;
    }

    Renderer::Material* MeshRenderer::GetMaterial() const
    {
        return m_material;
    }
}
