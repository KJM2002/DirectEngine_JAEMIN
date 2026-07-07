# Save Load System

마지막 갱신: 2026-07-07

## 개요

씬 저장/로드는 `SceneSerializer`가 담당한다. 포맷은 사람이 읽을 수 있는 커스텀 text 기반 `key=value` 구조다.

주요 파일:

- `.scene`
- `.mat`
- `.meta`
- `.colliders`

## Scene 저장 대상

GameObject:

- id
- name
- parent/children
- outliner folder
- transform position / rotation / scale

Component:

- MeshRendererComponent
- ColliderComponent
- LightComponent
- PostProcessComponent
- PlayerStartComponent

MeshRenderer:

- mesh path
- mesh GUID
- material path
- material GUID
- inline material state

## Material 저장

Material asset은 `.mat` 파일에 저장된다.

지원 값:

- name
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

씬 저장 시 material path가 `.mat` asset이면 path/GUID를 기록한다. asset 참조가 없는 runtime/override material은 inline material로 저장될 수 있다.

## Built-in Mesh 저장

기본 shape는 built-in path로 저장한다.

- `builtin:mesh:cube`
- `builtin:mesh:sphere`
- `builtin:mesh:cylinder`
- `builtin:mesh:cone`
- `builtin:mesh:plane`

로드 시 `ResourceManager::LoadMesh()`가 해당 path를 primitive 생성 함수로 매핑한다.

## Load fallback

로드 중 mesh가 없으면 fallback cube가 들어갈 수 있다. material이 없으면 fallback default material이 들어갈 수 있다. 따라서 로드 성공이 모든 asset 참조 성공을 의미하지는 않는다.

## Dirty / Save Prompt

에디터는 destructive action 전에 dirty scene 저장 여부를 묻는다.

대상:

- New Scene
- Open Scene
- Exit

## 주의

- 포맷 변경 시 기존 `.scene`과 `.mat` 호환성을 확인한다.
- material texture slot을 추가하면 `Material`, `MaterialLoader`, `SceneSerializer`, HLSL binding을 함께 수정한다.
- built-in path는 실제 파일 경로가 아니므로 일반 import asset처럼 처리하면 안 된다.

## 관련 파일

- `DirectEngine_JAEMIN/Engine/Scene/Serialization/SceneSerializer.cpp`
- `DirectEngine_JAEMIN/Engine/Resource/MaterialLoader.cpp`
- `DirectEngine_JAEMIN/Engine/Scene/MeshRendererComponent.*`
