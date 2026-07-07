# Engine Foundation Plan

마지막 갱신: 2026-07-07

## 현재 완료된 기반

- ObjectID / ComponentID
- ComponentRegistry 1차 구조
- GameObjectSnapshot 기반 command 복원
- Transform 일부 Undo/Redo
- Material base color / roughness / metallic 일부 Undo/Redo
- SceneCloner 기반 복제 구조
- MeshRenderer material path/GUID 보관
- Material asset 적용 시 per-object material instance clone
- Material Editor live sync
- Inspector component add/remove
- Content Browser asset delete/rename
- Selection Outline render pass
- built-in primitive mesh 5종

## 아직 미완성인 기반

- Prefab MVP
- TransactionCommand 통합
- EditorLayer 패널 분리
- GUID 기반 scene reference 복구 완성
- SceneSerializer component별 분리
- Play Mode scene isolation 완성
- Action Mapping 기반 input
- Asset import pipeline 명문화

## Material Asset / Instance 방향

현재 구현:

- `.mat` 적용 시 오브젝트마다 material clone
- clone에는 material path/GUID 유지
- Material Editor는 같은 path/GUID를 가진 clone들을 live sync
- Inspector에서 직접 값 편집 시 override로 보고 path/GUID 제거

다음 단계:

- UI에서 "Asset Material"과 "Instance Override" 상태를 명확히 표시
- override를 asset에 다시 적용하는 기능 추가
- asset dirty와 scene dirty를 분리

## Prefab 방향

권장 단계:

1. GameObjectSnapshot을 prefab 저장 단위로 사용
2. prefab root와 child hierarchy 저장
3. prefab instance link GUID 추가
4. override diff는 후순위로 미룸

## Transaction 방향

현재 CommandManager는 개별 command 중심이다. drag/slider/edit 종료 시점에서 하나의 command로 묶는 정책을 더 넓혀야 한다.

대상:

- Transform
- Material
- Collider
- Light
- PostProcess
- hierarchy 편집

## 우선순위

1. 저장/로드 안정화
2. Material asset/instance UI 명확화
3. EditorLayer 분리
4. Prefab MVP
5. Play Mode scene isolation
