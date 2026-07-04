#include "D3D11Device.h"

#include "D3D11Buffer.h"
#include "D3D11Shader.h"
#include "D3D11SwapChain.h"
#include "D3D11Texture.h"
#include "Engine/Resource/ImageData.h"

namespace Engine::Graphics::D3D11
{
    namespace
    {
        std::uint32_t AlignConstantBufferSize(std::uint32_t size)
        {
            return (size + 15u) & ~15u;
        }

        UINT ToBindFlags(RHI::BufferType type)
        {
            switch (type)
            {
            case RHI::BufferType::Vertex:
                return D3D11_BIND_VERTEX_BUFFER;
            case RHI::BufferType::Index:
                return D3D11_BIND_INDEX_BUFFER;
            case RHI::BufferType::Constant:
                return D3D11_BIND_CONSTANT_BUFFER;
            }

            return 0;
        }

        DXGI_FORMAT ToDXGIFormat(RHI::TextureFormat format)
        {
            switch (format)
            {
            case RHI::TextureFormat::RGBA8:
                return DXGI_FORMAT_R8G8B8A8_UNORM;
            case RHI::TextureFormat::RGBA8_sRGB:
                return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            case RHI::TextureFormat::BGRA8:
                return DXGI_FORMAT_B8G8R8A8_UNORM;
            case RHI::TextureFormat::D24S8:
                return DXGI_FORMAT_D24_UNORM_S8_UINT;
            case RHI::TextureFormat::D32:
                return DXGI_FORMAT_D32_FLOAT;
            case RHI::TextureFormat::Unknown:
            default:
                return DXGI_FORMAT_UNKNOWN;
            }
        }

        DXGI_FORMAT ToTextureFormat(const RHI::TextureDesc& desc)
        {
            if (desc.depthStencil && desc.shaderResource)
            {
                if (desc.format == RHI::TextureFormat::D32)
                {
                    return DXGI_FORMAT_R32_TYPELESS;
                }
                if (desc.format == RHI::TextureFormat::D24S8)
                {
                    return DXGI_FORMAT_R24G8_TYPELESS;
                }
            }

            return ToDXGIFormat(desc.format);
        }

        DXGI_FORMAT ToShaderResourceFormat(RHI::TextureFormat format)
        {
            switch (format)
            {
            case RHI::TextureFormat::D32:
                return DXGI_FORMAT_R32_FLOAT;
            case RHI::TextureFormat::D24S8:
                return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
            default:
                return ToDXGIFormat(format);
            }
        }

        D3D11_FILTER ToD3DFilter(RHI::TextureFilter filter)
        {
            switch (filter)
            {
            case RHI::TextureFilter::Point:
                return D3D11_FILTER_MIN_MAG_MIP_POINT;
            case RHI::TextureFilter::ComparisonLinear:
                return D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
            case RHI::TextureFilter::Linear:
            default:
                return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            }
        }

        D3D11_TEXTURE_ADDRESS_MODE ToD3DAddressMode(RHI::TextureAddressMode mode)
        {
            switch (mode)
            {
            case RHI::TextureAddressMode::Clamp:
                return D3D11_TEXTURE_ADDRESS_CLAMP;
            case RHI::TextureAddressMode::Border:
                return D3D11_TEXTURE_ADDRESS_BORDER;
            case RHI::TextureAddressMode::Wrap:
            default:
                return D3D11_TEXTURE_ADDRESS_WRAP;
            }
        }
    }

    D3D11Device::D3D11Device() = default;
    D3D11Device::~D3D11Device() = default;

    bool D3D11Device::Initialize()
    {
        UINT flags = 0;
#if defined(_DEBUG)
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0
        };

        ComPtr<ID3D11DeviceContext> immediateContext;
        D3D_FEATURE_LEVEL createdFeatureLevel = D3D_FEATURE_LEVEL_11_0;

        HRESULT result = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            flags,
            featureLevels,
            _countof(featureLevels),
            D3D11_SDK_VERSION,
            m_device.GetAddressOf(),
            &createdFeatureLevel,
            immediateContext.GetAddressOf());

#if defined(_DEBUG)
        if (result == DXGI_ERROR_SDK_COMPONENT_MISSING)
        {
            flags &= ~D3D11_CREATE_DEVICE_DEBUG;
            result = D3D11CreateDevice(
                nullptr,
                D3D_DRIVER_TYPE_HARDWARE,
                nullptr,
                flags,
                featureLevels,
                _countof(featureLevels),
                D3D11_SDK_VERSION,
                m_device.GetAddressOf(),
                &createdFeatureLevel,
                immediateContext.GetAddressOf());
        }
#endif

        if (Failed(result))
        {
            return false;
        }

        m_context = std::make_unique<D3D11Context>(immediateContext);
        return true;
    }

    void D3D11Device::Shutdown()
    {
        m_context.reset();
        m_device.Reset();
    }

    D3D11Context& D3D11Device::GetContext()
    {
        return *m_context;
    }

    std::shared_ptr<RHI::RHIBuffer> D3D11Device::CreateBuffer(const RHI::BufferDesc& desc, const void* initialData)
    {
        if (desc.byteSize == 0)
        {
            return nullptr;
        }

        D3D11_BUFFER_DESC nativeDesc = {};
        nativeDesc.ByteWidth = static_cast<UINT>(desc.byteSize);
        nativeDesc.Usage = desc.usage == RHI::BufferUsage::Dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
        nativeDesc.BindFlags = ToBindFlags(desc.type);
        nativeDesc.CPUAccessFlags = desc.usage == RHI::BufferUsage::Dynamic ? D3D11_CPU_ACCESS_WRITE : 0;

        D3D11_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pSysMem = initialData;

        ComPtr<ID3D11Buffer> buffer;
        HRESULT result = m_device->CreateBuffer(
            &nativeDesc,
            initialData != nullptr ? &subresourceData : nullptr,
            buffer.GetAddressOf());

        if (Failed(result))
        {
            return nullptr;
        }

        return std::make_shared<D3D11Buffer>(desc, buffer);
    }

    std::shared_ptr<RHI::RHIBuffer> D3D11Device::CreateVertexBuffer(const void* data, std::uint32_t size, std::uint32_t stride)
    {
        RHI::BufferDesc desc;
        desc.usage = RHI::BufferUsage::Static;
        desc.type = RHI::BufferType::Vertex;
        desc.byteSize = size;
        desc.stride = stride;
        desc.count = stride == 0 ? 0 : size / stride;
        return CreateBuffer(desc, data);
    }

    std::shared_ptr<RHI::RHIBuffer> D3D11Device::CreateIndexBuffer(const void* data, std::uint32_t size, std::uint32_t indexCount)
    {
        RHI::BufferDesc desc;
        desc.usage = RHI::BufferUsage::Static;
        desc.type = RHI::BufferType::Index;
        desc.byteSize = size;
        desc.stride = sizeof(std::uint32_t);
        desc.count = indexCount;
        return CreateBuffer(desc, data);
    }

    std::shared_ptr<RHI::RHIBuffer> D3D11Device::CreateConstantBuffer(std::uint32_t size)
    {
        const std::uint32_t alignedSize = AlignConstantBufferSize(size);

        RHI::BufferDesc desc;
        desc.usage = RHI::BufferUsage::Dynamic;
        desc.type = RHI::BufferType::Constant;
        desc.byteSize = alignedSize;
        desc.stride = alignedSize;
        desc.count = 1;
        desc.dynamic = true;
        return CreateBuffer(desc, nullptr);
    }

    std::shared_ptr<RHI::RHIShader> D3D11Device::CreateShader(const RHI::ShaderDesc& desc)
    {
        return D3D11Shader::Compile(m_device.Get(), desc);
    }

    std::shared_ptr<RHI::RHIShader> D3D11Device::CreateVertexShader(const std::wstring& filePath, const std::string& entryPoint)
    {
        RHI::ShaderDesc desc;
        desc.stage = RHI::ShaderStage::Vertex;
        desc.path = filePath;
        desc.entryPoint = entryPoint;
        desc.target = "vs_5_0";
        return CreateShader(desc);
    }

    std::shared_ptr<RHI::RHIShader> D3D11Device::CreatePixelShader(const std::wstring& filePath, const std::string& entryPoint)
    {
        RHI::ShaderDesc desc;
        desc.stage = RHI::ShaderStage::Pixel;
        desc.path = filePath;
        desc.entryPoint = entryPoint;
        desc.target = "ps_5_0";
        return CreateShader(desc);
    }

    std::shared_ptr<RHI::RHITexture> D3D11Device::CreateTexture2D(const RHI::TextureDesc& desc)
    {
        if (desc.width == 0 || desc.height == 0)
        {
            return nullptr;
        }

        const DXGI_FORMAT format = ToTextureFormat(desc);
        if (format == DXGI_FORMAT_UNKNOWN)
        {
            return nullptr;
        }

        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width = desc.width;
        textureDesc.Height = desc.height;
        textureDesc.MipLevels = desc.generateMips ? 0 : 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = format;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = 0;
        textureDesc.MiscFlags = desc.generateMips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;
        if (desc.shaderResource)
        {
            textureDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
        }
        if (desc.renderTarget || desc.generateMips)
        {
            textureDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
        }
        if (desc.depthStencil)
        {
            textureDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
        }

        D3D11_SUBRESOURCE_DATA textureData = {};
        textureData.pSysMem = desc.initialData;
        textureData.SysMemPitch = desc.rowPitch;

        ComPtr<ID3D11Texture2D> texture;
        const bool uploadInitialDataAfterCreate = desc.generateMips && desc.initialData;
        const D3D11_SUBRESOURCE_DATA* initialData = desc.initialData && !uploadInitialDataAfterCreate ? &textureData : nullptr;
        HRESULT result = m_device->CreateTexture2D(&textureDesc, initialData, texture.GetAddressOf());
        if (Failed(result))
        {
            return nullptr;
        }

        ComPtr<ID3D11ShaderResourceView> shaderResourceView;
        if (desc.shaderResource)
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = ToShaderResourceFormat(desc.format);
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = desc.generateMips ? -1 : 1;

            result = m_device->CreateShaderResourceView(texture.Get(), &srvDesc, shaderResourceView.GetAddressOf());
            if (Failed(result))
            {
                return nullptr;
            }
        }

        ComPtr<ID3D11RenderTargetView> renderTargetView;
        if (desc.renderTarget)
        {
            result = m_device->CreateRenderTargetView(texture.Get(), nullptr, renderTargetView.GetAddressOf());
            if (Failed(result))
            {
                return nullptr;
            }
        }

        ComPtr<ID3D11DepthStencilView> depthStencilView;
        if (desc.depthStencil)
        {
            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = ToDXGIFormat(desc.format);
            dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            result = m_device->CreateDepthStencilView(texture.Get(), &dsvDesc, depthStencilView.GetAddressOf());
            if (Failed(result))
            {
                return nullptr;
            }
        }

        ComPtr<ID3D11SamplerState> samplerState;
        if (desc.shaderResource)
        {
            D3D11_SAMPLER_DESC samplerDesc = {};
            samplerDesc.Filter = ToD3DFilter(desc.filter);
            samplerDesc.AddressU = ToD3DAddressMode(desc.addressMode);
            samplerDesc.AddressV = ToD3DAddressMode(desc.addressMode);
            samplerDesc.AddressW = ToD3DAddressMode(desc.addressMode);
            samplerDesc.ComparisonFunc = desc.filter == RHI::TextureFilter::ComparisonLinear ? D3D11_COMPARISON_LESS_EQUAL : D3D11_COMPARISON_NEVER;
            samplerDesc.BorderColor[0] = 1.0f;
            samplerDesc.BorderColor[1] = 1.0f;
            samplerDesc.BorderColor[2] = 1.0f;
            samplerDesc.BorderColor[3] = 1.0f;
            samplerDesc.MinLOD = 0.0f;
            samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

            result = m_device->CreateSamplerState(&samplerDesc, samplerState.GetAddressOf());
            if (Failed(result))
            {
                return nullptr;
            }
        }

        if (desc.generateMips && shaderResourceView && desc.initialData)
        {
            m_context->GetNativeContext()->UpdateSubresource(texture.Get(), 0, nullptr, desc.initialData, desc.rowPitch, 0);
            m_context->GetNativeContext()->GenerateMips(shaderResourceView.Get());
        }

        return std::make_shared<D3D11Texture>(desc, texture, shaderResourceView, samplerState, renderTargetView, depthStencilView);
    }

    std::shared_ptr<RHI::RHITexture> D3D11Device::CreateTexture2D(const Resource::ImageData& image)
    {
        if (!image.IsValid())
        {
            return nullptr;
        }

        RHI::TextureDesc desc;
        desc.width = image.width;
        desc.height = image.height;
        desc.format = image.format;
        desc.initialData = image.pixels.data();
        desc.rowPitch = image.GetRowPitch();
        return CreateTexture2D(desc);
    }

    std::shared_ptr<RHI::RHITexture> D3D11Device::CreateRenderTarget(std::uint32_t width, std::uint32_t height, RHI::TextureFormat format)
    {
        RHI::TextureDesc desc;
        desc.width = width;
        desc.height = height;
        desc.format = format;
        desc.shaderResource = true;
        desc.renderTarget = true;
        desc.depthStencil = false;
        desc.filter = RHI::TextureFilter::Linear;
        desc.addressMode = RHI::TextureAddressMode::Clamp;
        return CreateTexture2D(desc);
    }

    std::shared_ptr<RHI::RHITexture> D3D11Device::CreateDepthTarget(std::uint32_t width, std::uint32_t height, RHI::TextureFormat format, bool shaderResource)
    {
        RHI::TextureDesc desc;
        desc.width = width;
        desc.height = height;
        desc.format = format;
        desc.shaderResource = shaderResource;
        desc.renderTarget = false;
        desc.depthStencil = true;
        desc.filter = shaderResource ? RHI::TextureFilter::ComparisonLinear : RHI::TextureFilter::Point;
        desc.addressMode = RHI::TextureAddressMode::Border;
        return CreateTexture2D(desc);
    }

    std::unique_ptr<RHI::RHISwapChain> D3D11Device::CreateSwapChain(const RHI::SwapChainDesc& desc)
    {
        ComPtr<IDXGIFactory> factory = CreateDXGIFactoryFromDevice();
        if (!factory)
        {
            return nullptr;
        }

        auto swapChain = std::make_unique<D3D11SwapChain>(m_device.Get(), factory.Get(), m_context.get(), desc.nativeWindowHandle, desc.width, desc.height);
        if (swapChain->GetRenderTargetView() == nullptr || swapChain->GetDepthStencilView() == nullptr)
        {
            return nullptr;
        }

        m_context->SetDefaultRenderTargets(swapChain->GetRenderTargetView(), swapChain->GetDepthStencilView());
        return swapChain;
    }

    void* D3D11Device::GetNativeHandleForDebugUI() const
    {
        return m_device.Get();
    }

    ComPtr<IDXGIFactory> D3D11Device::CreateDXGIFactoryFromDevice() const
    {
        ComPtr<IDXGIDevice> dxgiDevice;
        ComPtr<IDXGIAdapter> adapter;
        ComPtr<IDXGIFactory> factory;

        HRESULT result = m_device.As(&dxgiDevice);
        if (Succeeded(result))
        {
            result = dxgiDevice->GetAdapter(adapter.GetAddressOf());
        }
        if (Succeeded(result))
        {
            result = adapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(factory.GetAddressOf()));
        }

        return Succeeded(result) ? factory : nullptr;
    }

    std::unique_ptr<RHI::RHIDevice> CreateD3D11Device()
    {
        return std::make_unique<D3D11Device>();
    }
}
