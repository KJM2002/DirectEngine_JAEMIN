# Object Component System

마지막 갱신: 2026-07-07

## 개요

씬은 GameObject와 Component로 구성된다.

```text
Scene
  -> GameObject
      -> Transform
      -> Component list
```

GameObject와 Component는 저장/복원 및 command snapshot을 위해 ID를 가진다.

## GameObject

주요 데이터:

- ObjectID
- name
- parent ID
- children IDs
- outliner folder
- transform
- components

## Transform

- position
- rotation
- scale

에디터 Inspector에서 편집 가능하며 일부는 command 기반 Undo/Redo에 들어간다.

## MeshRendererComponent

렌더링에 사용되는 실제 컴포넌트다.

보관 데이터:

- mesh shared_ptr
- material shared_ptr
- mesh path
- material path
- mesh GUID
- material GUID

Material asset을 적용하면 clone된 material instance를 들고 path/GUID를 유지한다. Inspector에서 직접 material 값을 편집하면 override로 간주하고 material path/GUID를 지운다.

## ColliderComponent

지원 타입:

- AABB
- Sphere

Collider 표시 UI는 기본 off다. Static Mesh Editor에서는 `.colliders` sidecar로 reusable collider 설정을 편집할 수 있다.

## LightComponent

지원 타입:

- Directional
- Point
- Spot

Renderer는 scene light 정보를 모아 shader constant buffer로 넘긴다.

## PostProcessComponent

지원 effect:

- Grayscale
- Vignette
- Tone Mapping
- Color Adjust

활성화된 PostProcessComponent가 renderer setting에 반영된다.

## PlayerStartComponent

Play Mode 시작 위치로 사용된다. PlayerStart가 없으면 현재 카메라 위치로 Play Mode를 시작한다.

fallback 시작 씬에는 PlayerStart를 자동 생성하지 않는다.

## Inspector Add / Remove

Inspector에서 다음 component를 추가/삭제할 수 있다.

- Mesh Renderer
- Collider
- Light
- Post Process
- Player Start

## 관련 파일

- `DirectEngine_JAEMIN/Engine/Scene/GameObject.*`
- `DirectEngine_JAEMIN/Engine/Scene/Component.*`
- `DirectEngine_JAEMIN/Engine/Scene/MeshRendererComponent.*`
- `DirectEngine_JAEMIN/Engine/Physics/ColliderComponent.*`
- `DirectEngine_JAEMIN/Engine/Scene/LightComponent.*`
- `DirectEngine_JAEMIN/Engine/Scene/PostProcessComponent.*`
- `DirectEngine_JAEMIN/Engine/Scene/PlayerStartComponent.*`
