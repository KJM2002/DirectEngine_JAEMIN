# Engine Overview

마지막 갱신: 2026-07-07

## 목적

`DirectEngine_JAEMIN`은 C++17 / DirectX11 기반 자체 게임 엔진과 ImGui 에디터를 함께 개발하는 프로젝트다. 현재 목표는 완성형 게임보다 엔진/에디터 기본 워크플로우를 갖추는 것이다.

## 실제 실행 경로

- 렌더링 API: D3D11
- UI: ImGui + D3D11 backend
- 진입점: `main.cpp`
- 애플리케이션 소유자: `Engine::Core::Application`
- 기본 실행 파일: `x64/Debug/DirectEngine_JAEMIN.exe`

D3D12 backend 파일은 존재하지만 현재 런타임 검증 경로가 아니다.

## 주요 시스템

- `Core`: Application, Time, Log
- `Platform/Windows`: Win32Window
- `Input`: 키보드/마우스 입력
- `Graphics/RHI`: API 독립 렌더링 인터페이스
- `Graphics/D3D11`: D3D11 device/context/swapchain 구현
- `Graphics/Renderer`: Renderer, Mesh, Material, DebugRenderer, GizmoRenderer, SelectionOutlineRenderer, PostProcess, ShadowMap
- `Resource`: ResourceManager, MaterialLoader, 이미지 로딩 데이터
- `Asset`: AssetDatabase, `.meta` GUID 관리
- `Scene`: Scene, GameObject, Component, Serializer
- `Editor`: EditorLayer, Command, Static Mesh Editor, PropertySystem

## 현재 기능

- GameObject / Component 기반 씬 구성
- MeshRenderer, Collider, Light, PostProcess, PlayerStart 컴포넌트
- OBJ 모델 로딩
- built-in primitive mesh: Cube, Sphere, Cylinder, Cone, Plane
- PBR 성격의 Material 파라미터 및 텍스처 슬롯
- Material Editor에서 `.mat` 에셋 편집
- Content Browser에서 에셋 생성, 삭제, rename, drag/drop
- Selection Outline screen-space pass
- DebugRenderer 기반 grid, axis, collider, fallback selection line
- Move / Rotate / Scale 기즈모
- `.scene` 저장/로드
- `.mat` 저장/로드
- Static Mesh Editor의 collider sidecar 편집

## 시작 씬

`Assets/Scenes/Test.scene` 로드에 성공하면 해당 씬을 연다. 없으면 fallback 씬을 생성한다.

fallback 씬은 회전하지 않는 `Cube` 하나를 만들고, 카메라는 큐브 우측면을 사선으로 바라보는 위치에 놓인다.

## 주의

- D3D11을 기준으로 변경하고 검증한다.
- D3D12는 현재 기능 구현 대상이 아니다.
- `.gltf`, `.glb`, `.fbx`는 AssetType 분류에 포함될 수 있지만 실제 메시 로딩은 OBJ 중심이다.
- 에셋 경로와 GUID가 함께 저장되지만, 실제 로드 흐름은 아직 경로 기반이 중심이다.
- `EditorLayer.cpp`에 많은 에디터 기능이 집중되어 있어 장기적으로 분리가 필요하다.
