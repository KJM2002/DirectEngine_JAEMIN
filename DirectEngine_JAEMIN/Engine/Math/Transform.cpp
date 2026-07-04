#include "Transform.h"

namespace Engine::Math
{
    Matrix Transform::GetWorldMatrix() const
    {
        const Matrix scaleMatrix = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
        const Matrix rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
        const Matrix translationMatrix = DirectX::XMMatrixTranslation(position.x, position.y, position.z);

        // Row-vector convention: local position * Scale * Rotation * Translation.
        return scaleMatrix * rotationMatrix * translationMatrix;
    }
}
