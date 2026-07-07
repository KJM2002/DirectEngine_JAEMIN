# Maintenance Guide

마지막 갱신: 2026-07-07

## 기본 원칙

- 실제 실행 경로인 D3D11을 먼저 확인한다.
- D3D12는 현재 동등 기능 구현으로 가정하지 않는다.
- Scene, Resource, Editor, Renderer 변경은 저장/로드와 함께 확인한다.
- material 관련 변경은 C++ class, `.mat` 포맷, SceneSerializer, HLSL register를 함께 확인한다.
- built-in path는 실제 파일 경로가 아니므로 일반 asset import와 구분한다.

## Renderer 변경 시 확인

- `Renderer::Vertex`
- D3D11 input layout
- HLSL vertex input
- constant buffer packing
- texture register
- sampler register
- RenderQueue sorting
- PostProcess / SelectionOutline composite 경로

## Material 변경 시 확인

수정 대상:

- `Material.h/.cpp`
- `MaterialLoader.h/.cpp`
- `ResourceManager::LoadMaterial`
- `SceneSerializer`
- `EditorLayer` Inspector
- `EditorLayer` Material Editor
- `BasicPixel.hlsl`

현재 슬롯:

- base color
- roughness
- metallic
- normal
- normal strength

## Mesh 변경 시 확인

- `Mesh::CreateFromVertices`
- tangent 생성
- built-in primitive winding
- bounds 계산
- `ResourceManager::LoadMesh` built-in path 매핑
- Scene save/load path

## Editor 변경 시 확인

`EditorLayer.cpp`에 기능이 많이 모여 있다. 새 기능을 추가할 때 다음 충돌을 확인한다.

- ImGui ID 충돌
- drag/drop payload type
- selection 상태
- command undo/redo
- dirty scene 처리
- asset refresh
- material path/GUID 유지 여부

## Save/Load 변경 시 확인

- 기존 `.scene` 호환
- 기존 `.mat` 호환
- fallback이 조용히 들어가는지 여부
- `.tmp` / `.bak` 복구 시나리오
- GUID/path 우선순위

## 권장 리팩터링

- EditorLayer를 패널별 클래스로 분리
- Material Asset과 Material Instance 모델 명문화
- SceneSerializer를 component별 serializer로 분리
- ResourceManager 캐시 무효화 정책 강화
- D3D12 backend 유지/제거/격리 결정
- Editor/Game input context 분리
