# Scene System

마지막 갱신: 2026-07-07

## 개요

`Scene`은 GameObject 목록, active camera, directional light, outliner folder 정보를 가진다.

## 기본 흐름

```text
Application::InitializeScene
  -> Assets/Scenes/Test.scene 로드 시도
  -> 실패하면 fallback scene 생성
```

fallback scene:

- Cube 1개
- 기본 방향광
- 큐브 우측면을 사선으로 보는 카메라

## Update

```text
Scene::Update
  -> GameObject::Update
      -> Component::Update
```

현재 대부분의 component는 데이터 중심이며 runtime behavior는 제한적이다.

## Render

```text
Scene::Render
  -> Renderer::SetActiveCamera
  -> Renderer::SetDirectionalLight
  -> Renderer::SetLights
  -> Renderer::Render(GameObject)
  -> Renderer::FlushRenderQueue
```

`MeshRendererComponent`가 있고 mesh/material이 유효한 GameObject만 렌더 큐에 제출된다.

## Play Mode

Play Mode 진입 시:

- 현재 editor camera를 저장한다.
- PlayerStart가 있으면 camera를 PlayerStart 위치/설정으로 이동한다.
- 없으면 현재 camera를 그대로 사용한다.

Play Mode 종료 시 저장한 editor camera를 복원한다.

## Selection

에디터는 single/multi selection을 관리한다. 선택 오브젝트는 Renderer에 전달되어 screen-space selection outline과 gizmo 렌더링에 사용된다.

빈 공간 클릭 시 selection이 해제되고 gizmo도 비활성화된다.

## 관련 파일

- `DirectEngine_JAEMIN/Engine/Scene/Scene.*`
- `DirectEngine_JAEMIN/Engine/Core/Application.cpp`
- `DirectEngine_JAEMIN/Engine/Editor/EditorLayer.cpp`
