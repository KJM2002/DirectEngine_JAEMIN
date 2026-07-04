# OBJECT COMPONENT SYSTEM

## GameObject 역할

`GameObject`는 씬 안의 기본 오브젝트 단위다. 현재 구조에서 GameObject는 다음을 가진다.

- 이름
- Transform
- Component 목록

```cpp
class GameObject
{
public:
    const std::string& GetName() const;
    void SetName(std::string name);
    Math::Transform& GetTransform();
    template <typename T, typename... Args> T& AddComponent(Args&&... args);
    template <typename T> T* GetComponent() const;
};
```

GameObject는 복사할 수 없으며 `Scene`이 `unique_ptr`로 소유한다.

## Transform 역할

Transform은 위치, 회전, 스케일을 담고 월드 행렬을 만든다.

```text
position: DirectX::XMFLOAT3
rotation: DirectX::XMFLOAT3, radian 기준
scale:    DirectX::XMFLOAT3
```

에디터 Details 패널에서는 rotation을 degree로 표시하고 내부 저장은 radian으로 유지한다.

## Component 역할

`Component`는 GameObject에 붙는 기능 단위다. 기본 클래스는 owner 포인터와 virtual update만 가진다.

```cpp
class Component
{
public:
    void SetOwner(GameObject* owner);
    GameObject* GetOwner() const;
    virtual void Update(float deltaTime);
};
```

현재 기본 `Component::Update()`는 비어 있다.

## Component 추가 / 제거 흐름

### 추가

`GameObject::AddComponent<T>()`는 `std::make_shared<T>()`로 컴포넌트를 만들고 owner를 설정한 뒤 vector에 추가한다.

```text
GameObject::AddComponent<T>
  -> make_shared<T>
  -> component->SetOwner(this)
  -> m_components.push_back(component)
```

### 조회

`GetComponent<T>()`와 `GetComponents<T>()`는 `dynamic_cast`로 타입을 확인한다.

### 제거

`RemoveComponents<T>()`는 해당 타입 컴포넌트를 모두 제거한다. 현재 `ApplyModelCollidersToSelectedObjects()`에서 기존 ColliderComponent 제거 후 새 Collider를 적용할 때 사용된다.

## 실제 확인된 Component

### MeshRendererComponent

역할:

- `Renderer::Mesh` shared_ptr 보관
- `Renderer::Material` shared_ptr 보관
- mesh/material path 보관
- mesh/material GUID 보관

실제 렌더링은 `Renderer::Render(GameObject)`가 이 컴포넌트를 조회해 수행한다.

### LightComponent

역할:

- Directional/Point/Spot 타입
- color, intensity, range
- direction
- inner/outer cone angle
- ambient color

`Scene::Render()`는 모든 GameObject의 LightComponent를 수집해 `Renderer::SetLights()`에 전달한다.

### ColliderComponent

역할:

- AABB 또는 Sphere collider
- local center, size, radius
- trigger flag
- world center/size/axis/radius 계산

사용 경로:

- `PhysicsWorld::BuildFromScene`
- `PhysicsWorld::Raycast`
- `Renderer::RenderColliders`
- Editor selection fallback

### PostProcessComponent

역할:

- 후처리 활성화 여부
- grayscale, vignette, tone mapping, color adjust 설정

`EditorLayer::ApplyRendererSettings()`가 scene에서 첫 번째 활성 PostProcessComponent를 찾아 Renderer에 전달한다.

### PlayerStartComponent

역할:

- Play Mode 시작 위치에 붙이는 marker
- player height, move speed, fast move multiplier, mouse sensitivity

`Application::EnterPlayMode()`가 첫 번째 PlayerStartComponent를 찾아 카메라 위치와 이동 설정을 적용한다.

## 미사용 또는 주의할 클래스

`Engine::Scene::MeshRenderer` 클래스가 별도로 존재하지만 현재 검색 기준 실제 호출 경로가 확인되지 않았다. 실제 렌더링에 사용되는 클래스는 `MeshRendererComponent`다.

## 신규 Component 추가 절차

새 컴포넌트를 추가할 때 최소 확인 대상:

1. `Component`를 상속한 새 클래스 작성
2. GameObject에 `AddComponent<NewComponent>()`로 붙일 수 있는지 확인
3. `EditorLayer` Details 또는 Add Component UI에 노출할지 결정
4. `SceneSerializer` 저장/로드 지원 추가 여부 결정
5. `GameObjectSnapshot`에 캡처/복원 지원 추가 여부 결정
6. Undo/Redo 명령에 포함할지 결정
7. Runtime update가 필요하면 `Update()` 구현
8. Renderer/Physics/ResourceManager와 연결되는 경우 수명과 참조 방식 확인

## 관련 파일

- `DirectEngine_JAEMIN/Engine/Scene/GameObject.h`
- `DirectEngine_JAEMIN/Engine/Scene/GameObject.cpp`
- `DirectEngine_JAEMIN/Engine/Scene/Component.h`
- `DirectEngine_JAEMIN/Engine/Scene/Component.cpp`
- `DirectEngine_JAEMIN/Engine/Math/Transform.h`
- `DirectEngine_JAEMIN/Engine/Scene/MeshRendererComponent.h`
- `DirectEngine_JAEMIN/Engine/Scene/LightComponent.h`
- `DirectEngine_JAEMIN/Engine/Physics/ColliderComponent.h`
- `DirectEngine_JAEMIN/Engine/Scene/PostProcessComponent.h`
- `DirectEngine_JAEMIN/Engine/Scene/PlayerStartComponent.h`
- `DirectEngine_JAEMIN/Engine/Editor/Command/GameObjectSnapshot.cpp`

## 유지보수 시 주의점

- 컴포넌트 조회가 `dynamic_cast` 기반이므로 타입 이름 변경과 다중 상속 구조에 주의한다.
- Component owner는 raw pointer다. GameObject 제거 후 외부에 남은 Component/GameObject 포인터를 쓰면 위험하다.
- 저장/로드가 없는 컴포넌트는 씬 저장 후 사라진다.
- Snapshot 복원이 없는 컴포넌트는 Undo/Redo에서 보존되지 않는다.

## 앞으로 개선할 점

- Component registry 도입 검토
- Component별 serialization 함수 분리
- Editor property drawer 구조 도입
- 미사용 `MeshRenderer` 클래스 정리
- GameObject 고유 ID 도입
