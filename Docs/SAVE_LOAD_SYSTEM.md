# Save / Load System

이 문서는 현재 코드에서 확인된 씬 저장/로드 구조와 저장 안정성 관련 주의점을 정리한다.

## 현재 구현 상태

씬 저장과 로드는 `Engine::Scene::SceneSerializer`가 담당한다.

```cpp
class SceneSerializer
{
public:
    static bool LoadScene(const std::wstring& path, Scene& scene, Resource::ResourceManager& resourceManager);
    static bool SaveScene(const std::wstring& path, const Scene& scene);
};
```

저장 파일은 커스텀 텍스트 포맷이다. JSON, YAML, 바이너리 포맷은 현재 코드에서 확인되지 않았다.

## 저장 가능한 데이터

현재 저장 코드에서 확인된 데이터는 다음과 같다.

- Camera
- DirectionalLight
- GameObject 이름
- Transform position/rotation/scale
- MeshRendererComponent
- Mesh 경로와 GUID
- Material 경로와 GUID
- Material의 inline 속성
- LightComponent
- ColliderComponent
- PostProcessComponent
- PlayerStartComponent

`GameObject`의 모든 Component가 자동 저장되는 구조는 아니다. `SceneSerializer.cpp`에 명시적으로 처리 코드가 들어간 컴포넌트만 저장된다.

## 저장 파일 포맷

저장 파일은 섹션 기반 텍스트 구조다.

확인된 섹션은 다음과 같다.

- `[Camera]`
- `[DirectionalLight]`
- `[GameObject]`

각 섹션 아래에는 `key=value` 형태의 값이 기록된다. 예를 들어 메시 렌더러는 `component=MeshRenderer`와 함께 mesh/material 관련 값이 저장된다.

씬 포맷의 명시적인 버전 필드는 확인되지 않았다. `scene_name=SavedScene` 형태의 값은 기록되지만, 스키마 버전으로 보기는 어렵다.

## 로드 흐름

로드 흐름은 다음과 같다.

1. `SceneSerializer::LoadScene(path, scene, resourceManager)` 호출
2. `std::ifstream`으로 씬 파일 열기
3. 섹션과 key/value를 파싱
4. Camera, DirectionalLight, GameObject 데이터를 임시 구조에 누적
5. GameObject를 생성하고 Transform과 컴포넌트를 복원
6. Mesh/Material은 `ResourceManager`를 통해 로드
7. 로드 실패 시 일부 리소스는 fallback을 사용할 수 있음

로드 중 메시나 머티리얼이 없을 경우 built-in cube/default material이 들어갈 수 있는 흐름이 확인된다. 따라서 로드가 성공했다고 해서 모든 에셋 참조가 정상이라고 단정하면 안 된다.

## Save 흐름

저장 흐름은 안전 저장을 어느 정도 고려하고 있다.

1. 대상 경로의 부모 폴더 생성
2. `대상.scene.tmp` 파일에 먼저 저장
3. 기존 대상 파일이 있으면 `.bak` 백업 처리
4. 임시 파일을 대상 파일로 교체
5. 실패 시 가능한 범위에서 백업 복구 시도

확인된 경로 생성 방식은 다음과 같다.

- `tempPath = path + ".tmp"`
- `backupPath = path + ".bak"`

이 구조는 바로 원본 파일을 덮어쓰는 방식보다 안전하지만, 모든 실패 케이스가 완전히 원자적으로 처리된다고 보기는 어렵다. 특히 같은 드라이브/파일시스템 보장, 백업 복구 실패, 권한 문제는 별도 검증이 필요하다.

## Dirty State

Dirty 상태는 두 층이 있다.

- `Scene` 자체의 dirty flag
- `EditorDirtyManager`와 `CommandManager` revision 기반 dirty 판단

`EditorDirtyManager`는 현재 Scene과 AssetDatabase 포인터를 가지고 저장되지 않은 변경 여부를 확인한다.

```cpp
bool HasUnsavedSceneChanges() const;
bool HasUnsavedAssetChanges() const;
bool HasAnyUnsavedChanges() const;
```

`CommandManager`는 undo/redo revision과 saved revision을 관리한다.

```cpp
std::uint64_t GetCurrentRevision() const;
std::uint64_t GetSavedRevision() const;
void MarkSaved();
bool IsDirtySinceSave() const;
```

주의할 점은 모든 에디터 변경이 CommandManager를 통과하지 않는다는 점이다. Transform 변경, 생성, 삭제, 복제, 이름 변경은 Command로 확인되지만, 일부 머티리얼/컴포넌트 속성 변경은 직접 적용 후 dirty 표시하는 흐름이 섞여 있다.

## Save / Save As

에디터 메뉴와 단축키에서 Save, Save As가 확인된다.

- `Ctrl + S`: Save
- `Ctrl + Shift + S`: Save As

현재 기본 씬 경로는 `EditorLayer`에서 `Assets/Scenes/Test.scene`를 사용하는 흐름이 확인된다. 그러나 실제 파일 목록에서는 `Test.scene`이 없고, `NewScene.scene`, `Scene_One.scene`, `Scne_Two.scene`가 확인되었다. 이 상태에서 저장하면 새 `Test.scene`이 생성될 가능성이 있으므로 경로 정책을 정리해야 한다.

## 저장 실패 처리

`SaveScene()`은 파일 열기 실패, 쓰기 실패, filesystem 오류를 로그로 기록하고 `false`를 반환한다. `.tmp`와 `.bak`를 사용한 복구 시도도 존재한다.

다만 에디터 UI가 모든 실패 원인을 사용자에게 충분히 보여주는지는 별도 확인이 필요하다. 현재 문서에서는 `SceneSerializer` 내부의 반환값과 로그 처리까지만 구현 확인으로 본다.

## 위험 요소

`SceneSerializer`에는 wide string 경로를 ASCII 문자열로 좁히는 `ToNarrowAscii()` 계열 함수가 있다. ASCII 범위를 벗어난 문자는 `?`로 대체되는 구조다. 한글 사용자 경로, 한글 에셋 파일명, 비 ASCII 폴더명이 포함되면 파일 열기/저장/참조 경로가 깨질 위험이 있다.

또한 파서는 수동 문자열 파싱 기반이다. 잘못된 숫자, 누락 키, 예기치 않은 값에 대한 견고성이 충분한지는 추가 테스트가 필요하다.

## 현재 한계

- 씬 포맷 버전이 없다.
- 저장/로드 테스트 코드가 확인되지 않았다.
- 경로 문자열의 비 ASCII 처리가 위험하다.
- 모든 Component를 자동 직렬화하지 않는다.
- GUID가 저장되지만 로드 복구의 중심은 아직 경로 기반이다.
- 일부 PostProcess 저장/로드 값은 `postprocess_effect`를 `None`으로 저장하는 코드가 확인되어 의도 여부 확인이 필요하다.

## 관련 파일

- `DirectEngine_JAEMIN/Engine/Scene/Serialization/SceneSerializer.h`
- `DirectEngine_JAEMIN/Engine/Scene/Serialization/SceneSerializer.cpp`
- `DirectEngine_JAEMIN/Engine/Scene/Scene.h`
- `DirectEngine_JAEMIN/Engine/Scene/Scene.cpp`
- `DirectEngine_JAEMIN/Engine/Editor/EditorDirtyManager.h`
- `DirectEngine_JAEMIN/Engine/Editor/EditorDirtyManager.cpp`
- `DirectEngine_JAEMIN/Engine/Editor/Command/CommandManager.h`
- `DirectEngine_JAEMIN/Engine/Editor/Command/CommandManager.cpp`
- `DirectEngine_JAEMIN/Engine/Editor/EditorLayer.cpp`

## 유지보수 시 주의점

- 새 Component를 추가하면 `SceneSerializer` 저장과 로드를 모두 추가해야 한다.
- 저장 포맷 키 이름을 바꾸면 기존 `.scene` 파일 호환성이 깨질 수 있다.
- Dirty 표시만 하고 Undo Command를 만들지 않는 변경은 Undo/Redo와 저장 상태가 어긋날 수 있다.
- 경로 저장 로직을 수정할 때는 한글 경로와 공백 경로를 반드시 테스트해야 한다.
- Save 실패 시 `.tmp`와 `.bak` 잔여 파일이 남는 경우를 확인해야 한다.

## 앞으로 개선할 점

- 씬 포맷 버전 필드를 추가한다.
- 비 ASCII 경로를 안전하게 처리하도록 파일 입출력 방식을 정리한다.
- GUID 기반 로드 복구를 실제 로드 경로에 통합한다.
- 저장/로드 회귀 테스트를 만든다.
- Save 실패 원인을 에디터 UI에서 명확히 표시한다.
- Component별 직렬화 책임을 분리할지 검토한다.
