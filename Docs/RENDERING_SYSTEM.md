# RENDERING SYSTEM

## 개요

현재 실제 런타임 렌더링 경로는 DirectX 11이다. `Renderer::Initialize()`는 RHI factory를 통해 `D3D11Device`를 생성하고, swap chain, render target, constant buffer, debug renderer, shadow map, post process stack을 초기화한다.

`Application::Initialize()`에서 API가 명시적으로 D3D11로 전달된다.

```cpp
m_renderer.Initialize(RHI::GraphicsAPI::D3D11, m_window.GetNativeHandle(), width, height)
```

## DirectX11 초기화 흐름

```text
Application::Initialize
  -> Renderer::Initialize(D3D11)
      -> RHI::CreateRHIDevice(GraphicsAPI::D3D11)
          -> CreateD3D11Device()
      -> D3D11Device::Initialize()
          -> D3D11CreateDevice()
          -> D3D11Context 생성
      -> D3D11Device::CreateSwapChain()
      -> ResourceManager 생성
      -> CreateRenderResources()
      -> DebugRenderer::Initialize()
      -> ShadowMap::Initialize()
      -> PostProcessStack::Initialize()
```

## Device / Context / SwapChain 역할

### `D3D11Device`

역할:

- D3D11 device 생성
- Buffer 생성
- Shader 컴파일/생성
- Texture 생성
- RenderTarget/DepthTarget 생성
- SwapChain 생성
- ImGui에 native device handle 제공

### `D3D11Context`

역할:

- render target/depth clear
- viewport 설정
- primitive topology 설정
- vertex/index buffer 바인딩
- vertex/pixel shader 바인딩
- constant buffer 업데이트/바인딩
- texture/sampler 바인딩
- draw/draw indexed 호출

### `D3D11SwapChain`

역할:

- DXGI swap chain 생성
- back buffer render target view 생성
- depth stencil texture/view 생성
- resize 처리
- present 처리

## RenderTarget / DepthStencil 구조

SwapChain은 기본 back buffer RTV와 depth stencil view를 가진다.

Renderer는 후처리를 위해 별도의 scene render target도 만든다.

```text
Renderer
  m_sceneColorTarget: RGBA8 render target + shader resource
  m_sceneDepthTarget: D24S8 depth target
```

후처리가 꺼져 있으면 기본 swap chain render target에 직접 렌더링한다. 후처리가 켜져 있으면 scene color/depth target에 먼저 그리고, 이후 PostProcessStack이 scene color를 읽어 기본 render target에 fullscreen pass를 그린다.

## Mesh 렌더링 흐름

```text
Scene::Render
  -> Renderer::SetActiveCamera
  -> Renderer::SetDirectionalLight
  -> Renderer::SetLights
  -> Renderer::Render(GameObject)
      -> GameObject::GetComponent<MeshRendererComponent>()
      -> Renderer::SubmitMesh
  -> Renderer::FlushRenderQueue
      -> material/mesh pointer 기준 stable_sort
      -> Renderer::DrawMesh
      -> RHIContext::DrawIndexed
```

`Renderer::Render(GameObject)`는 `MeshRendererComponent`가 없거나 Mesh/Material이 없으면 아무 것도 제출하지 않는다.

## Vertex Buffer / Index Buffer 구조

렌더링 정점 구조는 `Renderer::Vertex`다.

```cpp
struct Vertex
{
    float position[3];
    float normal[3];
    float color[4];
    float uv[2];
    float tangent[3];
};
```

현재 D3D11 input layout은 `POSITION`, `NORMAL`, `COLOR`, `TEXCOORD`까지만 정의한다. `tangent`는 Vertex 구조에는 있지만 현재 D3D11 input layout과 기본 셰이더 입력에는 직접 포함되어 있지 않다.

## Blender 같은 모델링 툴의 Vertex Count와 엔진 Vertex Count 차이

현재 OBJLoader는 face의 `position/texcoord/normal` 조합을 문자열 key로 만들어 vertex cache를 구성한다.

```text
key = positionIndex / texcoordIndex / normalIndex
```

따라서 모델링 툴에서 보이는 정점 수와 엔진의 render vertex count가 다를 수 있다.

이유:

- 모델링 툴의 Vertex Count는 보통 위치 정점 수를 기준으로 보여준다.
- GPU Vertex Buffer는 position만이 아니라 normal, uv 조합이 함께 필요하다.
- 같은 위치라도 UV seam이 다르면 별도 렌더 정점이 된다.
- 같은 위치라도 normal이 다르면 hard edge 처리를 위해 별도 렌더 정점이 된다.
- OBJ face가 삼각형이 아니면 코드가 triangle fan 방식으로 삼각분할한다.

즉 `StaticMeshEditor`의 Vertices 값은 렌더링용 GPU vertex buffer 기준에 가깝다.

## Shader / Constant Buffer 구조

기본 렌더링 셰이더:

- `Shaders/BasicVertex.hlsl`
- `Shaders/BasicPixel.hlsl`

주요 constant buffer:

- `TransformBufferData`: world, view, projection
- `MaterialBufferData`: base color, texture 사용 여부, roughness, metallic
- `LightBufferData`: 최대 16개 `RenderLight`, ambient, light count
- `ShadowBufferData`: light view/projection, shadow params

주의: C++의 `LightBufferData`는 `RenderLight lights[16]`를 사용한다. HLSL의 `LightBuffer`는 `float4 Lights[64]`로 받아 1개 light를 float4 4개 단위로 해석한다. 메모리 레이아웃 계약이 깨지면 조명 데이터가 잘못 전달될 수 있다.

## Texture / Material 구조

`Material`은 다음 데이터를 가진다.

- vertex shader
- pixel shader
- base color
- roughness
- metallic
- base color texture
- roughness texture
- metallic texture
- normal texture
- 각 리소스 경로

`ResourceManager::CreateMaterial()`은 기본적으로 `BasicVertex.hlsl`, `BasicPixel.hlsl`을 로드한다.

텍스처는 WIC로 RGBA8 CPU image를 읽은 뒤 D3D11 texture/SRV/sampler로 만든다. 기본 옵션은 sRGB, mip generation, linear filtering, wrap address mode다.

## Camera / View / Projection 처리

`Scene::Camera`는 position, yaw, pitch, fov, near/far plane을 가진다.

```text
Camera::GetViewMatrix
  -> XMMatrixLookAtLH

Camera::GetProjectionMatrix
  -> XMMatrixPerspectiveFovLH
```

`Renderer::DrawMesh()`는 active camera가 없으면 fallback camera를 사용한다.

## DebugDraw

`DebugRenderer`는 line list 기반으로 동작한다.

확인된 기능:

- DrawLine
- DrawAABB
- DrawBox
- DrawCircle
- DrawSphere
- Flush

사용 경로:

- `Renderer::RenderColliders()`
- `Renderer::RenderSelectionGizmo()`

주의: `DebugRenderer::Flush()`는 매 flush마다 새 vertex buffer를 생성한다. 디버그 라인 수가 많아지면 비용이 커질 수 있다.

## PostProcess

`PostProcessStack`은 fullscreen quad를 만들고 다음 효과를 적용한다.

- Grayscale
- Vignette
- Tone Mapping
- Color Adjust

실제 활성화 조건:

```text
EditorLayer::ApplyRendererSettings
  -> scene의 GameObject 중 enabled PostProcessComponent 검색
  -> Renderer::SetPostProcessSettings
```

## ShadowMap

`ShadowMap` 클래스는 depth target과 shadow depth shader를 초기화한다. `Renderer::RenderShadowPass()`도 존재한다.

하지만 현재 `Renderer`의 `m_enableShadows`가 false이고, 이를 true로 설정하는 경로를 확인하지 못했다. 따라서 현재 코드 기준으로 shadow rendering은 리소스/함수는 있으나 실제 활성 기능으로 보기는 어렵다.

## D3D12 상태

D3D12 backend는 컴파일 대상이고 factory에도 등록되어 있다. 그러나 현재 앱은 D3D11을 선택한다.

D3D12 쪽은 다음이 부분 구현되어 있다.

- Device 생성
- Command queue/list/fence/descriptor heap 초기화
- SwapChain 생성
- ClearRenderTarget 일부

하지만 RHIDevice의 buffer/shader/texture 생성 함수가 대부분 `nullptr`을 반환하고, RHIContext의 draw 관련 함수 대부분이 비어 있다. 현재 렌더링 기능으로 사용하면 안 된다.

## 관련 파일

- `DirectEngine_JAEMIN/Engine/Graphics/Renderer/Renderer.cpp`
- `DirectEngine_JAEMIN/Engine/Graphics/Renderer/Mesh.cpp`
- `DirectEngine_JAEMIN/Engine/Graphics/Renderer/Material.cpp`
- `DirectEngine_JAEMIN/Engine/Graphics/Renderer/DebugRenderer.cpp`
- `DirectEngine_JAEMIN/Engine/Graphics/Renderer/PostProcess.cpp`
- `DirectEngine_JAEMIN/Engine/Graphics/Renderer/ShadowMap.cpp`
- `DirectEngine_JAEMIN/Engine/Graphics/D3D11/D3D11Device.cpp`
- `DirectEngine_JAEMIN/Engine/Graphics/D3D11/D3D11Context.cpp`
- `DirectEngine_JAEMIN/Engine/Graphics/D3D11/D3D11SwapChain.cpp`
- `DirectEngine_JAEMIN/Engine/AssetImport/OBJLoader.cpp`
- `DirectEngine_JAEMIN/Shaders/BasicVertex.hlsl`
- `DirectEngine_JAEMIN/Shaders/BasicPixel.hlsl`

## 유지보수 시 주의점

- Vertex 구조, D3D11 input layout, HLSL input 구조를 함께 맞춰야 한다.
- Constant buffer 구조체 변경 시 HLSL register와 packing을 같이 확인한다.
- Texture slot 번호는 C++와 HLSL 양쪽에서 맞춰야 한다.
- RenderQueue 정렬 기준이 material/mesh 포인터이므로 리소스 공유 방식 변경 시 draw order도 확인한다.
- D3D12는 현재 기능 완성 상태가 아니므로 D3D11과 동일하게 동작한다고 가정하면 안 된다.

## 앞으로 개선할 점

- Shadow 활성화 경로와 설정 UI 추가 또는 미사용 코드 정리
- D3D12 backend 완성 여부 결정
- DebugRenderer dynamic buffer 재사용
- RenderPass 또는 FrameGraph 구조 도입 검토
- Material/Shader binding 규칙 명문화
- Normal map을 실제 셰이더에서 사용하는지 점검
- Renderer 통계와 실제 draw call 집계 범위 정리
