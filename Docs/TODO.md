# TODO

마지막 갱신: 2026-07-07

## 완료된 최근 항목

- PBR material 속성 확장: roughness, metallic, normal map, normal strength
- Material Editor 추가
- Material Editor texture drag/drop 복구
- Material Editor 값 live sync
- Inspector material slot을 `.mat` asset 중심으로 변경
- material 적용 시 per-object clone으로 주변 오브젝트까지 같이 바뀌는 문제 수정
- Content Browser asset delete 추가
- texture thumbnail 표시
- Inspector component add/remove
- Collider 표시 기본 off
- 빈 공간 클릭 시 gizmo 비활성화
- Selection Outline screen-space pass 추가
- Debug line selection outline과 screen-space outline 옵션 분리
- Move gizmo 표시 문제 수정
- built-in primitive mesh 5종 추가
- fallback 시작 씬을 정지 Cube 1개로 정리
- fallback 시작 카메라를 큐브 우측면 사선 구도로 조정
- Cylinder/Cone cap winding 수정

## Priority 0 - 안정성

- `.scene` 저장/로드 회귀 테스트 작성
- `.mat` 저장/로드 회귀 테스트 작성
- Material Editor에서 texture slot 변경 후 저장/재로드 검증 자동화
- Asset delete/rename 후 scene reference 처리 테스트
- ResourceManager fallback 사용 시 editor notification 강화
- Debug x64 외 Release x64 빌드 확인

## Priority 1 - 에디터 구조

- `EditorLayer.cpp`를 패널별 클래스로 분리
- Content Browser 로직 분리
- Inspector 로직 분리
- Material Editor 로직 분리
- Selection/Gizmo 입력 로직 분리
- Dirty state와 Command state 분리

## Priority 2 - Material 워크플로우

- Inspector에 Asset Material / Instance Override 상태 표시
- Override material을 새 `.mat`으로 저장하는 기능
- Override를 원본 material asset에 적용하는 기능
- Material Editor unsaved change prompt
- Material preview sphere 또는 plane 추가

## Priority 3 - Asset Pipeline

- AssetDatabase가 인식하는 mesh 확장자와 실제 loader 지원 범위 정리
- glTF/FBX 로더 도입 여부 결정
- GUID 기반 scene reference 복구 강화
- `.meta` migration/version 정책 추가
- asset import 실패 UI 개선

## Priority 4 - Rendering

- ShadowMap 활성화 정책 결정
- PostProcess stack 우선순위 명문화
- SelectionOutline pass 성능 확인
- DebugRenderer dynamic buffer 재사용
- transparent material 정책 추가
- render pass/frame graph 구조 검토

## Priority 5 - Scene / Game Framework

- Prefab MVP
- Play Mode scene isolation
- GameMode/Pawn/Controller 도입 여부 결정
- PlayerStart workflow 정리
- hierarchy 편집 UI 강화

## Priority 6 - Tooling

- editor log panel
- pass별 draw call/triangle 통계
- CPU/GPU frame time 분리
- save/load 자동 검증 스크립트
- docs와 코드 변경을 함께 검증하는 체크리스트
