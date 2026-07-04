# TODO

이 TODO는 현재 코드 분석 결과를 기준으로 작성했다. 예시 기능 목록을 그대로 넣지 않고, 실제 프로젝트에서 확인된 위험과 미완성 지점을 우선순위로 정리했다.

## Priority 0 - 안정화

- `Assets/Scenes/Test.scene` 기본 경로와 실제 존재하는 씬 파일 불일치를 정리한다.
- `SceneSerializer`의 비 ASCII 경로 처리 문제를 해결한다.
- 씬 저장/로드 회귀 테스트를 만든다.
- `.scene` 저장 실패 시 `.tmp`와 `.bak` 복구 시나리오를 검증한다.
- ResourceManager 로딩 실패 시 fallback이 사용된 사실을 에디터에서 명확히 표시한다.
- AssetDatabase가 인식하는 Mesh 포맷과 ResourceManager가 실제 로드 가능한 포맷을 일치시킨다.
- DirectX11 리소스 resize, shutdown, fallback 경로에서 null 사용 가능성을 점검한다.
- 현재 Debug x64 기준 전체 빌드 경고와 링크 경고를 정리한다.

## Priority 1 - 구조 정리

- `EditorLayer.cpp`에 집중된 메뉴, 패널, Content Browser, Details, 저장 프롬프트 코드를 기능별 클래스로 분리한다.
- Runtime 코드와 Editor 코드의 의존성을 정리한다.
- `Scene::Render()`와 `Renderer`의 책임 경계를 정리한다.
- `ResourceManager`의 캐시 키 정책을 정리한다.
- 경로 기반 참조와 GUID 기반 참조의 우선순위를 문서화하고 코드에 반영한다.
- `Scene::MeshRenderer`와 `MeshRendererComponent`처럼 이름이 비슷하지만 실제 사용 여부가 다른 코드를 정리한다.
- D3D12 backend를 유지할지, 제거할지, 실험 코드로 명확히 격리할지 결정한다.
- AudioSystem의 현재 범위를 명확히 한다.

## Priority 2 - 저장 / 에셋 워크플로우

- `.scene` 포맷에 version 필드를 추가한다.
- Component별 직렬화 책임을 분리할지 검토한다.
- GUID 기반 로드 복구를 실제 Scene 로드 흐름에 통합한다.
- Asset rename/move 후 기존 씬 참조가 어떻게 동작하는지 테스트한다.
- `.meta` 파일 생성, 이동, 삭제 실패 시 UI 피드백을 강화한다.
- Material 수정 후 캐시 무효화와 저장 흐름을 정리한다.
- Static Mesh Editor의 collider sidecar 파일(`.colliders`) 저장/로드 정책을 문서화하거나 씬/에셋 포맷과 통합한다.

## Priority 3 - 에디터 기능

- Transform 외 머티리얼/컴포넌트 속성 변경도 Undo/Redo 정책을 통일한다.
- Dirty State가 Scene dirty, Command revision, Asset dirty 사이에서 어긋나지 않도록 정리한다.
- Content Browser의 지원 확장자와 실제 import 가능 포맷을 UI에서 구분한다.
- Save 실패, Load 실패, fallback 리소스 사용을 에디터 알림으로 표시한다.
- Static Mesh Editor의 비활성화된 Save/Reimport/Import Scale 버튼 정책을 결정한다.
- 기존 Model Asset Editor 관련 함수가 실제 UI에서 쓰이는지 확인하고, 미사용이면 정리한다.
- Play Mode에서 에디터 입력과 게임 입력의 경계를 명확히 한다.

## Priority 4 - 렌더링

- ShadowMap 활성화 경로를 실제 설정/UI와 연결할지 결정한다.
- PostProcess 설정 적용 우선순위를 정리한다.
- Material 텍스처 슬롯과 HLSL register 정책을 문서화한다.
- tangent를 실제 셰이더 입력으로 사용할지 결정하고 input layout과 HLSL을 맞춘다.
- Render Queue 정렬 기준과 투명/불투명 분리 정책을 준비한다.
- DebugRenderer 대량 라인 처리 성능을 점검한다.
- Static Mesh Editor preview를 실제 RHI 렌더링으로 확장할지 결정한다.
- Vertex Count 표시 기준을 에디터 UI에 설명하거나 이름을 명확히 한다.

## Priority 5 - 입력 / 게임 실행 구조

- 에디터 입력과 게임 입력을 컨텍스트 단위로 분리한다.
- Action Mapping 시스템 도입 여부를 결정한다.
- PlayerStartComponent가 실제 플레이 시작 위치로 사용되는 흐름을 만들지, 데이터 컴포넌트로만 둘지 결정한다.
- GameFramework 폴더가 비어 있는 현재 상태를 유지할지, GameMode/Pawn/Controller 구조를 도입할지 결정한다.
- Play In Editor의 목표 범위를 정의한다.

## Priority 6 - 디버그 / 품질 도구

- 에디터 로그 패널을 추가한다.
- 파일 로그 저장과 로그 레벨 필터를 추가한다.
- CPU/GPU 프레임 타이밍을 분리해서 표시한다.
- Draw call, triangle count 외 pass별 통계를 추가한다.
- 저장/로드, 에셋 스캔, 리소스 로딩에 대한 자동 검증 스크립트를 만든다.

## 확인 필요

- 현재 `Application::InitializeScene()`의 기본 씬 경로가 의도적으로 `Test.scene`인지 확인 필요
- `PostProcessComponent` 저장 시 `postprocess_effect=None`으로 기록하는 것이 의도인지 확인 필요
- D3D12 파일들이 장기 목표인지 과거 실험 코드인지 확인 필요
- AudioSystem이 실제 게임 기능 목표인지 placeholder인지 확인 필요
- Static Mesh Editor와 기존 Model Asset Editor 계열 함수의 관계 확인 필요

## 관련 파일

- `DirectEngine_JAEMIN/Engine/Core/Application.cpp`
- `DirectEngine_JAEMIN/Engine/Editor/EditorLayer.cpp`
- `DirectEngine_JAEMIN/Engine/Scene/Serialization/SceneSerializer.cpp`
- `DirectEngine_JAEMIN/Engine/Resource/ResourceManager.cpp`
- `DirectEngine_JAEMIN/Engine/Asset/AssetDatabase.cpp`
- `DirectEngine_JAEMIN/Engine/Graphics/Renderer/Renderer.cpp`
- `DirectEngine_JAEMIN/Engine/Graphics/Renderer/ShadowMap.cpp`
- `DirectEngine_JAEMIN/Engine/Graphics/Renderer/PostProcess.cpp`
- `DirectEngine_JAEMIN/Engine/Input/Input.cpp`
- `DirectEngine_JAEMIN/Engine/Editor/Command/*`

## 유지보수 시 주의점

- TODO를 구현하기 전에는 해당 항목이 실제 현재 코드에서 재현되는지 다시 확인해야 한다.
- 저장 포맷, AssetDatabase, ResourceManager를 동시에 건드리는 작업은 작은 단계로 나눠야 한다.
- 렌더링 변경은 D3D11 실행 경로 기준으로 먼저 검증해야 한다.

## 앞으로 개선할 점

- 이 TODO를 이슈 트래커나 작업 보드로 옮겨 담당자와 완료 기준을 붙인다.
- 각 Priority 0 항목에 재현 절차와 검증 절차를 추가한다.
- 문서와 코드가 어긋나지 않도록 주요 구조 변경 시 Docs를 함께 갱신한다.
