# Resource and Asset System

이 문서는 현재 코드에서 확인된 리소스 로딩, 에셋 스캔, `.meta` 관리 구조를 설명한다.

## 현재 구현 상태

리소스 시스템은 `Engine::Resource::ResourceManager`가 담당한다. 렌더러 초기화 과정에서 `Renderer`가 `ResourceManager`를 생성하고, 에디터의 `AssetDatabase` 포인터를 연결해 GUID 기반 조회도 일부 사용할 수 있게 만든다.

에셋 시스템은 `Engine::Asset::AssetDatabase`가 담당한다. `Assets` 폴더를 스캔하고, 지원하는 확장자에 대해 `.meta` 파일을 생성하거나 읽어 GUID와 에셋 타입을 관리한다.

다만 현재 씬 저장 데이터는 GUID와 경로를 모두 기록하지만, 씬 로드의 실제 리소스 로딩은 주로 경로 기반으로 동작한다. GUID 기반 로딩 함수는 존재하지만 모든 워크플로우가 GUID 기반으로 통일된 상태는 아니다.

## 주요 클래스

```cpp
class ResourceManager
{
public:
    std::shared_ptr<Renderer::Mesh> LoadMesh(const std::wstring& path);
    std::shared_ptr<Renderer::Mesh> LoadMeshByGuid(const std::string& guid);
    std::shared_ptr<RHI::RHITexture> LoadTexture(const std::wstring& path);
    std::shared_ptr<RHI::RHITexture> LoadTextureByGuid(const std::string& guid);
    std::shared_ptr<Renderer::Material> LoadMaterial(const std::wstring& path);
    std::shared_ptr<Renderer::Material> LoadMaterialByGuid(const std::string& guid);
};
```

```cpp
class AssetDatabase
{
public:
    void ScanAssets(const std::filesystem::path& root = "Assets");
    bool ImportAsset(const std::filesystem::path& path);
    bool CreateMetaIfMissing(const std::filesystem::path& path);
    std::string GetGuidFromPath(const std::filesystem::path& path) const;
    std::filesystem::path GetPathFromGuid(const std::string& guid) const;
};
```

## AssetDatabase 역할

`AssetDatabase::ScanAssets()`는 기본적으로 `Assets` 폴더를 순회한다. 일반 파일만 대상으로 하며, `.meta`, `.tmp`, `.bak`, `.gitignore`는 제외한다.

지원 타입은 `AssetType` 기준으로 다음과 같다.

- `Mesh`
- `Texture`
- `Material`
- `Scene`
- `Prefab`
- `Unknown`

확장자 기반 타입 판정은 `AssetTypeFromPath()`에서 수행된다. 코드상 `.obj`, `.gltf`, `.glb`, `.fbx` 등이 Mesh 타입으로 분류될 수 있지만, 실제 메시 로딩 경로는 확인된 범위에서 OBJ 로더 중심이다. 따라서 FBX/glTF는 "에셋 타입으로 인식 가능"과 "실제 렌더링 로드 가능"을 분리해서 봐야 한다.

`.meta` 파일은 `AssetMeta` 구조를 저장한다.

```cpp
struct AssetMeta
{
    std::string guid;
    AssetType type = AssetType::Unknown;
    std::filesystem::path sourcePath;
    int importerVersion = 1;
};
```

에셋 이동, 이름 변경, 삭제 함수도 존재한다.

- `MoveAsset()`
- `RenameAsset()`
- `DeleteAsset()`

이 함수들은 파일과 `.meta`를 함께 이동/삭제한 뒤 `ScanAssets("Assets")`를 다시 호출한다.

## ResourceManager 역할

`ResourceManager`는 메시, 셰이더, 텍스처, 머티리얼을 캐시한다.

확인된 캐시 맵은 다음과 같다.

- `m_meshes`
- `m_meshFiles`
- `m_vertexShaders`
- `m_pixelShaders`
- `m_textures`
- `m_fileTextures`
- `m_materials`
- `m_materialFiles`

캐시는 `std::shared_ptr` 기반으로 보관된다. `Clear()` 함수가 존재하지만, 개별 리소스의 참조 추적이나 사용 중 리소스 해제 정책은 별도 시스템으로 분리되어 있지 않다.

## Mesh 로딩

확인된 실제 메시 로딩은 `ResourceManager::LoadMesh()`에서 경로를 받아 OBJ 로딩 경로로 이어진다. 실패하면 내장 큐브 메시를 fallback으로 사용할 수 있다.

`OBJLoader`는 다음 데이터를 처리한다.

- position
- texcoord
- normal
- material library
- object/group/usemtl
- face triangulation
- normal/tangent 생성 보정

주의할 점은 AssetDatabase가 `.fbx`, `.gltf`, `.glb`를 Mesh로 분류할 수 있어도, 현재 ResourceManager에서 해당 포맷을 실제로 파싱하는 구현은 확인되지 않았다는 점이다.

## Texture 로딩

`ResourceManager::LoadTexture()`는 파일 경로 기반 캐시를 먼저 확인한다. 실패 시 `CreateCheckerTexture()`로 생성한 체크 텍스처를 fallback으로 캐시에 넣는다.

텍스처 옵션 구조는 다음과 같다.

```cpp
struct TextureLoadOptions
{
    bool sRGB = true;
    bool generateMips = true;
    RHI::TextureFilter filter = RHI::TextureFilter::Linear;
    RHI::TextureAddressMode addressMode = RHI::TextureAddressMode::Wrap;
};
```

현재 문서 기준으로 확인된 것은 로딩 옵션 구조와 fallback 처리다. 각 이미지 포맷별 세부 동작은 `ResourceManager.cpp`와 이미지 로더 구현을 함께 확인해야 한다.

## Material 로딩

머티리얼은 `MaterialLoader`가 커스텀 `.mat` 파일을 읽고 쓴다.

```cpp
struct MaterialDesc
{
    std::string name;
    std::wstring vertexShaderPath;
    std::wstring pixelShaderPath;
    Math::Vector4 baseColor;
    float roughness;
    float metallic;
    std::wstring texturePath;
    std::wstring roughnessTexturePath;
    std::wstring metallicTexturePath;
    std::wstring normalTexturePath;
};
```

`ResourceManager::LoadMaterial()`은 `.mat` 파일을 읽어 머티리얼을 만들고, 지정된 셰이더와 텍스처 경로가 있으면 함께 로드한다.

확인된 텍스처 슬롯은 다음과 같다.

- Base Color Texture
- Roughness Texture
- Metallic Texture
- Normal Texture

## 경로 참조와 GUID 참조

현재 프로젝트에는 두 참조 방식이 함께 존재한다.

- 경로 기반: `LoadMesh(path)`, `LoadTexture(path)`, `LoadMaterial(path)`
- GUID 기반: `LoadMeshByGuid(guid)`, `LoadTextureByGuid(guid)`, `LoadMaterialByGuid(guid)`

GUID 기반 함수는 `AssetDatabase`가 연결되어 있어야 동작한다. 연결되지 않았거나 GUID를 경로로 해석할 수 없으면 경고 로그 후 `nullptr`을 반환한다.

씬 저장 시 `MeshRendererComponent`의 `mesh`, `mesh_guid`, `material`, `material_guid`를 기록하는 코드가 확인된다. 그러나 현재 씬 로드 흐름은 경로 기반 로딩이 중심이므로, 에셋 이동 후 GUID만으로 씬 참조가 완전히 복구된다고 단정하면 안 된다.

## 에셋 추가 흐름

현재 코드 기준 에디터에서 확인된 흐름은 다음과 같다.

1. Content Browser 또는 Import Model 기능에서 파일을 `Assets` 아래로 가져온다.
2. `AssetDatabase::ImportAsset()` 또는 `ScanAssets()`가 호출된다.
3. `.meta`가 없으면 `CreateMetaIfMissing()`이 생성한다.
4. 에셋 타입과 GUID가 `AssetDatabase` 맵에 등록된다.
5. 선택한 에셋 타입에 따라 씬 오브젝트에 메시, 머티리얼, 텍스처를 적용한다.

## 현재 한계

- AssetDatabase의 타입 인식 범위와 ResourceManager의 실제 로딩 지원 범위가 일치하지 않는다.
- 씬 로드는 GUID보다 경로에 더 의존한다.
- 리소스 캐시는 단순 맵 기반이며 참조 카운트 정책, 핫 리로드, 메모리 예산 관리가 없다.
- `LoadMesh()` 실패 시 fallback cube가 들어갈 수 있어, 잘못된 에셋 경로가 화면상으로는 큐브처럼 보일 수 있다.
- `ForgetMaterial()`은 확인되지만 전체 에셋 변경에 대한 일관된 캐시 무효화 정책은 아직 제한적이다.

## 관련 파일

- `DirectEngine_JAEMIN/Engine/Resource/ResourceManager.h`
- `DirectEngine_JAEMIN/Engine/Resource/ResourceManager.cpp`
- `DirectEngine_JAEMIN/Engine/Resource/MaterialLoader.h`
- `DirectEngine_JAEMIN/Engine/Resource/MaterialLoader.cpp`
- `DirectEngine_JAEMIN/Engine/Asset/AssetDatabase.h`
- `DirectEngine_JAEMIN/Engine/Asset/AssetDatabase.cpp`
- `DirectEngine_JAEMIN/Engine/Asset/AssetMeta.h`
- `DirectEngine_JAEMIN/Engine/Asset/AssetType.h`
- `DirectEngine_JAEMIN/Engine/AssetImport/OBJLoader.h`
- `DirectEngine_JAEMIN/Engine/AssetImport/OBJLoader.cpp`
- `DirectEngine_JAEMIN/Engine/Graphics/Renderer/Material.h`
- `DirectEngine_JAEMIN/Engine/Graphics/Renderer/Mesh.h`

## 유지보수 시 주의점

- 새 포맷을 AssetType에 추가했다면 실제 로더도 함께 구현해야 한다.
- `.meta` GUID를 신뢰하는 기능을 만들 때는 경로 변경, 이름 변경, 삭제 시나리오를 반드시 같이 확인해야 한다.
- ResourceManager 캐시 키는 문자열과 경로가 섞여 있으므로 같은 리소스가 중복 로드되지 않는지 확인해야 한다.
- 실패 fallback을 사용할 때는 사용자에게 경고가 보이도록 로그나 에디터 표시를 강화하는 편이 안전하다.
- 머티리얼 텍스처 슬롯을 추가하면 C++ 머티리얼, `.mat` 포맷, 씬 저장/로드, 셰이더 바인딩을 함께 수정해야 한다.

## 앞으로 개선할 점

- 경로 기반 참조와 GUID 기반 참조의 우선순위를 명확히 정리한다.
- OBJ 외 Mesh 포맷 지원 여부를 코드와 UI에서 일치시킨다.
- 리소스 캐시 무효화와 핫 리로드 정책을 만든다.
- 에셋 누락 시 fallback 사용 여부를 에디터에서 명확히 표시한다.
- `.meta` 파일 포맷에 버전/마이그레이션 정책을 추가한다.
