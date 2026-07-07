# Editor Guide

마지막 갱신: 2026-07-07

## 개요

에디터는 ImGui 기반이며 `EditorLayer`가 대부분의 UI와 상호작용을 관리한다.

```text
Application::Render
  -> ImGuiDebugUI::BeginFrame
  -> EditorLayer::Render
  -> ImGuiDebugUI::EndFrame
```

## Main Menu

### Scene

- New Scene
- Open Selected Scene
- Save
- Save As
- Exit
- Play Mode
- Show Colliders
- Asset Browser
- Gizmo Tools
- Selection Outline
  - Screen Space
  - Debug Lines
  - Color
  - Width
  - Opacity

Collider 표시는 기본적으로 꺼져 있다.

### Edit

- Undo
- Redo

Command 기반으로 구현된 작업만 Undo/Redo 대상이다.

### Window

- Static Mesh Editor
- Reset Layout

## Main Toolbar

- Create Empty
- Shapes
  - Cube
  - Sphere
  - Cylinder
  - Cone
  - Plane
- Create Post Process
- Create Player Start
- Delete Selected
- Gizmo mode: Move / Rotate / Scale

Shapes는 built-in mesh를 사용하며 기본 MeshRenderer와 Collider를 함께 만든다. Sphere는 sphere collider, Plane은 얇은 box collider를 사용한다.

## Viewport 선택과 기즈모

- 좌클릭: 오브젝트 선택
- 빈 공간 클릭: selection 및 gizmo 비활성화
- W: Move gizmo
- R: Rotate gizmo
- S: Scale gizmo
- Alt + gizmo drag: duplicate 후 drag 시작
- Delete: selected object 삭제

선택 외곽선은 기본적으로 screen-space outline pass를 사용한다. Debug line outline은 옵션이다.

## Inspector

선택된 GameObject를 편집한다.

지원 항목:

- Transform position / rotation / scale
- MeshRenderer
- Material asset drag/drop
- Material instance 값 편집
  - Base Color
  - Roughness
  - Metallic
  - Normal Strength
- Collider
- Light
- PostProcess
- PlayerStart

Add Component 메뉴에서 Mesh Renderer, Collider, Light, Post Process, Player Start를 추가할 수 있다. 각 component header의 Remove 버튼으로 삭제할 수 있다.

Inspector에서 `.mat` asset을 MeshRenderer의 Material 슬롯에 drag/drop하면 선택 오브젝트에 material이 적용된다. 적용 시 material은 오브젝트별 인스턴스로 clone되며, 원본 asset path/GUID는 유지된다.

## Material Editor

`.mat` asset을 더블 클릭하거나 context menu에서 열 수 있다.

편집 가능 항목:

- Base Color
- Roughness
- Metallic
- Normal Strength
- Base Color Map
- Roughness Map
- Metallic Map
- Normal Map

텍스처는 Content Browser에서 각 슬롯으로 drag/drop한다. Material Editor에서 값이 바뀌면 같은 material path/GUID를 참조하는 씬 내 MeshRenderer material 인스턴스에 즉시 동기화된다. Save는 `.mat` 파일에 저장하고, Reload는 material cache를 비운 뒤 다시 로드한다.

Inspector에서 직접 material 값을 수정하면 해당 오브젝트는 editable instance override가 되며 material asset path/GUID가 끊긴다. 이 경우 asset editor의 live sync 대상에서 제외된다.

## Content Browser

기능:

- Refresh
- Import Model
- Save Scene
- Create Scene
- Create Material
- Rename Asset
- Delete Asset
- `.obj` asset 배치 또는 선택 오브젝트 mesh 교체
- `.mat` asset 적용
- texture thumbnail 표시
- texture drag/drop
- `.scene` 열기
- `.mat` 더블 클릭으로 Material Editor 열기
- `.obj` 더블 클릭으로 Static Mesh Editor 열기

지원 texture 확장자:

- `.png`
- `.jpg`
- `.jpeg`
- `.bmp`
- `.tga`

Mesh asset type은 `.obj`, `.gltf`, `.glb`, `.fbx`를 분류할 수 있지만 실제 로더는 현재 OBJ 중심이다.

## Static Mesh Editor

OBJ mesh preview와 collider sidecar 편집을 지원한다.

- mesh path/GUID 표시
- vertex/index/triangle/submesh/material slot 수 표시
- bounds 표시
- grid/axis/bounds/collider 표시 토글
- Box/Sphere collider 추가 및 편집
- `.colliders` sidecar 저장/로드

## 단축키

- P: Play Mode
- Ctrl+Z: Undo
- Ctrl+Shift+Z 또는 Ctrl+Y: Redo
- Ctrl+S: Save
- Ctrl+Shift+S: Save As
- Ctrl+N: New Scene
- Ctrl+O: selected scene open
- Delete: selected object 또는 focused Content Browser asset 삭제
- F2: object 또는 asset rename
- W/R/S: Move/Rotate/Scale gizmo

## 관련 파일

- `DirectEngine_JAEMIN/Engine/Editor/EditorLayer.cpp`
- `DirectEngine_JAEMIN/Engine/Editor/EditorLayer.h`
- `DirectEngine_JAEMIN/Engine/Editor/Command/*`
- `DirectEngine_JAEMIN/Engine/Editor/MeshEditor/*`
