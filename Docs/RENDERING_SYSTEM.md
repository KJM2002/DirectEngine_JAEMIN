# Rendering System

마지막 갱신: 2026-07-07

## 개요

현재 실제 렌더링 경로는 D3D11이다.

```text
Renderer::Initialize(D3D11)
  -> RHI::CreateRHIDevice
  -> D3D11Device::Initialize
  -> swapchain 생성
  -> render target/depth target 생성
  -> ResourceManager 생성
  -> DebugRenderer 초기화
  -> GizmoRenderer 초기화
  -> SelectionOutlineRenderer 초기화
  -> PostProcessStack 초기화
```

## Frame 흐름

```text
BeginFrame
  -> scene color/depth 또는 backbuffer 준비
Scene::Render
  -> MeshRendererComponent submit
  -> FlushRenderQueue
RenderColliders
RenderSelectionOutlines
ApplyPostProcess
RenderSelectionGizmo
EditorLayer::Render
EndFrame
```

PostProcess 또는 Selection Outline을 쓰는 경우 scene color target을 거쳐 최종 backbuffer에 합성한다.

## Mesh

`Renderer::Vertex`:

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

현재 HLSL input도 `TANGENT`를 받는다. normal map 계산에 tangent가 사용된다.

Built-in mesh:

- Cube
- Sphere
- Cylinder
- Cone
- Plane

Cylinder와 Cone은 cap 면을 포함한다.

## Material / PBR

`Material` 주요 속성:

- vertex shader
- pixel shader
- base color
- roughness
- metallic
- normal strength
- base color texture
- roughness texture
- metallic texture
- normal texture
- 각 texture path

`MaterialBufferData`는 다음 값을 GPU로 보낸다.

- base color
- texture 사용 여부
- roughness
- metallic
- normal strength

HLSL texture register:

- `t0`: Base Color
- `t1`: Shadow Map
- `t2`: Roughness
- `t3`: Metallic
- `t4`: Normal

Roughness/Metallic/Normal texture는 linear 옵션으로 로드한다. Base Color texture는 기본 sRGB 옵션을 사용한다.

## Selection Outline

선택 오브젝트 외곽선은 두 경로가 있다.

1. `SelectionOutlineRenderer` 기반 screen-space outline
2. `DebugRenderer` 기반 debug line outline

기본 권장 경로는 screen-space outline이다.

Screen-space outline 흐름:

```text
RenderSelectionOutlines
  -> SelectionOutlineRenderer::RenderMask
      -> selected MeshRenderer만 selection mask에 흰색 렌더
  -> SelectionOutlineRenderer::Composite
      -> SelectionOutlinePixel.hlsl에서 주변 mask 샘플링
      -> scene color 위에 outline 합성
```

설정:

- enabled
- color
- width
- opacity

선택 오브젝트가 없으면 outline pass는 건너뛴다.

## DebugRenderer

라인 기반 디버그 렌더러다.

사용처:

- grid
- axis
- collider 표시
- debug selection outline
- fallback gizmo line

Collider 표시는 기본 off다.

## GizmoRenderer

기즈모는 DebugRenderer line 기반만 쓰지 않고, 더 두꺼운 시각 요소를 위한 `GizmoRenderer` 경로도 가진다. Move / Rotate / Scale 모드를 지원한다.

## PostProcess

지원 effect:

- Grayscale
- Vignette
- Tone Mapping
- Color Adjust

`PostProcessComponent`가 활성화된 오브젝트를 기반으로 `EditorLayer::ApplyRendererSettings()`가 renderer setting을 갱신한다.

## ShadowMap

`ShadowMap` 클래스와 pass는 존재하지만, 현재 editor workflow에서 완성 기능으로 취급하기에는 제한적이다. Shadow 관련 변경은 D3D11 경로에서 별도 검증이 필요하다.

## 주의

- D3D11 input layout, C++ Vertex, HLSL input 구조를 함께 맞춰야 한다.
- Material texture register를 바꾸면 C++ 바인딩과 HLSL을 같이 수정해야 한다.
- RenderQueue 정렬은 material/mesh pointer 기준이다.
- D3D12는 현재 동등 기능을 기대하면 안 된다.
