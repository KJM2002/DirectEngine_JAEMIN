#include "RHIFactory.h"

#include "RHIDevice.h"

namespace Engine::Graphics::D3D11
{
    std::unique_ptr<RHI::RHIDevice> CreateD3D11Device();
}

namespace Engine::Graphics::D3D12
{
    std::unique_ptr<RHI::RHIDevice> CreateD3D12Device();
}

namespace Engine::RHI
{
    std::unique_ptr<RHIDevice> CreateRHIDevice(GraphicsAPI api)
    {
        if (api == GraphicsAPI::D3D11)
        {
            return Graphics::D3D11::CreateD3D11Device();
        }

        if (api == GraphicsAPI::D3D12)
        {
            return Graphics::D3D12::CreateD3D12Device();
        }

        return nullptr;
    }
}
