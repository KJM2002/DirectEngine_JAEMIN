#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <wrl/client.h>

namespace Engine::Graphics::D3D11
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
