#include "D3D12Context.h"

#include "D3D12CommandList.h"
#include "D3D12CommandQueue.h"
#include "D3D12SwapChain.h"

namespace Engine::Graphics::D3D12
{
    void D3D12Context::ClearRenderTarget(float red, float green, float blue, float alpha)
    {
        if (!m_commandList || !m_commandQueue || !m_swapChain || !m_commandList->Begin())
        {
            return;
        }

        ID3D12GraphicsCommandList* commandList = m_commandList->GetNativeCommandList();
        ID3D12Resource* backBuffer = m_swapChain->GetCurrentBackBuffer();
        D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_swapChain->GetCurrentRenderTargetView();
        if (!commandList || !backBuffer || rtv.ptr == 0)
        {
            return;
        }

        D3D12_RESOURCE_BARRIER toRenderTarget = {};
        toRenderTarget.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        toRenderTarget.Transition.pResource = backBuffer;
        toRenderTarget.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        toRenderTarget.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        toRenderTarget.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &toRenderTarget);

        const float color[4] = { red, green, blue, alpha };
        commandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
        commandList->ClearRenderTargetView(rtv, color, 0, nullptr);

        D3D12_RESOURCE_BARRIER toPresent = toRenderTarget;
        toPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        toPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        commandList->ResourceBarrier(1, &toPresent);

        if (m_commandList->Close())
        {
            m_commandQueue->ExecuteCommandList(commandList);
        }
    }
}
