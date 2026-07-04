# Maintenance Guide

이 문서는 현재 프로젝트 구조를 기준으로 새 기능을 추가하거나 기존 코드를 수정할 때 지켜야 할 기준을 정리한다. 없는 시스템을 전제로 한 절차는 포함하지 않았고, 실제 코드에서 확인된 구조를 중심으로 작성했다.

## 기본 원칙

- 먼저 호출 흐름을 확인한 뒤 수정한다.
- 클래스 이름만 보고 기능을 단정하지 않는다.
- 사용되지 않는 파일과 실제 실행 경로를 구분한다.
- Runtime 코드, Editor 코드, Asset 코드, RHI 코드를 섞어 수정하지 않는다.
- 저장/로드 포맷을 바꾸는 작업은 기존 `.scene`, `.mat`, `.meta` 파일 호환성을 먼저 확인한다.
- DirectX 리소스 수정은 생성, 바인딩, 해제 지점을 한 세트로 본다.
- 기능 추가 후 최소한 Debug x64 빌드와 에디터 실행 흐름을 확인한다.

## 코드 영역별 책임

현재 확인된 큰 영역은 다음과 같다.

```text
Core
  Application, Time, Log

Platform/Windows
  Win32Window, 메시 처리

Input
  키보드/마우스 상태

Graphics/RHI
  RHIDevice, RHIContext, RHISwapChain 추상화

Graphics/D3D11
  실제 사용 중인 DirectX11 backend

Graphics/D3D12
  부분 구현 또는 준비 코드

Graphics/Renderer
  Renderer, Mesh, Material, DebugRenderer, PostProcess, ShadowMap

Scene
  Scene, GameObject, Component, Camera, Light, 직렬화

Resource / Asset / AssetImport
  리소스 캐시, 에셋 DB, OBJ 로딩, Material 로딩

Editor
  EditorLayer, Command, DirtyManager, StaticMeshEditor

Physics
  ColliderComponent, PhysicsWorld

Audio
  WinMM 기반 skeleton 성격의 AudioSystem
```

## 새 기능 추가 시 작업 순서

1. 기존 호출 흐름을 먼저 찾는다.
2. 기능이 Runtime 기능인지 Editor 기능인지 구분한다.
3. 데이터가 저장되어야 하는지 결정한다.
4. 저장이 필요하면 `SceneSerializer`, `.mat`, `.meta` 중 어느 포맷이 관련되는지 확인한다.
5. Undo/Redo가 필요한 에디터 조작인지 확인한다.
6. 렌더링 리소스가 필요하면 ResourceManager와 Renderer의 수명 경계를 확인한다.
7. 최소 단위로 구현하고 Debug x64 빌드를 확인한다.
8. 새 파일을 추가했다면 `.vcxproj` 포함 여부를 확인한다.
9. 문서를 갱신한다.

## 렌더링 코드 수정 시 주의사항

현재 실행 경로는 D3D11이다. `Application::Initialize()`에서 `Renderer.Initialize(RHI::GraphicsAPI::D3D11, ...)`가 호출된다.

렌더링 흐름은 대략 다음과 같다.

```text
Application::Render
-> Renderer::BeginFrame
-> Scene::Render
-> Renderer::Render
-> Renderer::FlushRenderQueue
-> Renderer::DrawMesh
-> DebugRenderer::Flush
-> EditorLayer::Render
-> Renderer::EndFrame
```

수정 시 다음을 함께 확인해야 한다.

- `Renderer::Vertex` 구조
- D3D11 input layout
- HLSL shader input 구조
- Transform/Material/Light/Shadow constant buffer
- Texture slot 바인딩
- RenderTarget/DepthStencil resize 처리
- DebugRenderer와 일반 Mesh 렌더링의 셰이더 호환성

`Renderer::Vertex`에는 tangent가 있지만 현재 기본 D3D11 input layout과 기본 셰이더에서는 position, normal, color, uv 중심으로 사용된다. tangent를 실제 셰이더 입력으로 쓰려면 C++ input layout과 HLSL 구조를 같이 수정해야 한다.

ShadowMap 코드는 존재하지만 현재 `m_enableShadows` 기본값과 호출 구조상 실제 shadow pass가 켜져 있다고 단정하면 안 된다. Shadow 기능을 수정할 때는 UI/설정에서 활성화 경로가 실제로 연결되어 있는지 먼저 확인해야 한다.

## DirectX11 리소스 생성 / 해제 주의사항

현재 RHI 리소스는 `std::shared_ptr`과 COM wrapper 성격의 D3D11 클래스 조합으로 관리된다. 수정 시 다음 기준을 지킨다.

- D3D11 resource 생성 실패를 반드시 확인한다.
- 실패 시 null 리소스가 후속 draw에서 사용되지 않도록 한다.
- resize 시 swap chain, render target, depth stencil이 함께 갱신되는지 확인한다.
- shader constant buffer 크기와 HLSL cbuffer layout을 함께 검증한다.
- texture slot을 추가하면 C++ bind 지점과 HLSL register를 함께 문서화한다.
- DebugRenderer도 RHI device/context를 사용하므로 Renderer 변경의 영향을 받는다.

## Scene / GameObject 수정 시 주의사항

`Scene`은 `std::vector<std::unique_ptr<GameObject>>`로 오브젝트를 소유한다. `GameObject`는 `std::vector<std::shared_ptr<Component>>`로 컴포넌트를 가진다.

주의할 점은 다음과 같다.

- `Component`는 owner raw pointer를 가진다.
- `AddComponent<T>()`, `GetComponent<T>()`는 템플릿과 `dynamic_cast` 기반이다.
- `Scene::Render()`는 `MeshRendererComponent`, `LightComponent` 등 실제 컴포넌트를 탐색한다.
- 새 Component를 추가해도 자동으로 업데이트, 렌더링, 저장이 되는 구조는 아니다.
- 오브젝트 생성/삭제/복제/이름 변경은 에디터 Command와도 연결된다.

Scene 구조를 바꿀 때는 다음 파일을 같이 확인한다.

- `Scene.h/.cpp`
- `GameObject.h/.cpp`
- `Component.h/.cpp`
- `SceneSerializer.cpp`
- `EditorLayer.cpp`
- `GameObjectSnapshot.cpp`

## ResourceManager 수정 시 주의사항

`ResourceManager`는 리소스 캐시와 로딩을 함께 담당한다. 새 리소스 타입이나 로딩 옵션을 추가할 때는 다음을 확인한다.

- 캐시 키 정책
- 경로 정규화 여부
- GUID 기반 조회와 경로 기반 조회의 관계
- fallback 리소스 사용 여부
- 실패 로그
- AssetDatabase와의 연결 여부
- SceneSerializer 또는 MaterialLoader 저장 포맷 영향

AssetDatabase가 인식하는 타입과 ResourceManager가 실제 로드할 수 있는 포맷이 다르면 에디터에서 보이지만 로딩은 실패하는 상태가 된다. 이 차이를 UI나 로그에서 명확히 보여줘야 한다.

## Editor UI 수정 시 주의사항

에디터 코드는 `EditorLayer.cpp`에 많은 기능이 집중되어 있다. 메뉴, 툴바, Content Browser, Details, Viewport, 저장 프롬프트, Static Mesh Editor 연동이 한 파일에 많이 들어 있다.

수정 시 다음을 확인한다.

- ImGui 입력 포커스와 `InputBlocked` 처리
- DirtyManager 갱신
- CommandManager Undo/Redo 등록 여부
- Scene 선택 상태와 삭제/복제 시 포인터 안정성
- AssetDatabase 재스캔 시 선택된 에셋 경로 유효성
- Save Prompt가 뜨는 조건
- Play Mode 중 에디터 입력과 카메라 입력 충돌 여부

UI 버튼이 있다고 해서 실제 기능이 완전히 연결되어 있다고 보면 안 된다. 예를 들어 Static Mesh Editor의 일부 Save/Reimport/Import Scale UI는 비활성화된 상태로 확인된다.

## 저장 / 로드 시스템 수정 시 주의사항

`SceneSerializer`는 수동 key/value 파서다. 새 저장 필드를 추가할 때는 저장과 로드를 동시에 수정해야 한다.

수정 기준은 다음과 같다.

- 새 key 이름은 기존 key와 충돌하지 않게 정한다.
- 기본값을 명확히 둔다.
- 누락된 key를 읽어도 기존 파일이 깨지지 않게 한다.
- 저장 경로가 비 ASCII 문자를 포함할 때 동작을 확인한다.
- `.tmp`와 `.bak` 복구 흐름을 깨지 않게 한다.
- 새 Component 저장 시 `GameObjectSnapshot`도 필요한지 확인한다.

현재 wide string을 ASCII로 좁히는 코드가 있어 한글 경로 안정성이 약하다. 저장/로드를 고치기 전에는 인코딩 정책을 먼저 정하는 것이 좋다.

## 신규 Component 추가 절차

현재 구조 기준 권장 절차는 다음과 같다.

1. `Engine/Scene` 아래에 Component 헤더/소스를 추가한다.
2. `Component`를 상속하고 필요한 데이터를 정의한다.
3. `GameObject::AddComponent<T>()`로 생성 가능한지 확인한다.
4. 에디터 Details 패널에 표시할지 결정한다.
5. 에디터에서 추가/삭제 버튼이 필요하면 `EditorLayer`에 연결한다.
6. Undo/Redo가 필요하면 Command 또는 Snapshot 지원을 추가한다.
7. 저장/로드가 필요하면 `SceneSerializer`에 저장과 로드를 모두 추가한다.
8. 렌더링이나 물리 처리에 필요하면 `Scene::Render()`, `PhysicsWorld` 등 실제 소비 지점을 연결한다.
9. 새 파일을 `.vcxproj`에 포함한다.

새 Component 파일만 추가하고 소비 지점을 연결하지 않으면 실제 기능으로 동작하지 않는다.

## 신규 AssetType 추가 절차

현재 AssetType은 `Unknown`, `Mesh`, `Texture`, `Material`, `Scene`, `Prefab`이다.

새 타입 추가 시 다음을 확인한다.

1. `AssetType.h/.cpp`에 enum과 문자열 변환을 추가한다.
2. `AssetTypeFromPath()`에 확장자 매핑을 추가한다.
3. `AssetDatabase::ScanAssets()`에서 `.meta` 생성이 되는지 확인한다.
4. Content Browser에서 표시/클릭/드래그 동작이 필요한지 확인한다.
5. 실제 로더 또는 생성기를 ResourceManager나 별도 시스템에 추가한다.
6. GUID 기반 조회가 필요한지 결정한다.
7. 삭제/이름 변경/이동 시 참조 복구 정책을 정한다.

타입만 추가하고 로더를 추가하지 않으면 에셋 브라우저에는 보이지만 실제 사용은 실패할 수 있다.

## 신규 Editor Panel 추가 절차

현재 에디터 패널은 주로 `EditorLayer`가 직접 그린다.

새 패널 추가 시 권장 절차는 다음과 같다.

1. 패널 상태를 `EditorLayer` 멤버로 둘지 별도 클래스로 분리할지 결정한다.
2. 메뉴 `Window` 또는 툴바에서 표시 토글을 연결한다.
3. ImGui Begin/End 짝을 엄격히 맞춘다.
4. 입력 포커스가 필요한 패널이면 `InputBlocked`와 충돌 여부를 확인한다.
5. Scene/Asset을 변경하면 DirtyManager와 CommandManager를 갱신한다.
6. 저장 가능한 UI 상태인지, 매 실행 초기화 상태인지 결정한다.

EditorLayer가 이미 커지고 있으므로, 새 기능은 가능하면 별도 클래스로 분리하는 편이 유지보수에 유리하다.

## 빌드 깨짐을 줄이는 작업 순서

1. 헤더 의존성을 최소화한다.
2. 전방 선언 가능한 타입은 전방 선언을 우선 고려한다.
3. 새 `.cpp` 파일을 프로젝트 파일에 포함한다.
4. Debug x64 빌드부터 확인한다.
5. 링크 에러가 나면 `.vcxproj` 포함 여부와 namespace를 먼저 확인한다.
6. 런타임 에러가 나면 Application 초기화 순서와 Renderer 초기화 순서를 확인한다.
7. 에셋 로딩 에러가 나면 working directory와 `Assets` 상대 경로를 확인한다.
8. 저장/로드 문제는 먼저 새 파일이 아닌 기존 씬 파일로 재현한다.

## 확인 필요 항목

- D3D12 backend를 계속 유지할지, 명확히 실험 코드로 둘지 결정 필요
- `Assets/Scenes/Test.scene` 기본 경로와 실제 존재하는 씬 파일 불일치 정리 필요
- AudioSystem의 현재 목표 범위 결정 필요
- Static Mesh Editor의 미리보기와 실제 렌더링 통합 여부 결정 필요
- Save/Load 포맷 버전 정책 필요

## 관련 파일

- `DirectEngine_JAEMIN/main.cpp`
- `DirectEngine_JAEMIN/Engine/Core/Application.cpp`
- `DirectEngine_JAEMIN/Engine/Graphics/Renderer/Renderer.cpp`
- `DirectEngine_JAEMIN/Engine/Graphics/D3D11/*`
- `DirectEngine_JAEMIN/Engine/Graphics/D3D12/*`
- `DirectEngine_JAEMIN/Engine/Scene/*`
- `DirectEngine_JAEMIN/Engine/Scene/Serialization/SceneSerializer.cpp`
- `DirectEngine_JAEMIN/Engine/Resource/ResourceManager.cpp`
- `DirectEngine_JAEMIN/Engine/Asset/AssetDatabase.cpp`
- `DirectEngine_JAEMIN/Engine/Editor/EditorLayer.cpp`
- `DirectEngine_JAEMIN/Engine/Editor/Command/*`

## 유지보수 시 주의점

- 현재 구조는 에디터와 런타임이 강하게 붙어 있는 부분이 있으므로, 새 기능을 추가할 때 의존성 방향을 먼저 확인해야 한다.
- 저장/로드, Undo/Redo, Dirty State는 서로 다른 코드 경로에 흩어져 있어 한 곳만 수정하면 상태가 어긋날 수 있다.
- 리소스 로딩 실패가 fallback으로 가려질 수 있으므로 로그를 꼭 확인해야 한다.
- 렌더링 변경은 C++ 구조체와 HLSL 구조체를 항상 한 쌍으로 검토해야 한다.

## 앞으로 개선할 점

- EditorLayer 기능 분리
- SceneSerializer 버전 도입
- Asset GUID 기반 참조 복구 강화
- D3D12 backend 상태 명확화
- 저장/로드와 리소스 로딩 회귀 테스트 추가
- Runtime과 Editor 의존성 경계 정리
