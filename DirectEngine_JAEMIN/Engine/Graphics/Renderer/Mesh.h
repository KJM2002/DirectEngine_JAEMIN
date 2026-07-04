#pragma once

#include "Engine/Graphics/RHI/RHIBuffer.h"
#include "Engine/Math/MathTypes.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Engine::RHI
{
    class RHIDevice;
}

namespace Engine::Renderer
{
    struct Vertex
    {
        float position[3] = {};
        float normal[3] = {};
        float color[4] = {};
        float uv[2] = {};
        float tangent[3] = {};
    };

    struct BoundingBox
    {
        Math::Vector3 min = { 0.0f, 0.0f, 0.0f };
        Math::Vector3 max = { 0.0f, 0.0f, 0.0f };
        bool valid = false;

        Math::Vector3 GetCenter() const;
        Math::Vector3 GetExtents() const;
        Math::Vector3 GetSize() const;
        bool IsValid() const;
    };

    struct MaterialSlot
    {
        std::string name = "Default";
        std::string materialGuid;
    };

    class Mesh
    {
    public:
        Mesh(std::shared_ptr<RHI::RHIBuffer> vertexBuffer, std::shared_ptr<RHI::RHIBuffer> indexBuffer, const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices, const BoundingBox& bounds);

        // Creates a built-in cube mesh while keeping API-specific buffer objects behind RHI.
        static std::shared_ptr<Mesh> CreateCube(RHI::RHIDevice& device);
        static std::shared_ptr<Mesh> CreateFromVertices(RHI::RHIDevice& device, const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices);

        const std::shared_ptr<RHI::RHIBuffer>& GetVertexBuffer() const;
        const std::shared_ptr<RHI::RHIBuffer>& GetIndexBuffer() const;
        std::uint32_t GetVertexCount() const;
        std::uint32_t GetIndexCount() const;
        std::uint32_t GetTriangleCount() const;
        std::uint32_t GetSubMeshCount() const;
        const BoundingBox& GetBounds() const;
        const std::vector<MaterialSlot>& GetMaterialSlots() const;
        const std::vector<Vertex>& GetVertices() const;
        const std::vector<std::uint32_t>& GetIndices() const;

    private:
        std::shared_ptr<RHI::RHIBuffer> m_vertexBuffer;
        std::shared_ptr<RHI::RHIBuffer> m_indexBuffer;
        std::vector<Vertex> m_vertices;
        std::vector<std::uint32_t> m_indices;
        BoundingBox m_bounds;
        std::vector<MaterialSlot> m_materialSlots;
    };
}
