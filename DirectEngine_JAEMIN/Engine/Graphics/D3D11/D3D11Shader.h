#pragma once

#include "D3D11Common.h"
#include "Engine/Graphics/RHI/RHIShader.h"

#include <memory>
#include <string>

namespace Engine::Graphics::D3D11
{
    class D3D11Shader final : public RHI::RHIShader
    {
    public:
        D3D11Shader(RHI::ShaderStage stage, ComPtr<ID3DBlob> byteCode);
        ~D3D11Shader() override = default;

        RHI::ShaderStage GetStage() const override;

        bool CreateVertexShader(ID3D11Device* device);
        bool CreatePixelShader(ID3D11Device* device);

        ID3D11VertexShader* GetVertexShader() const;
        ID3D11PixelShader* GetPixelShader() const;
        ID3D11InputLayout* GetInputLayout() const;

        static std::shared_ptr<D3D11Shader> Compile(ID3D11Device* device, const RHI::ShaderDesc& desc);

    private:
        RHI::ShaderStage m_stage;
        ComPtr<ID3DBlob> m_byteCode;
        ComPtr<ID3D11VertexShader> m_vertexShader;
        ComPtr<ID3D11PixelShader> m_pixelShader;
        ComPtr<ID3D11InputLayout> m_inputLayout;
    };
}
