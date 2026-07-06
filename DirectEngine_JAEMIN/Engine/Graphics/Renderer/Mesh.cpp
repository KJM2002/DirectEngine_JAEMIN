#include "Mesh.h"

#include "Engine/Graphics/RHI/RHIDevice.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
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

        Math::Vector3 Subtract(const Vertex& a, const Vertex& b)
        {
            return
            {
                a.position[0] - b.position[0],
                a.position[1] - b.position[1],
                a.position[2] - b.position[2]
            };
        }

        float LengthSquared(const Math::Vector3& value)
        {
            return value.x * value.x + value.y * value.y + value.z * value.z;
        }

        Math::Vector3 Normalize(const Math::Vector3& value)
        {
            const float lengthSquared = LengthSquared(value);
            if (lengthSquared <= 0.000001f)
            {
                return { 1.0f, 0.0f, 0.0f };
            }

            const float invLength = 1.0f / std::sqrt(lengthSquared);
            return { value.x * invLength, value.y * invLength, value.z * invLength };
        }

        bool HasAnyTangent(const std::vector<Vertex>& vertices)
        {
            for (const Vertex& vertex : vertices)
            {
                const Math::Vector3 tangent = { vertex.tangent[0], vertex.tangent[1], vertex.tangent[2] };
                if (LengthSquared(tangent) > 0.000001f)
                {
                    return true;
                }
            }

            return false;
        }

        void AddTangent(Vertex& vertex, const Math::Vector3& tangent)
        {
            vertex.tangent[0] += tangent.x;
            vertex.tangent[1] += tangent.y;
            vertex.tangent[2] += tangent.z;
        }

        Math::Vector3 FallbackTangent(const Vertex& vertex)
        {
            const Math::Vector3 normal = Normalize({ vertex.normal[0], vertex.normal[1], vertex.normal[2] });
            const Math::Vector3 candidate = std::abs(normal.y) < 0.95f
                ? Math::Vector3{ 0.0f, 1.0f, 0.0f }
                : Math::Vector3{ 1.0f, 0.0f, 0.0f };

            return Normalize(
            {
                candidate.y * normal.z - candidate.z * normal.y,
                candidate.z * normal.x - candidate.x * normal.z,
                candidate.x * normal.y - candidate.y * normal.x
            });
        }

        void GenerateMissingTangents(std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices)
        {
            if (vertices.empty() || indices.size() < 3 || HasAnyTangent(vertices))
            {
                return;
            }

            for (std::size_t i = 0; i + 2 < indices.size(); i += 3)
            {
                Vertex& v0 = vertices[indices[i + 0]];
                Vertex& v1 = vertices[indices[i + 1]];
                Vertex& v2 = vertices[indices[i + 2]];
                const Math::Vector3 edge1 = Subtract(v1, v0);
                const Math::Vector3 edge2 = Subtract(v2, v0);
                const float du1 = v1.uv[0] - v0.uv[0];
                const float dv1 = v1.uv[1] - v0.uv[1];
                const float du2 = v2.uv[0] - v0.uv[0];
                const float dv2 = v2.uv[1] - v0.uv[1];
                const float determinant = du1 * dv2 - du2 * dv1;
                Math::Vector3 tangent = FallbackTangent(v0);
                if (std::abs(determinant) > 0.000001f)
                {
                    const float inv = 1.0f / determinant;
                    tangent =
                    {
                        (edge1.x * dv2 - edge2.x * dv1) * inv,
                        (edge1.y * dv2 - edge2.y * dv1) * inv,
                        (edge1.z * dv2 - edge2.z * dv1) * inv
                    };
                }

                AddTangent(v0, tangent);
                AddTangent(v1, tangent);
                AddTangent(v2, tangent);
            }

            for (Vertex& vertex : vertices)
            {
                Math::Vector3 tangent = Normalize({ vertex.tangent[0], vertex.tangent[1], vertex.tangent[2] });
                const Math::Vector3 normal = Normalize({ vertex.normal[0], vertex.normal[1], vertex.normal[2] });
                const float projection = tangent.x * normal.x + tangent.y * normal.y + tangent.z * normal.z;
                tangent = Normalize(
                {
                    tangent.x - normal.x * projection,
                    tangent.y - normal.y * projection,
                    tangent.z - normal.z * projection
                });
                vertex.tangent[0] = tangent.x;
                vertex.tangent[1] = tangent.y;
                vertex.tangent[2] = tangent.z;
            }
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

        std::vector<Vertex> resolvedVertices = vertices;
        GenerateMissingTangents(resolvedVertices, indices);

        std::shared_ptr<RHI::RHIBuffer> vertexBuffer = device.CreateVertexBuffer(resolvedVertices.data(), static_cast<std::uint32_t>(resolvedVertices.size() * sizeof(Vertex)), sizeof(Vertex));
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

        return std::make_shared<Mesh>(std::move(vertexBuffer), std::move(indexBuffer), resolvedVertices, indices, CalculateBounds(resolvedVertices));
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
