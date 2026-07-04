#pragma once

#include <cstdint>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

namespace Engine::Graphics::D3D12
{
    template <typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    inline bool Succeeded(HRESULT result)
    {
        return SUCCEEDED(result);
    }

    inline bool Failed(HRESULT result)
    {
        return FAILED(result);
    }
}
