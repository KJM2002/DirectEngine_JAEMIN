#include "D3D11Shader.h"

#include "Engine/Core/Log.h"

namespace Engine::Graphics::D3D11
{
    D3D11Shader::D3D11Shader(RHI::ShaderStage stage, ComPtr<ID3DBlob> byteCode)
        : m_stage(stage)
        , m_byteCode(byteCode)
    {
    }

    RHI::ShaderStage D3D11Shader::GetStage() const
    {
        return m_stage;
    }

    bool D3D11Shader::CreateVertexShader(ID3D11Device* device)
    {
        HRESULT result = device->CreateVertexShader(
            m_byteCode->GetBufferPointer(),
            m_byteCode->GetBufferSize(),
            nullptr,
            m_vertexShader.GetAddressOf());

        if (Failed(result))
        {
            return false;
        }

        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        result = device->CreateInputLayout(
            layout,
            _countof(layout),
            m_byteCode->GetBufferPointer(),
            m_byteCode->GetBufferSize(),
            m_inputLayout.GetAddressOf());

        return Succeeded(result);
    }

    bool D3D11Shader::CreatePixelShader(ID3D11Device* device)
    {
        HRESULT result = device->CreatePixelShader(
            m_byteCode->GetBufferPointer(),
            m_byteCode->GetBufferSize(),
            nullptr,
            m_pixelShader.GetAddressOf());

        return Succeeded(result);
    }

    ID3D11VertexShader* D3D11Shader::GetVertexShader() const
    {
        return m_vertexShader.Get();
    }

    ID3D11PixelShader* D3D11Shader::GetPixelShader() const
    {
        return m_pixelShader.Get();
    }

    ID3D11InputLayout* D3D11Shader::GetInputLayout() const
    {
        return m_inputLayout.Get();
    }

    std::shared_ptr<D3D11Shader> D3D11Shader::Compile(ID3D11Device* device, const RHI::ShaderDesc& desc)
    {
        UINT flags = 0;
#if defined(_DEBUG)
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        ComPtr<ID3DBlob> byteCode;
        ComPtr<ID3DBlob> errors;
        HRESULT result = D3DCompileFromFile(
            desc.path.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            desc.entryPoint.c_str(),
            desc.target.c_str(),
            flags,
            0,
            byteCode.GetAddressOf(),
            errors.GetAddressOf());

        if (Failed(result))
        {
            if (errors)
            {
                const char* message = static_cast<const char*>(errors->GetBufferPointer());
                Core::Log::Error(message);
            }
            else
            {
                Core::Log::Error("Shader compilation failed.");
            }
            return nullptr;
        }

        auto shader = std::make_shared<D3D11Shader>(desc.stage, byteCode);
        bool created = desc.stage == RHI::ShaderStage::Vertex
            ? shader->CreateVertexShader(device)
            : shader->CreatePixelShader(device);

        if (!created)
        {
            Core::Log::Error("Failed to create native D3D11 shader.");
            return nullptr;
        }

        return shader;
    }
}
