#include "Mesh.h"

#include "Engine/Graphics/RHI/RHIDevice.h"

#include <algorithm>
#include <cfloat>
#include <iterator>
#include <utility>

namespace Engine::Renderer
{
    namespace
    {
        BoundingBox CalculateBounds(const std::vector<Vertex>& vertices)
        {
            BoundingBox bounds;
            if (vertices.empty())
            {
                return bounds;
            }

            bounds.min = { FLT_MAX, FLT_MAX, FLT_MAX };
            bounds.max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
            bounds.valid = true;
            for (const Vertex& vertex : vertices)
            {
                bounds.min.x = std::min(bounds.min.x, vertex.position[0]);
                bounds.min.y = std::min(bounds.min.y, vertex.position[1]);
                bounds.min.z = std::min(bounds.min.z, vertex.position[2]);
                bounds.max.x = std::max(bounds.max.x, vertex.position[0]);
                bounds.max.y = std::max(bounds.max.y, vertex.position[1]);
                bounds.max.z = std::max(bounds.max.z, vertex.position[2]);
            }
            return bounds;
        }
    }

    Math::Vector3 BoundingBox::GetCenter() const
    {
        return { (min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f, (min.z + max.z) * 0.5f };
    }

    Math::Vector3 BoundingBox::GetExtents() const
    {
        return { (max.x - min.x) * 0.5f, (max.y - min.y) * 0.5f, (max.z - min.z) * 0.5f };
    }

    Math::Vector3 BoundingBox::GetSize() const
    {
        return { max.x - min.x, max.y - min.y, max.z - min.z };
    }

    bool BoundingBox::IsValid() const
    {
        return valid;
    }

    Mesh::Mesh(std::shared_ptr<RHI::RHIBuffer> vertexBuffer, std::shared_ptr<RHI::RHIBuffer> indexBuffer, const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices, const BoundingBox& bounds)
        : m_vertexBuffer(std::move(vertexBuffer))
        , m_indexBuffer(std::move(indexBuffer))
        , m_vertices(vertices)
        , m_indices(indices)
        , m_bounds(bounds)
    {
        m_materialSlots.push_back({ "Slot 0", {} });
    }

    std::shared_ptr<Mesh> Mesh::CreateCube(RHI::RHIDevice& device)
    {
        const Vertex vertices[] =
        {
            { { -0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 0.1f, 0.1f, 1.0f }, { 0.0f, 1.0f } },
            { { -0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 0.8f, 0.1f, 1.0f }, { 0.0f, 0.0f } },
            { {  0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 0.1f, 1.0f, 0.1f, 1.0f }, { 1.0f, 0.0f } },
            { {  0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 0.1f, 0.8f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
            { { -0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.7f, 0.1f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
            { {  0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.9f, 0.9f, 0.9f, 1.0f }, { 1.0f, 1.0f } },
            { {  0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.2f, 0.9f, 0.9f, 1.0f }, { 1.0f, 0.0f } },
            { { -0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 0.1f, 0.8f, 1.0f }, { 0.0f, 0.0f } },
            { { -0.5f, -0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.7f, 0.1f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
            { { -0.5f,  0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 0.1f, 0.8f, 1.0f }, { 0.0f, 0.0f } },
            { { -0.5f,  0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 0.8f, 0.1f, 1.0f }, { 1.0f, 0.0f } },
            { { -0.5f, -0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 0.1f, 0.1f, 1.0f }, { 1.0f, 1.0f } },
            { {  0.5f, -0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.1f, 0.8f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
            { {  0.5f,  0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.1f, 1.0f, 0.1f, 1.0f }, { 0.0f, 0.0f } },
            { {  0.5f,  0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.2f, 0.9f, 0.9f, 1.0f }, { 1.0f, 0.0f } },
            { {  0.5f, -0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.9f, 0.9f, 0.9f, 1.0f }, { 1.0f, 1.0f } },
            { { -0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 0.8f, 0.1f, 1.0f }, { 0.0f, 1.0f } },
            { { -0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 0.1f, 0.8f, 1.0f }, { 0.0f, 0.0f } },
            { {  0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.2f, 0.9f, 0.9f, 1.0f }, { 1.0f, 0.0f } },
            { {  0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.1f, 1.0f, 0.1f, 1.0f }, { 1.0f, 1.0f } },
            { { -0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.7f, 0.1f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
            { { -0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 0.1f, 0.1f, 1.0f }, { 0.0f, 0.0f } },
            { {  0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.1f, 0.8f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
            { {  0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.9f, 0.9f, 0.9f, 1.0f }, { 1.0f, 1.0f } }
        };

        const std::uint32_t indices[] =
        {
            0, 1, 2, 0, 2, 3,
            4, 5, 6, 4, 6, 7,
            8, 9, 10, 8, 10, 11,
            12, 13, 14, 12, 14, 15,
            16, 17, 18, 16, 18, 19,
            20, 21, 22, 20, 22, 23
        };

        return CreateFromVertices(device, std::vector<Vertex>(std::begin(vertices), std::end(vertices)), std::vector<std::uint32_t>(std::begin(indices), std::end(indices)));
    }

    std::shared_ptr<Mesh> Mesh::CreateFromVertices(RHI::RHIDevice& device, const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices)
    {
        if (vertices.empty() || indices.empty())
        {
            return nullptr;
        }

        std::shared_ptr<RHI::RHIBuffer> vertexBuffer = device.CreateVertexBuffer(vertices.data(), static_cast<std::uint32_t>(vertices.size() * sizeof(Vertex)), sizeof(Vertex));
        if (!vertexBuffer)
        {
            return nullptr;
        }

        const std::uint32_t indexCount = static_cast<std::uint32_t>(indices.size());
        std::shared_ptr<RHI::RHIBuffer> indexBuffer = device.CreateIndexBuffer(indices.data(), static_cast<std::uint32_t>(indices.size() * sizeof(std::uint32_t)), indexCount);
        if (!indexBuffer)
        {
            return nullptr;
        }

        return std::make_shared<Mesh>(std::move(vertexBuffer), std::move(indexBuffer), vertices, indices, CalculateBounds(vertices));
    }

    const std::shared_ptr<RHI::RHIBuffer>& Mesh::GetVertexBuffer() const
    {
        return m_vertexBuffer;
    }

    const std::shared_ptr<RHI::RHIBuffer>& Mesh::GetIndexBuffer() const
    {
        return m_indexBuffer;
    }

    std::uint32_t Mesh::GetIndexCount() const
    {
        return static_cast<std::uint32_t>(m_indices.size());
    }

    std::uint32_t Mesh::GetVertexCount() const
    {
        return static_cast<std::uint32_t>(m_vertices.size());
    }

    std::uint32_t Mesh::GetTriangleCount() const
    {
        return static_cast<std::uint32_t>(m_indices.size() / 3);
    }

    std::uint32_t Mesh::GetSubMeshCount() const
    {
        return 1;
    }

    const BoundingBox& Mesh::GetBounds() const
    {
        return m_bounds;
    }

    const std::vector<MaterialSlot>& Mesh::GetMaterialSlots() const
    {
        return m_materialSlots;
    }

    const std::vector<Vertex>& Mesh::GetVertices() const
    {
        return m_vertices;
    }

    const std::vector<std::uint32_t>& Mesh::GetIndices() const
    {
        return m_indices;
    }
}
