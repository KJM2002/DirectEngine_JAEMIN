# SCENE SYSTEM

## 씬 시스템 존재 여부

현재 코드 기준으로 `Scene` 시스템은 실제로 존재하고 사용된다. `Application`은 `Scene::Scene m_scene`을 소유하며, 업데이트와 렌더링에서 매 프레임 사용한다.

## Scene 역할

`Scene`은 다음 데이터를 관리한다.

- `std::vector<std::unique_ptr<GameObject>> m_gameObjects`
- active camera
- directional light
- scene file path
- dirty flag

주요 함수:

```cpp
GameObject& CreateGameObject(const std::string& name);
bool DestroyGameObject(GameObject* object);
void Update(float deltaTime);
void Render(Renderer::Renderer& renderer);
void Clear();
void EnsureDefaultCameraAndLight();
```

## 씬이 관리하는 데이터

### GameObject 목록

`Scene::CreateGameObject()`는 `unique_ptr<GameObject>`를 생성해 vector에 저장하고 참조를 반환한다.

### Camera

`Scene`은 `std::shared_ptr<Camera>`로 active camera를 저장한다. 카메라가 없으면 `EnsureDefaultCameraAndLight()`에서 기본 카메라를 생성한다.

### DirectionalLight

`Scene`은 전역 directional light도 보유한다. 이와 별개로 GameObject에 붙는 `LightComponent`도 존재한다.

## GameObject 구조

GameObject는 이름, Transform, Component 목록을 가진다.

```text
GameObject
  name
  Transform
  vector<shared_ptr<Component>>
```

컴포넌트 추가는 template 함수로 처리된다.

```cpp
T& AddComponent(Args&&... args);
T* GetComponent() const;
std::vector<T*> GetComponents() const;
void RemoveComponents();
```

컴포넌트 조회는 `dynamic_cast` 기반이다.

## Transform 구조

Transform은 position, rotation, scale을 가진다.

```cpp
struct Transform
{
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
    Matrix GetWorldMatrix() const;
};
```

월드 행렬은 다음 순서로 계산된다.

```text
Scale * Rotation * Translation
```

주석상 row-vector convention을 사용한다고 명시되어 있다.

## Component 구조

기본 `Component`는 owner GameObject 포인터와 virtual `Update()`를 가진다.

현재 실제 코드에서 사용되는 주요 컴포넌트:

- `MeshRendererComponent`
- `LightComponent`
- `Physics::ColliderComponent`
- `PostProcessComponent`
- `PlayerStartComponent`

## 씬 업데이트 흐름

```text
Application::Update
  -> EditorLayer::Update
  -> Camera update
  -> Scene::Update
      -> GameObject::Update
          -> Component::Update
  -> PhysicsWorld BuildFromScene / UpdateOverlaps
```

현재 기본 Component의 `Update()`는 비어 있다. 따라서 실제 per-component gameplay update는 아직 구현되어 있지 않다.

## 씬 렌더링 흐름

```text
Scene::Render(Renderer)
  -> renderer.SetActiveCamera(activeCamera)
  -> renderer.SetDirectionalLight(directionalLight)
  -> GameObject의 LightComponent 수집
  -> renderer.SetLights(lights, ambientColor)
  -> 모든 GameObject에 대해 renderer.Render(object)
  -> renderer.FlushRenderQueue()
```

`renderer.Render(object)`는 MeshRendererComponent가 있는 오브젝트만 렌더 큐에 제출한다.

## 씬 저장 / 로드

씬 저장/로드는 `SceneSerializer`가 담당한다. 포맷은 커스텀 텍스트 섹션 방식이다.

확인된 섹션:

- `[Camera]`
- `[DirectionalLight]`
- `[GameObject]`

GameObject 섹션에는 Transform, Mesh/Material, Component 데이터가 기록된다.

저장 가능한 컴포넌트:

- MeshRendererComponent
- LightComponent
- ColliderComponent
- PostProcessComponent
- PlayerStartComponent

## Play Mode와 Scene

`EditorLayer`의 `m_playMode`가 켜지면 `Application::EnterPlayMode()`가 호출된다.

동작:

- 현재 카메라 상태 저장
- 첫 번째 `PlayerStartComponent`를 가진 GameObject 검색
- 카메라 position/yaw/pitch와 이동 설정 적용
- mouse capture 활성화

Play Mode 종료 시 저장해 둔 에디터 카메라 상태를 복원한다.

주의: 현재 Play Mode가 별도 scene copy를 만들어 실행하는 구조는 아니다. 동일한 `m_scene`을 사용한다.

## 관련 파일

- `DirectEngine_JAEMIN/Engine/Scene/Scene.h`
- `DirectEngine_JAEMIN/Engine/Scene/Scene.cpp`
- `DirectEngine_JAEMIN/Engine/Scene/GameObject.h`
- `DirectEngine_JAEMIN/Engine/Scene/GameObject.cpp`
- `DirectEngine_JAEMIN/Engine/Scene/Component.h`
- `DirectEngine_JAEMIN/Engine/Scene/Serialization/SceneSerializer.cpp`
- `DirectEngine_JAEMIN/Engine/Core/Application.cpp`

## 유지보수 시 주의점

- `Scene::DestroyGameObject()`는 raw pointer를 찾아 vector에서 제거한다. 외부 선택 포인터는 반드시 검증해야 한다.
- `GameObject`는 복사 금지이며 snapshot/restore 방식으로 복제와 Undo를 처리한다.
- 새 컴포넌트는 `SceneSerializer`, `GameObjectSnapshot`, `EditorLayer` Details UI에 반영해야 저장/복원/편집이 가능하다.
- Play Mode가 scene copy를 사용하지 않으므로, 런타임 중 씬을 변경하는 기능을 추가할 때 에디터 상태 오염을 조심해야 한다.

## 앞으로 개선할 점

- Runtime scene instance와 Editor scene instance 분리 검토
- Component lifecycle 확장: OnCreate, OnDestroy, OnEnable 등
- Scene dirty 변경 지점 일관화
- Scene ID 또는 GameObject ID 도입 검토
- Camera를 GameObject Component로 통합할지 방향 결정
