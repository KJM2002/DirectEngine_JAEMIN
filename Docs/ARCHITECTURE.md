# ARCHITECTURE

## 전체 모듈 구조

현재 엔진 코드는 `DirectEngine_JAEMIN/Engine` 아래에 기능별 폴더로 나뉘어 있다.

```text
Engine/
  Asset/          .meta, GUID, AssetType, AssetDatabase
  AssetImport/    OBJLoader
  Audio/          AudioSystem, AudioClip, AudioSource
  Core/           Application, Time, Log
  Debug/          DebugStats, DebugUI, ImGuiDebugUI
  Editor/         EditorLayer, Command, MeshEditor
  Graphics/
    RHI/          API 독립 인터페이스
    D3D11/        실제 사용 중인 렌더링 백엔드
    D3D12/        스캐폴드
    Renderer/     Renderer, Mesh, Material, DebugRenderer, PostProcess, ShadowMap
  Input/          Input
  Math/           MathTypes, Transform
  Physics/        ColliderComponent, PhysicsWorld, Raycast
  Platform/       Win32Window, WICImageLoader
  Resource/       ResourceManager, MaterialLoader, ImageData
  Scene/          Scene, GameObject, Component, Camera, Light, Components
```

## 주요 클래스 관계

```text
Application
  owns Win32Window
  owns Renderer
  owns Scene
  owns EditorLayer
  owns ImGuiDebugUI
  owns Input
  owns AudioSystem
  owns Time

Scene
  owns vector<unique_ptr<GameObject>>
  owns shared_ptr<Camera>
  owns shared_ptr<DirectionalLight>

GameObject
  owns Transform
  owns vector<shared_ptr<Component>>

Component
  <- MeshRendererComponent
  <- LightComponent
  <- ColliderComponent
  <- PostProcessComponent
  <- PlayerStartComponent

Renderer
  owns RHIDevice
  owns RHISwapChain
  owns ResourceManager
  owns DebugRenderer
  owns PostProcessStack
  owns ShadowMap
```

## 시스템 의존성

`Application`은 대부분의 상위 시스템을 직접 소유한다. 따라서 초기화 순서와 종료 순서가 중요하다.

초기화 순서:

```text
Win32Window
  -> Renderer(D3D11)
  -> AudioSystem
  -> Scene
  -> DebugUI
  -> EditorLayer
  -> ImGuiDebugUI
```

렌더링 의존성:

```text
Scene
  -> Renderer::SetActiveCamera
  -> Renderer::SetDirectionalLight
  -> Renderer::SetLights
  -> Renderer::Render(GameObject)
      -> MeshRendererComponent
      -> Mesh
      -> Material
      -> RHIContext draw calls
```

에셋 의존성:

```text
EditorLayer
  -> AssetDatabase::ScanAssets
  -> ResourceManager::SetAssetDatabase
  -> ResourceManager::LoadMesh / LoadMaterial / LoadTexture
```

저장/로드 의존성:

```text
SceneSerializer
  -> Scene
  -> GameObject
  -> Component data
  -> ResourceManager
```

## 실행 흐름

```text
main()
  -> Application::Initialize()
  -> Application::Run()
      while running:
        Input::BeginFrame()
        Win32Window::ProcessMessages()
        Renderer::Resize() if needed
        Time::Update()
        GameApp::Update()
        AudioSystem::Update()
        Application::Update()
        Application::Render()
  -> Application::Shutdown()
```

`GameApp::Update()`는 매 프레임 호출되지만 현재 함수 내부에는 게임 로직이 없다.

## 데이터 흐름

### 렌더링 데이터

```text
Asset file(.obj/.mat/.png)
  -> ResourceManager
  -> Mesh / Material / RHITexture / RHIShader
  -> MeshRendererComponent
  -> Scene::Render
  -> Renderer::SubmitMesh
  -> Renderer::FlushRenderQueue
  -> RHIContext::DrawIndexed
```

### 씬 데이터

```text
.scene file
  -> SceneSerializer::LoadScene
  -> Scene::CreateGameObject
  -> AddComponent<T>
  -> ResourceManager loads referenced resources
```

저장 시에는 반대로 `Scene` 내부의 Camera, DirectionalLight, GameObject, Component 데이터를 텍스트 포맷으로 기록한다.

### 에디터 입력 데이터

```text
Win32 WindowProc
  -> Input static handlers
  -> Application::Update
  -> EditorLayer::Update
  -> selection / gizmo / shortcuts
```

ImGui 입력 캡처 상태는 `EditorLayer::WantsInputCapture()`를 통해 `Application`에서 게임/카메라 입력 차단에 사용된다.

## 현재 구조에서 중요한 구분

### Runtime과 Editor가 강하게 결합되어 있음

현재 `Application`은 항상 `EditorLayer`와 `ImGuiDebugUI`를 초기화한다. 런타임 전용 실행 모드는 별도로 분리되어 있지 않다.

### Scene과 Renderer가 직접 연결됨

`Scene::Render(Renderer&)`가 렌더러에 카메라, 라이트, 게임오브젝트 렌더 제출을 직접 수행한다. 별도 RenderWorld, RenderScene, RenderGraph 계층은 없다.

### Component는 RTTI 기반 조회

`GameObject::GetComponent<T>()`와 `GetComponents<T>()`는 `dynamic_cast`로 타입을 확인한다. 컴포넌트 수가 많아지면 성능과 구조 관리 이슈가 생길 수 있다.

## 관련 파일

- `DirectEngine_JAEMIN/Engine/Core/Application.cpp`
- `DirectEngine_JAEMIN/Engine/Scene/Scene.cpp`
- `DirectEngine_JAEMIN/Engine/Scene/GameObject.h`
- `DirectEngine_JAEMIN/Engine/Graphics/Renderer/Renderer.cpp`
- `DirectEngine_JAEMIN/Engine/Editor/EditorLayer.cpp`
- `DirectEngine_JAEMIN/Engine/Resource/ResourceManager.cpp`

## 유지보수 시 주의점

- `Application`에 새 시스템을 추가할 때 초기화/종료 순서를 함께 설계해야 한다.
- `Scene`과 `Renderer` 사이의 호출 계약을 바꾸면 저장/로드, 에디터 선택, 디버그 드로잉까지 영향을 받을 수 있다.
- 컴포넌트 추가 시 `GameObjectSnapshot`, `SceneSerializer`, `EditorLayer` Details 패널, Undo/Redo 범위를 함께 확인해야 한다.
- Editor 전용 코드가 Runtime 모듈에 섞이지 않도록 include 방향을 주의한다.

## 앞으로 개선할 점

- Runtime 모드와 Editor 모드 분리
- `EditorLayer.cpp` 책임 분리
- Scene/Renderer 사이에 렌더 데이터 수집 계층 도입 검토
- Component 등록/직렬화 메타데이터 구조 도입 검토
- D3D11/D3D12 백엔드 완성도 차이를 명확히 분리
