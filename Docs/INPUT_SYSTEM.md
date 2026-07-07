# Input System

마지막 갱신: 2026-07-07

## 개요

입력은 `Engine::Input::Input`이 관리한다. Win32 메시와 프레임별 key/mouse 상태를 기반으로 editor와 play mode 입력을 처리한다.

## Editor 입력

- 좌클릭: viewport selection
- 우클릭 hold: editor camera 조작
- W: Move gizmo
- R: Rotate gizmo
- S: Scale gizmo
- P: Play Mode toggle
- Delete: 선택 오브젝트 삭제 또는 Content Browser asset 삭제
- F2: object/asset rename
- Ctrl+Z: Undo
- Ctrl+Shift+Z: Redo
- Ctrl+Y: Redo
- Ctrl+S: Save
- Ctrl+Shift+S: Save As
- Ctrl+N: New Scene
- Ctrl+O: selected scene open

## Mouse Capture

에디터 카메라는 우클릭 hold 중 mouse capture를 사용한다. ImGui가 입력을 capture 중이어도 우클릭 카메라 조작은 허용한다.

## Play Mode 입력

Play Mode에서는 active camera가 runtime input을 받는다. PlayerStart가 있으면 해당 위치와 설정에서 시작한다.

## Gizmo 입력

기즈모는 선택 오브젝트가 있을 때만 표시된다.

- W/R/S로 mode 변경
- axis hover/active 상태 추적
- drag 중 transform 변경
- Alt + drag는 duplicate 후 drag 시작

## 주의

에디터 입력과 게임 입력은 아직 완전히 분리된 action mapping 시스템이 아니다. 향후 editor/game input context를 분리하는 것이 좋다.
