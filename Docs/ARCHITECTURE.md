# Architecture

마지막 갱신: 2026-07-07

## 큰 흐름

```text
main
  -> Core::Application
      -> Win32Window
      -> Input
      -> Renderer
      -> Scene
      -> EditorLayer
      -> ImGuiDebugUI
```

`Application`은 초기화, 메인 루프, 업데이트, 렌더링, 종료를 소유한다.

## 업데이트 흐름

```text
Application::Update
  -> SynchronizeSceneReferences
  -> EditorLayer::Update
  -> play mode enter/exit 처리
  -> editor camera 또는 runtime camera update
  -> Scene::Update
  -> PhysicsWorld overlap update
```

현재 fallback 기본 큐브를 자동 회전시키는 데모 코드는 제거되어 있다.

## 렌더링 흐름

```text
Application::Render
  -> EditorLayer::ApplyRendererSettings
  -> Renderer::RenderShadowPass
  -> Renderer::BeginFrame
  -> ImGuiDebugUI::BeginFrame
  -> Scene::Render
      -> Renderer::Render(GameObject)
      -> Renderer::FlushRenderQueue
  -> Renderer::RenderColliders
  -> Renderer::RenderSelectionOutlines
  -> Renderer::ApplyPostProcess
  -> Renderer::RenderSelectionGizmo
  -> EditorLayer::Render
  -> ImGuiDebugUI::EndFrame
  -> Renderer::EndFrame
```

Selection Outline은 Scene Color 위에 합성되는 별도 pass다. Debug line 기반 selection outline은 옵션으로 분리되어 있다.

## 에디터 구조

`EditorLayer`는 다음 역할을 가진다.

- Main Menu
- Main Toolbar
- Outliner
- Inspector
- Content Browser
- Material Editor
- Static Mesh Editor 연동
- selection / gizmo / shortcut 처리
- command 기반 Undo/Redo 일부

파일이 커져 있으므로 장기적으로 패널별 분리가 필요하다.

## 리소스 구조

```text
Renderer
  -> ResourceManager
      -> Mesh cache
      -> Texture cache
      -> Material cache
      -> Shader cache
```

AssetDatabase는 `Assets` 폴더를 스캔하고 `.meta` GUID를 관리한다.

## Scene 구조

```text
Scene
  -> GameObject
      -> Transform
      -> Component list
          -> MeshRendererComponent
          -> ColliderComponent
          -> LightComponent
          -> PostProcessComponent
          -> PlayerStartComponent
```

GameObject와 Component는 ID를 가진다. Scene save/load와 Command snapshot에서 이 ID를 사용한다.
