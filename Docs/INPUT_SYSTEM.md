# Input System

이 문서는 현재 코드에서 확인된 입력 처리 구조를 설명한다.

## 현재 구현 상태

입력 시스템은 `Engine::Input::Input` 클래스 하나가 중심이다. `Application`이 `Input` 인스턴스를 보유하고, 초기화 시 정적 active input으로 등록한다. Win32 메시지는 `Win32Window`에서 처리되며, 키보드/마우스 이벤트가 `Input::HandleKeyDown()`, `HandleKeyUp()`, `HandleMouseMove()`, `HandleMouseButtonDown()`, `HandleMouseButtonUp()` 같은 정적 함수로 전달된다.

현재 입력 시스템은 에디터 조작, 에디터 카메라, 플레이 모드 카메라 전환 등에 사용된다. 별도의 게임용 Action Mapping 시스템은 확인되지 않았다.

## 입력 데이터 구조

`Input`은 현재 프레임과 이전 프레임 상태를 배열로 보관한다.

```cpp
std::array<bool, 256> m_currentKeys;
std::array<bool, 256> m_previousKeys;
std::array<bool, 3> m_currentMouseButtons;
std::array<bool, 3> m_previousMouseButtons;
```

마우스 위치와 델타도 내부에 저장한다.

- `m_mouseDeltaX`
- `m_mouseDeltaY`
- `m_lastMouseX`
- `m_lastMouseY`
- `m_hasLastMousePosition`

프레임 시작 시 `BeginFrame()`이 이전 상태를 갱신하고 마우스 델타를 초기화하는 구조다.

## 키보드 입력

코드에서 정의된 주요 KeyCode는 다음과 같다.

- `KeyW`
- `KeyA`
- `KeyS`
- `KeyD`
- `KeyQ`
- `KeyE`
- `KeyControl`
- `KeyShift`
- `KeyAlt`
- `KeyEscape`
- `KeyDelete`

조회 함수는 다음처럼 현재 상태, 이번 프레임 눌림, 이번 프레임 떼짐을 나누어 제공한다.

```cpp
bool GetKey(int keyCode) const;
bool GetKeyDown(int keyCode) const;
bool GetKeyUp(int keyCode) const;
bool GetKeyRaw(int keyCode) const;
```

`Raw`가 붙은 함수는 입력 차단 상태와 관계없이 원본 상태를 확인하기 위한 용도로 보인다.

## 마우스 입력

마우스 버튼은 왼쪽, 오른쪽, 가운데 버튼을 지원한다.

```cpp
enum MouseButton
{
    MouseButtonLeft = 0,
    MouseButtonRight = 1,
    MouseButtonMiddle = 2
};
```

조회 함수는 키보드와 비슷하게 현재 상태, 눌림, 떼짐, Raw 조회를 제공한다.

```cpp
bool GetMouseButton(MouseButton button) const;
bool GetMouseButtonDown(MouseButton button) const;
bool GetMouseButtonUp(MouseButton button) const;
bool GetMouseButtonRaw(MouseButton button) const;
```

마우스 델타와 위치는 다음 함수로 조회한다.

- `GetMouseDeltaX()`
- `GetMouseDeltaY()`
- `GetMouseX()`
- `GetMouseY()`

## 입력 차단과 마우스 캡처

`Input`은 두 가지 상태 플래그를 가진다.

- `m_mouseCaptured`
- `m_inputBlocked`

`SetMouseCaptured()`는 카메라 조작이나 UI 포커스 상황에 따라 마우스 입력 캡처 여부를 제어하는 용도로 사용된다.

`SetInputBlocked()`는 ImGui UI 조작 중 게임/에디터 입력이 동시에 처리되는 문제를 막기 위한 구조로 보인다. 단, 이 동작의 정확한 정책은 `EditorLayer`와 `Application`의 호출 지점을 함께 확인해야 한다.

## 에디터 입력

확인된 에디터 단축키와 조작은 `EditorLayer`에서 처리된다.

- `P`: Play Mode 토글
- `Ctrl + Z`: Undo
- `Ctrl + Shift + Z`: Redo
- `Ctrl + Y`: Redo
- `Ctrl + S`: Save
- `Ctrl + Shift + S`: Save As
- `Ctrl + N`: New Scene
- `Ctrl + O`: 선택된 Scene 열기
- `Delete`: 선택 오브젝트 삭제
- `F2`: 오브젝트/에셋 이름 변경
- 우클릭 드래그: 에디터 카메라 조작
- `Alt` + 기즈모 드래그: 복제 후 드래그

이 목록은 코드에서 확인된 조작이다. 단축키 UI 표시와 실제 동작이 항상 1:1로 유지되는지는 수정 시 다시 확인해야 한다.

## 카메라 조작

카메라 입력은 두 경로가 있다.

- `Scene::Camera::UpdateFromInput()`
- `Editor::EditorCamera`

일반 카메라 입력에서는 `W/A/S/D` 이동, 마우스 델타 회전, `Shift` 가속이 확인된다. 에디터 카메라는 `EditorLayer`와 함께 뷰포트 상태, 우클릭 캡처, 플레이 모드 전환과 연결된다.

## 에디터 입력과 게임 입력 분리 여부

현재 코드 기준으로 완전한 게임 입력 레이어와 에디터 입력 레이어가 분리되어 있다고 보기는 어렵다. 하나의 `Input` 인스턴스를 중심으로 하고, `InputBlocked`, UI 포커스, 플레이 모드 여부를 통해 분기한다.

따라서 향후 실제 게임 로직 입력을 추가할 때는 에디터 단축키와 게임 입력이 충돌하지 않도록 명확한 라우팅 정책이 필요하다.

## 현재 한계

- Action Mapping 또는 Input Binding 테이블은 확인되지 않았다.
- 게임 입력과 에디터 입력의 책임 경계가 아직 약하다.
- Raw 입력과 차단 입력의 사용 기준을 문서화하거나 코드에서 명확히 강제하지 않는다.
- 키 코드는 Win32 가상 키 코드에 직접 가까운 형태로 사용된다.
- 게임패드, IME 텍스트 입력, 마우스 휠 입력은 이 문서 작성 범위에서 구현 사용을 확인하지 못했다.

## 관련 파일

- `DirectEngine_JAEMIN/Engine/Input/Input.h`
- `DirectEngine_JAEMIN/Engine/Input/Input.cpp`
- `DirectEngine_JAEMIN/Engine/Core/Application.cpp`
- `DirectEngine_JAEMIN/Engine/Platform/Windows/Win32Window.cpp`
- `DirectEngine_JAEMIN/Engine/Scene/Camera.h`
- `DirectEngine_JAEMIN/Engine/Scene/Camera.cpp`
- `DirectEngine_JAEMIN/Engine/Editor/EditorCamera.h`
- `DirectEngine_JAEMIN/Engine/Editor/EditorCamera.cpp`
- `DirectEngine_JAEMIN/Engine/Editor/EditorLayer.cpp`

## 유지보수 시 주의점

- 새 단축키를 추가할 때는 ImGui 입력 포커스와 충돌하지 않는지 확인해야 한다.
- 게임 플레이 입력을 추가할 때는 에디터 모드, 플레이 모드, UI 포커스 상태를 함께 고려해야 한다.
- `BeginFrame()` 호출 순서가 어긋나면 `GetKeyDown()`/`GetKeyUp()` 결과가 깨질 수 있다.
- Raw 입력 함수는 입력 차단을 우회하므로 에디터 UI 조작 중 오작동을 만들 수 있다.

## 앞으로 개선할 점

- 에디터 입력과 게임 입력을 별도 라우터나 컨텍스트로 분리한다.
- Action Mapping 시스템을 추가해 키 코드를 직접 쓰는 구간을 줄인다.
- 마우스 휠, 텍스트 입력, 게임패드 지원 여부를 정리한다.
- 단축키 목록을 코드와 문서에서 동기화할 수 있는 구조를 검토한다.
