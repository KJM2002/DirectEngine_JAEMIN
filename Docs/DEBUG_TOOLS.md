# Debug Tools

이 문서는 현재 코드에서 확인된 디버그, 통계, 시각화 도구를 설명한다.

## 현재 구현 상태

확인된 디버그 도구는 다음과 같다.

- 로그 출력
- FPS/통계 기반 윈도우 타이틀 갱신
- ImGui 기반 에디터/디버그 UI 프레임 관리
- DebugRenderer 라인 렌더링
- Collider, Gizmo, Grid, Axis 표시
- Static Mesh Editor의 미리보기 정보 표시

전용 프로파일러, GPU 타이머, 로그 패널, 렌더 패스 디버거는 현재 코드 기준으로 구현 확인되지 않았다.

## 로그 시스템

`Engine::Core::Log`는 Info, Warning, Error 성격의 로그를 콘솔 출력으로 기록한다. 여러 시스템에서 로딩 실패, 에셋 스캔, 파일 저장 실패 등을 알릴 때 사용된다.

로그는 유지보수에 중요하지만, 현재 구조는 콘솔 출력 중심으로 보이며 파일 로그, 에디터 로그 패널, 로그 필터링 시스템은 확인되지 않았다.

## DebugStats

렌더링과 애플리케이션 상태 통계는 `DebugStats` 구조로 정리된다.

```cpp
struct DebugStats
{
    float fps;
    float deltaTime;
    std::uint32_t objectCount;
    std::uint32_t drawCallCount;
    std::uint32_t triangleCount;
    Math::Vector3 cameraPosition;
};
```

`DebugUI`는 이 값을 기반으로 윈도우 타이틀을 일정 주기마다 갱신한다.

## ImGuiDebugUI

`ImGuiDebugUI`는 ImGui 초기화, Win32 메시지 처리, 프레임 시작/종료를 담당한다.

확인된 역할은 다음과 같다.

- ImGui Win32/DX11 backend 초기화
- Win32 메시지를 ImGui에 전달
- 매 프레임 ImGui begin/end 처리
- shutdown 처리

실제 에디터 패널 구성은 대부분 `EditorLayer`가 담당한다.

## DebugRenderer

`Renderer::DebugRenderer`는 라인 기반 디버그 렌더링을 담당한다.

```cpp
void DrawLine(const Math::Vector3& start, const Math::Vector3& end, const Math::Vector4& color);
void DrawAABB(const Math::Vector3& center, const Math::Vector3& size, const Math::Vector4& color);
void DrawBox(...);
void DrawCircle(...);
void DrawSphere(...);
void Flush(RHI::RHIContext& context, const Math::Matrix& view, const Math::Matrix& projection);
```

현재 확인된 사용처는 충돌체 표시, 기즈모/선 표시 계열이다. `Flush()`에서 쌓인 라인 정점을 GPU에 보내고 그린 뒤 `Clear()`로 정리하는 구조다.

## Collider 표시

`ColliderComponent`가 있는 오브젝트는 에디터에서 collider 표시가 가능하다. Box/Sphere 콜라이더 표시가 확인된다.

관련 흐름은 다음과 같다.

```text
EditorLayer option
-> Scene/GameObject ColliderComponent 탐색
-> DebugRenderer DrawAABB/DrawBox/DrawSphere
-> Renderer DebugRenderer Flush
```

Collider 표시 여부는 에디터 메뉴/툴바 옵션과 연결된다.

## Grid / Axis / Bounds

Static Mesh Editor의 preview viewport는 ImGui draw list 기반으로 grid, axis, bounds, collider 표시를 수행한다. 이 미리보기는 별도 RHI 렌더 타깃에 실제 메시를 렌더링하는 완성형 뷰포트라기보다, 메시 정보를 기반으로 한 에디터 미리보기/분석 UI에 가깝다.

확인된 표시 요소는 다음과 같다.

- Grid
- Axis
- Bounds
- Collider
- Mesh 통계 정보

## 통계 표시

에디터에는 stats 패널 또는 상태 표시가 있으며, 렌더러에서 draw call count, triangle count 같은 값을 갱신한다. `DebugStats`와 `Renderer`의 통계 갱신 흐름이 연결되어 있다.

다만 GPU 시간, pass별 draw call, material별 비용 같은 세부 프로파일링 값은 확인되지 않았다.

## 현재 한계

- 로그가 에디터 내부 패널로 통합되어 있지 않다.
- 로그 파일 저장 기능은 확인되지 않았다.
- DebugRenderer는 매 flush마다 라인 정점을 처리하는 단순 구조로 보이며 대량 디버그 라인 성능 정책은 없다.
- GPU 프로파일러나 RenderDoc 연동 기능은 확인되지 않았다.
- Static Mesh Editor 미리보기는 실제 렌더러 결과와 다를 수 있다.
- Debug 표시 옵션의 저장/복원 정책은 명확하지 않다.

## 관련 파일

- `DirectEngine_JAEMIN/Engine/Core/Log.h`
- `DirectEngine_JAEMIN/Engine/Core/Log.cpp`
- `DirectEngine_JAEMIN/Engine/Debug/DebugStats.h`
- `DirectEngine_JAEMIN/Engine/Debug/DebugUI.h`
- `DirectEngine_JAEMIN/Engine/Debug/DebugUI.cpp`
- `DirectEngine_JAEMIN/Engine/Debug/ImGuiDebugUI.h`
- `DirectEngine_JAEMIN/Engine/Debug/ImGuiDebugUI.cpp`
- `DirectEngine_JAEMIN/Engine/Graphics/Renderer/DebugRenderer.h`
- `DirectEngine_JAEMIN/Engine/Graphics/Renderer/DebugRenderer.cpp`
- `DirectEngine_JAEMIN/Engine/Editor/EditorLayer.cpp`
- `DirectEngine_JAEMIN/Engine/Editor/MeshEditor/StaticMeshPreviewViewport.h`
- `DirectEngine_JAEMIN/Engine/Editor/MeshEditor/StaticMeshPreviewViewport.cpp`
- `DirectEngine_JAEMIN/Engine/Physics/ColliderComponent.h`

## 유지보수 시 주의점

- DebugRenderer는 렌더링 파이프라인과 셰이더 리소스에 연결되어 있으므로, RHI 입력 레이아웃이나 셰이더를 바꾸면 함께 확인해야 한다.
- Collider 표시와 실제 PhysicsWorld 판정 로직이 어긋나면 디버그 뷰가 잘못된 판단을 유도할 수 있다.
- 통계 값은 렌더러가 갱신하는 값만 신뢰해야 하며, 에디터 표시만 보고 기능 동작을 단정하면 안 된다.
- 로그를 에디터 UI에 연결할 경우 멀티스레드/수명/문자 인코딩 정책을 먼저 정해야 한다.

## 앞으로 개선할 점

- 에디터 로그 패널을 추가한다.
- 파일 로그와 로그 레벨 필터를 도입한다.
- GPU/CPU 프레임 타이밍을 분리해 표시한다.
- Render pass별 draw call/triangle 통계를 추가한다.
- DebugRenderer 대량 라인 처리와 lifetime 옵션을 정리한다.
- Static Mesh Editor preview를 실제 렌더 타깃 기반으로 확장할지 검토한다.
