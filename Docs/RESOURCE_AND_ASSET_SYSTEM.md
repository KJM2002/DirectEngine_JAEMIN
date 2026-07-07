# Resource And Asset System

마지막 갱신: 2026-07-07

## 개요

리소스 로딩과 캐싱은 `ResourceManager`가 담당한다. 에셋 스캔과 GUID 관리는 `AssetDatabase`가 담당한다.

```text
Renderer
  -> ResourceManager

EditorLayer
  -> AssetDatabase
  -> ResourceManager::SetAssetDatabase
```

## AssetDatabase

`Assets` 폴더를 스캔하고 `.meta` 파일을 만든다.

지원 AssetType:

- Mesh
- Texture
- Material
- Scene
- Prefab
- Unknown

주요 기능:

- ScanAssets
- ImportAsset
- CreateMetaIfMissing
- GetGuidFromPath
- GetPathFromGuid
- RenameAsset
- MoveAsset
- DeleteAsset

Content Browser의 Delete Asset은 실제 파일과 `.meta`를 함께 삭제한 뒤 asset list를 갱신한다.

## ResourceManager 캐시

주요 캐시:

- `m_meshes`
- `m_meshFiles`
- `m_vertexShaders`
- `m_pixelShaders`
- `m_textures`
- `m_fileTextures`
- `m_materials`
- `m_materialFiles`

개별 material은 `ForgetMaterial(path)`로 캐시에서 제거할 수 있다. Material Editor의 Reload에서 사용한다.

## Built-in Mesh

코드 생성 primitive:

- `builtin:mesh:cube`
- `builtin:mesh:sphere`
- `builtin:mesh:cylinder`
- `builtin:mesh:cone`
- `builtin:mesh:plane`

`LoadMesh()`는 위 built-in path를 파일 로더로 보내지 않고 직접 생성 함수로 매핑한다.

## Mesh 로딩

실제 파일 로딩은 현재 OBJ 중심이다.

OBJLoader 처리:

- position
- texcoord
- normal
- material library 일부
- object/group/usemtl
- face triangulation
- normal/tangent 보정

`.gltf`, `.glb`, `.fbx`는 asset type으로 분류될 수 있지만 실제 로딩 경로는 아직 OBJ 수준으로 봐야 한다.

## Texture 로딩

WIC 기반 이미지 로딩 후 D3D11 texture/SRV/sampler를 만든다.

기본 옵션:

```cpp
struct TextureLoadOptions
{
    bool sRGB = true;
    bool generateMips = true;
    RHI::TextureFilter filter = RHI::TextureFilter::Linear;
    RHI::TextureAddressMode addressMode = RHI::TextureAddressMode::Wrap;
};
```

Material의 scalar map은 linear 옵션을 사용한다.

## Material 로딩

`.mat` 파일은 `MaterialLoader`가 읽고 쓴다.

현재 주요 필드:

- name
- vertex shader path
- pixel shader path
- base color
- roughness
- metallic
- normal strength
- base color texture path
- roughness texture path
- metallic texture path
- normal texture path

`ResourceManager::LoadMaterial()`은 `.mat` 파일을 읽고 필요한 shader/texture를 함께 로드한다.

## Material Asset 과 Instance

에셋 브라우저나 Inspector에서 `.mat`을 오브젝트에 적용하면 material asset을 그대로 공유하지 않고 clone된 material instance를 넣는다. 대신 `MeshRendererComponent`에 material path/GUID를 유지한다.

이 구조의 목적:

- 한 오브젝트 Inspector 편집이 주변 오브젝트에 전파되는 문제 방지
- Material Editor에서 asset 값을 바꿀 때는 같은 path/GUID를 참조하는 모든 instance에 live sync 가능

Inspector에서 material 값을 직접 편집하면 editable override로 간주하고 material path/GUID를 지운다. 이 오브젝트는 이후 asset editor live sync 대상이 아니다.

## GUID와 경로

씬에는 mesh/material path와 GUID가 함께 저장될 수 있다. 현재 로드는 경로 기반이 중심이고, GUID 기반 API는 보조 경로다.

에셋 rename/move/delete 작업은 path/GUID 참조를 같이 고려해야 한다.

## 관련 파일

- `DirectEngine_JAEMIN/Engine/Resource/ResourceManager.*`
- `DirectEngine_JAEMIN/Engine/Resource/MaterialLoader.*`
- `DirectEngine_JAEMIN/Engine/Asset/AssetDatabase.*`
- `DirectEngine_JAEMIN/Engine/Graphics/Renderer/Material.*`
- `DirectEngine_JAEMIN/Engine/Graphics/Renderer/Mesh.*`
