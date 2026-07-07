# Build And Run

마지막 갱신: 2026-07-07

## 요구 환경

- Windows
- Visual Studio 2022
- MSVC C++17
- Windows SDK
- DirectX 11 런타임

## 솔루션

```text
DirectEngine_JAEMIN.sln
```

주 프로젝트:

```text
DirectEngine_JAEMIN/DirectEngine_JAEMIN.vcxproj
```

## Debug x64 빌드

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe' DirectEngine_JAEMIN.sln /p:Configuration=Debug /p:Platform=x64 /m
```

성공 시:

```text
x64/Debug/DirectEngine_JAEMIN.exe
```

## 실행

```powershell
.\x64\Debug\DirectEngine_JAEMIN.exe
```

## 시작 씬 규칙

1. `Assets/Scenes/Test.scene` 로드를 시도한다.
2. 성공하면 해당 씬을 사용한다.
3. 실패하면 코드 fallback 씬을 만든다.

fallback 씬:

- 정지된 `Cube` 1개
- 기본 방향광
- 카메라는 큐브 우측면을 사선으로 보는 위치

## 자주 확인할 폴더

- `Assets/Scenes`: 씬 파일
- `Assets/Materials`: material asset
- `Assets/Textures`: texture asset
- `Assets/Models`: model asset
- `Shaders`: HLSL
- `Docs`: 문서

## 빌드 검증 기준

최근 변경은 `Debug | x64` 기준으로 확인한다. D3D12는 현재 실제 실행 경로가 아니므로 D3D11 경로를 우선 검증한다.
