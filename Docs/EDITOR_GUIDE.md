# EDITOR GUIDE

## 에디터 존재 여부

현재 코드 기준으로 ImGui 기반 에디터가 실제로 존재한다. `Application`은 `EditorLayer`와 `ImGuiDebugUI`를 초기화하고, 매 프레임 `EditorLayer::Update()`와 `EditorLayer::Render()`를 호출한다.

```text
Application::Render
  -> ImGuiDebugUI::BeginFrame
  -> EditorLayer::Render
  -> ImGuiDebugUI::EndFrame
```

## 에디터 실행 방법

별도 에디터 실행 파일은 없다. `DirectEngine_JAEMIN.exe` 실행 시 에디터 UI가 함께 뜨는 구조다.

초기화 순서:

```text
Application::Initialize
  -> EditorLayer::Initialize
  -> ImGuiDebugUI::Initialize
```

에디터는 D3D11 ImGui backend를 사용한다. `ImGuiDebugUI::Initialize()`는 D3D11 device/context native handle을 필요로 한다.

## 주요 패널

### Main Menu Bar

`EditorLayer::Render()`에서 `ImGui::BeginMainMenuBar()`로 구성된다.

확인된 메뉴:

- `Scene`
- `Edit`
- `Window`

### Main Toolbar

`RenderObjectTools()`에서 그린다.

확인된 버튼:

- `Create Empty`
- `Create Cube`
- `Create Post Process`
- `Create Player Start`
- `Delete Selected`
- Add Component: `Mesh Renderer`, `Collider`, `Light`, `Post Process`, `Player Start`
- Gizmo mode: `Move`, `Rotate`, `Scale`

### Viewport Toolbar

현재 씬 이름, 렌더링 모드 텍스트, 스냅 값, 카메라 위치를 표시한다.

### Outliner

`Scene::GetGameObjects()`를 순회해 GameObject 목록을 표시한다. 선택, Shift 다중 선택, F2 이름 변경이 연결되어 있다.

### Details

선택된 GameObject의 Transform과 컴포넌트를 편집한다.

확인된 편집 대상:

- Transform position/rotation/scale
- MeshRendererComponent
- Material base color, roughness, metallic, texture slots
- LightComponent
- ColliderComponent
- PostProcessComponent
- PlayerStartComponent

### Viewport Stats

FPS, delta time, object count, draw calls, triangle count, camera position, editor/play mode를 표시한다.

### Content Browser

`RenderAssetBrowser()`에서 구현된다. `AssetDatabase::ScanAssets("Assets")` 결과를 기반으로 폴더/에셋을 표시한다.

지원되는 것으로 확인된 에셋 동작:

- Refresh
- Import Model: OBJ 파일만 지원
- Save Scene
- Create Scene
- Create Material
- Delete Material
- Rename Asset
- `.obj` 클릭 시 선택 오브젝트 또는 새 오브젝트에 Mesh 적용
- `.mat` 클릭 시 선택 오브젝트에 Material 적용
- 이미지 파일 클릭/드래그로 Material texture slot 적용
- `.scene` 클릭 시 씬 열기 요청
- `.obj` 더블클릭 시 `StaticMeshEditor` 열기

## 메뉴 기능

### Scene 메뉴

- `New Scene`: 현재 씬이 dirty이면 저장 확인 후 새 씬 생성
- `Open Selected Scene`: 선택된 scene asset 열기
- `Save`: 현재 씬 저장
- `Save As`: 선택 폴더와 입력 이름 기준으로 저장
- `Exit`: dirty scene이면 저장 확인 후 종료 요청
- `Play Mode`: `m_playMode` 토글
- `Show Colliders`: 콜라이더 표시 토글
- `Asset Browser`: Content Browser 표시 토글
- `Gizmo Tools`: 기즈모 UI 표시 토글

### Edit 메뉴

- Undo
- Redo

Undo/Redo는 `CommandManager`를 통해 수행된다. 현재 모든 에디터 변경이 명령화되어 있지는 않다.

### Window 메뉴

- `Static Mesh Editor`
- `Reset Layout`

`Static Mesh Editor`는 선택된 `.obj`, `.gltf`, `.glb`, `.fbx`에 대해 열 수 있게 UI 조건이 되어 있으나 실제 Mesh 로딩은 현재 OBJLoader 기반이다. OBJ 외 포맷은 구현 확인이 필요하다.

## 단축키

코드에서 확인된 단축키:

- `P`: Play Mode 토글
- `Ctrl+Z`: Undo
- `Ctrl+Shift+Z`: Redo
- `Ctrl+Y`: Redo
- `Ctrl+S`: Save
- `Ctrl+Shift+S`: Save As
- `Ctrl+N`: New Scene
- `Ctrl+O`: selected scene open
- `Delete`: selected object delete
- `F2`: object 또는 asset rename
- 마우스 좌클릭: viewport selection
- 마우스 우클릭 hold: editor camera 조작 허용
- 기즈모 축 드래그: transform 수정
- Alt + gizmo drag: duplicate 후 drag 시작

## 씬 뷰포트

별도 scene render target UI가 아니라, 실제 백버퍼 렌더링 위에 ImGui 패널들이 올라가는 구조다. 오브젝트 선택은 마우스 위치에서 카메라 ray를 생성해 mesh bounds 또는 PhysicsWorld raycast로 판정한다.

기즈모는 `Renderer::RenderSelectionGizmo()`와 `DebugRenderer`를 통해 실제 라인 렌더링으로 그려진다.

## 인스펙터

Details 패널은 선택된 GameObject를 직접 수정한다. Transform 편집은 `TransformCommand`로 묶여 Undo/Redo에 들어간다. 하지만 Material 값, Light 값, Collider 값 등은 현재 명령화가 일관되게 적용되어 있지 않고, 패널 focus/release 시 `scene.MarkDirty()`가 호출되는 구조다.

## 에셋 브라우저

에셋 브라우저는 `AssetDatabase`와 `ResourceManager`를 연결한다.

```text
EditorLayer::RefreshAssets
  -> AssetDatabase::ScanAssets("Assets")
  -> m_modelAssets / m_textureAssets / m_materialAssets / m_sceneAssets 구성

RenderAssetBrowser
  -> ResourceManager::SetAssetDatabase
  -> 에셋 클릭/드래그/생성/삭제
```

## 저장 / 열기

씬 저장과 열기는 `SceneSerializer`를 사용한다.

- `SaveCurrentScene()`
- `SaveCurrentSceneAs()`
- `RequestOpenScene()`
- `ExecuteOpenScene()`
- `RenderSavePromptDialog()`

Dirty scene에 대해 destructive action을 수행하기 전 저장 확인 팝업을 띄운다.

## Static Mesh Editor

`StaticMeshEditor`는 실제로 열리는 에디터 창이다. 기능은 다음과 같다.

- Mesh asset path/GUID 표시
- vertex/index/triangle/submesh/material slot 수 표시
- bounds 표시
- ImGui draw list 기반 wireframe preview
- grid/axis/bounds/collider 표시 토글
- collider sidecar 파일 로드/저장

주의할 점:

- 툴바의 `Save`, `Reimport` 버튼은 `BeginDisabled()`로 비활성화되어 있다.
- Preview는 실제 RHI offscreen render target이 아니라 ImGui draw list 기반 2D 와이어프레임이다.

## 구현 확인 불가 또는 부분 구현

- `EditorLayer::OpenModelEditor()` 기반 `Model Asset Editor` 함수는 존재하지만 현재 확인한 UI 호출 경로에서는 사용되지 않는다.
- `.gltf/.glb/.fbx`는 UI 조건과 AssetType에는 등장하지만 실제 로더는 OBJ 기반이다.
- 모든 property 변경이 Undo/Redo에 포함되지는 않는다.

## 관련 파일

- `DirectEngine_JAEMIN/Engine/Editor/EditorLayer.h`
- `DirectEngine_JAEMIN/Engine/Editor/EditorLayer.cpp`
- `DirectEngine_JAEMIN/Engine/Editor/Command/CommandManager.cpp`
- `DirectEngine_JAEMIN/Engine/Editor/MeshEditor/StaticMeshEditor.cpp`
- `DirectEngine_JAEMIN/Engine/Debug/ImGuiDebugUI.cpp`
- `DirectEngine_JAEMIN/Engine/Asset/AssetDatabase.cpp`

## 유지보수 시 주의점

- 에디터 UI 버튼이 있어도 실제 기능이 연결되어 있는지 함수 호출 경로를 확인해야 한다.
- 새 컴포넌트를 추가하면 Details 패널, SceneSerializer, GameObjectSnapshot, Command 계층까지 같이 확인한다.
- `EditorLayer.cpp`가 이미 크기와 책임이 크므로 신규 패널은 별도 클래스로 분리하는 방향이 좋다.
- Dirty state와 Undo/Redo가 분리되어 있어, 저장 확인 로직과 명령 스택이 어긋날 수 있다.

## 앞으로 개선할 점

- `EditorLayer` 기능별 분리
- Property 변경 Undo/Redo 확장
- Content Browser의 포맷 지원과 실제 로더 정합성 개선
- Static Mesh Editor의 비활성 Save/Reimport 기능 방향 결정
- Model Asset Editor 중복 코드 제거 또는 통합
- 에디터 레이아웃 저장/복원 정책 정리
