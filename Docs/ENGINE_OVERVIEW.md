# ENGINE OVERVIEW

## 프로젝트 목적

이 프로젝트는 C++17과 DirectX 기반으로 작성된 초기 단계의 게임 엔진/에디터 프로젝트다. 현재 코드 기준으로는 게임 콘텐츠보다 엔진 기초 구조와 에디터 워크플로우를 만드는 데 초점이 있다.

실행 진입점은 `main.cpp`이며, `Engine::Core::Application`이 윈도우, 렌더러, 오디오 시스템, 씬, 에디터, ImGui를 생성하고 메인 루프를 관리한다.

```cpp
Engine::Core::Application application;
application.Initialize();
application.Run();
application.Shutdown();
```

## 현재 개발 단계

현재 구조는 "런타임 + 에디터 통합 애플리케이션" 형태다. D3D11 렌더링, 씬/오브젝트/컴포넌트, 에셋 스캔, 씬 저장/로드, ImGui 에디터가 실제 코드에 연결되어 있다.

반면 D3D12, Audio, ShadowMap, Prefab, glTF/FBX 로딩 등은 파일이나 enum, 일부 함수가 존재하지만 현재 실행 흐름에서 완성된 기능으로 보기는 어렵다. 이 문서에서는 이런 항목을 "구현 확인 불가" 또는 "부분 구현"으로 구분한다.

## 실제 확인된 주요 기능

- Win32 창 생성과 메시지 루프
- 키보드/마우스 입력 처리
- D3D11 기반 RHI 백엔드
- Mesh/Material/Texture/Shader 리소스 생성 및 캐싱
- OBJ 메시 로딩
- WIC 기반 이미지 로딩
- Scene, GameObject, Component 구조
- MeshRenderer, Light, Collider, PostProcess, PlayerStart 컴포넌트
- 커스텀 `.scene` 저장/로드
- 커스텀 `.mat` 저장/로드
- `.meta` 기반 AssetDatabase와 GUID 관리
- ImGui 기반 에디터 UI
- Content Browser, Outliner, Details 패널
- 일부 Undo/Redo 명령
- Debug line 기반 콜라이더/기즈모 표시
- Static Mesh Editor의 와이어프레임/Bounds/Collider sidecar 편집
- PostProcessComponent 기반 후처리 적용

## 전체 시스템 구성

```text
main
  -> Application
      -> Win32Window
      -> Input
      -> Renderer
          -> RHI Factory
              -> D3D11Device
          -> ResourceManager
          -> DebugRenderer
          -> ShadowMap
          -> PostProcessStack
      -> Scene
          -> GameObject
              -> Component
      -> EditorLayer
          -> AssetDatabase
          -> CommandManager
          -> StaticMeshEditor
      -> ImGuiDebugUI
      -> AudioSystem
      -> GameApp
```

실제 런타임 렌더링 API는 `Application::Initialize()`에서 `RHI::GraphicsAPI::D3D11`로 고정되어 있다. D3D12 코드는 프로젝트에 포함되어 컴파일되지만 현재 앱에서 선택되지 않는다.

## 현재 한계

- 초기 로드 경로가 `Assets/Scenes/Test.scene`인데 현재 프로젝트 파일 목록에는 해당 씬 파일이 없다.
- `DirectEngine_JAEMIN.vcxproj`에 등록된 일부 에셋(`Default.mat`, `TestTextured.mat`, `Test.scene`)이 실제 폴더에서 확인되지 않았다.
- `EditorLayer.cpp`에 에디터 UI, 파일 IO, 에셋 처리, 기즈모, 저장/로드, 모델 콜라이더 편집이 집중되어 있다.
- D3D12는 대부분의 RHI 함수가 no-op 또는 `nullptr` 반환이다.
- PhysicsWorld는 매 프레임 생성되어 오버랩을 계산하지만 결과가 게임 로직으로 전달되지 않는다.
- AudioSystem은 초기화되지만 현재 코드에서 실제 재생 호출 경로가 확인되지 않았다.
- Undo/Redo는 일부 오브젝트 작업에만 적용된다.
- 커스텀 텍스트 포맷 파서는 ASCII 변환을 사용해 비 ASCII 경로/이름에 취약하다.

## 앞으로의 방향

우선순위는 현재 있는 기능을 안정화하고, 실제 사용되는 경로와 미사용 스캐폴드를 분리하는 것이다.

1. 프로젝트 파일과 실제 에셋 경로 불일치 정리
2. D3D11 렌더링 경로 안정화
3. Scene/Asset 저장 로드 테스트 강화
4. EditorLayer 책임 분리
5. Undo/Redo 적용 범위 확장
6. D3D12, Audio, Shadow, Prefab 등 부분 구현 기능의 방향 결정

## 관련 파일

- `DirectEngine_JAEMIN/main.cpp`
- `DirectEngine_JAEMIN/Engine/Core/Application.h`
- `DirectEngine_JAEMIN/Engine/Core/Application.cpp`
- `DirectEngine_JAEMIN/DirectEngine_JAEMIN.vcxproj`
- `DirectEngine_JAEMIN/Game/GameApp.cpp`

## 유지보수 시 주의점

- 파일이 존재한다고 해서 실제 기능으로 문서화하거나 확장하지 않는다. 호출 경로를 먼저 확인한다.
- 런타임 경로는 현재 D3D11이다. D3D12 관련 변경은 별도 안정화 없이는 기본 경로로 전환하지 않는다.
- 에디터 기능 추가 시 `EditorLayer.cpp`가 더 커지지 않도록 작은 패널/서비스로 분리하는 방향을 우선 고려한다.
- 저장/로드 포맷 변경은 기존 `.scene`, `.mat`, `.colliders`와 호환성을 확인해야 한다.

## 앞으로 개선할 점

- `Assets/Scenes/Test.scene` 경로 문제 해결
- 프로젝트 파일의 에셋 항목과 실제 에셋 정합성 점검
- 기능별 문서와 실제 코드 경로를 지속적으로 동기화
- 빌드/실행 자동 검증 절차 추가
