# Debug Tools

마지막 갱신: 2026-07-07

## Debug Stats

Viewport Stats는 다음 값을 표시한다.

- FPS
- Delta Time
- Object Count
- Draw Calls
- Triangle Count
- Camera Position
- Editor/Play Mode 상태

## DebugRenderer

라인 기반 디버그 렌더러다.

지원 기능:

- DrawLine
- DrawAABB
- DrawBox
- DrawCircle
- DrawSphere
- Flush

사용처:

- collider 표시
- grid / axis
- debug selection outline
- 일부 fallback gizmo line

Collider 표시는 기본 off다.

## Selection Outline Debug

기본 선택 외곽선은 screen-space pass를 사용한다. 필요하면 Scene 메뉴의 Selection Outline에서 Debug Lines 옵션을 켤 수 있다.

Screen-space outline이 꺼져 있거나 디버깅이 필요할 때 Debug Lines를 함께 사용할 수 있다.

## Static Mesh Editor Preview

Static Mesh Editor는 ImGui draw list 기반 preview를 제공한다.

- grid
- axis
- bounds
- collider wire
- collider gizmo

이 preview는 메인 viewport와 별도이며, static mesh collider sidecar 편집에 초점을 둔다.

## 주의

- DebugRenderer는 line 수가 많으면 비용이 커질 수 있다.
- Collider 표시와 selection debug line은 렌더링 상태 확인용이며 최종 selection outline은 별도 pass가 담당한다.
- GPU pass별 profiling은 아직 없다.
