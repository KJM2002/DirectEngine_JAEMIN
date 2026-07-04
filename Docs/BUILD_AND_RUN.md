# BUILD AND RUN

## 사용 IDE

현재 솔루션 파일은 Visual Studio 2022 형식이다.

- Solution format: Visual Studio Version 17
- Platform Toolset: `v143`
- Language Standard: `stdcpp17`
- Windows Target Platform Version: `10.0`

## 솔루션 / 프로젝트 파일

- 솔루션: `DirectEngine_JAEMIN.sln`
- 프로젝트: `DirectEngine_JAEMIN/DirectEngine_JAEMIN.vcxproj`
- 실행 진입점: `DirectEngine_JAEMIN/main.cpp`

솔루션에는 `DirectEngine_JAEMIN` 단일 C++ 프로젝트가 등록되어 있다.

## 빌드 구성

확인된 구성:

- `Debug|Win32`
- `Release|Win32`
- `Debug|x64`
- `Release|x64`

프로젝트 설정은 Console Application이며, Debug 구성에서는 `_DEBUG`, Release 구성에서는 `NDEBUG`가 정의된다.

## 필요 라이브러리

프로젝트 파일의 linker 설정에서 다음 라이브러리를 확인했다.

- `d3d11.lib`
- `d3d12.lib`
- `dxgi.lib`
- `d3dcompiler.lib`
- `winmm.lib`

또한 `WICImageLoader.cpp`에는 `windowscodecs.lib` pragma comment가 있다.

## Include 경로

프로젝트의 추가 include 경로:

- `$(ProjectDir)`
- `$(ProjectDir)ThirdParty\ImGui`
- `$(ProjectDir)ThirdParty\ImGui\backends`

## 실행 방법

Visual Studio에서 `DirectEngine_JAEMIN.sln`을 열고 `Debug|x64` 또는 원하는 구성을 선택한 뒤 실행한다.

현재 기존 산출물은 다음 위치에서 확인되었다.

- `x64/Debug/DirectEngine_JAEMIN.exe`
- `DirectEngine_JAEMIN/x64/Debug/*.obj`

실행 시 `Application::Initialize()`는 다음 순서로 초기화한다.

```text
Win32Window
Renderer(D3D11)
AudioSystem
Scene
DebugUI
EditorLayer
ImGuiDebugUI
```

## 필요한 에셋 / 셰이더

렌더러와 리소스 시스템은 상대 경로로 셰이더와 에셋을 찾는다. 실행 working directory가 프로젝트 루트 또는 `DirectEngine_JAEMIN` 폴더 기준으로 맞지 않으면 파일 로드 실패가 발생할 수 있다.

주요 셰이더:

- `Shaders/BasicVertex.hlsl`
- `Shaders/BasicPixel.hlsl`
- `Shaders/DebugLineVertex.hlsl`
- `Shaders/DebugLinePixel.hlsl`
- `Shaders/PostProcessVertex.hlsl`
- `Shaders/PostProcessPixel.hlsl`
- `Shaders/ShadowDepthVertex.hlsl`
- `Shaders/ShadowDepthPixel.hlsl`

주요 에셋 폴더:

- `Assets/Models`
- `Assets/Textures`
- `Assets/Materials`
- `Assets/Scenes`

## 현재 확인된 경로 불일치

코드와 프로젝트 파일에는 다음 경로가 등장한다.

- `Assets/Scenes/Test.scene`
- `Assets/Materials/Default.mat`
- `Assets/Materials/TestTextured.mat`

하지만 현재 실제 파일 목록에서는 이 파일들이 확인되지 않았다. 실제 존재하는 씬 파일은 예를 들어 다음과 같다.

- `Assets/Scenes/NewScene.scene`
- `Assets/Scenes/Scene_One.scene`
- `Assets/Scenes/Scne_Two.scene`

`Application::InitializeScene()`은 먼저 `Assets/Scenes/Test.scene`을 로드하고, 실패하면 기본 카메라/라이트/큐브/PlayerStart를 생성한다.

## 빌드 실패 시 확인할 점

- Visual Studio 2022와 v143 toolset 설치 여부
- Windows SDK 설치 여부
- DirectX 관련 SDK 라이브러리 링크 가능 여부
- `ThirdParty/ImGui` 소스와 backend 파일 포함 여부
- HLSL 파일의 상대 경로가 실행 working directory에서 맞는지 확인
- `.vcxproj`에 등록된 에셋 경로와 실제 파일이 일치하는지 확인
- D3D11 debug layer가 없는 환경에서는 코드가 debug flag 없이 재시도하도록 되어 있으나, 그래도 디바이스 생성 실패 로그를 확인해야 한다.

## 관련 파일

- `DirectEngine_JAEMIN.sln`
- `DirectEngine_JAEMIN/DirectEngine_JAEMIN.vcxproj`
- `DirectEngine_JAEMIN/main.cpp`
- `DirectEngine_JAEMIN/Engine/Core/Application.cpp`
- `DirectEngine_JAEMIN/Engine/Graphics/D3D11/D3D11Device.cpp`
- `DirectEngine_JAEMIN/Engine/Platform/Windows/WICImageLoader.cpp`

## 유지보수 시 주의점

- 새 소스 파일을 추가하면 `.vcxproj`와 `.vcxproj.filters` 등록이 필요하다.
- 새 셰이더 파일을 추가할 때 실행 경로 기준 상대 경로를 확인해야 한다.
- 에셋 파일을 프로젝트에 등록할 경우 실제 `Assets` 폴더와 일치하는지 확인한다.
- D3D12 파일은 컴파일되지만 현재 런타임 경로가 아니므로, D3D12 빌드 성공을 기능 완성으로 판단하면 안 된다.

## 앞으로 개선할 점

- 초기 씬 경로를 실제 존재하는 파일로 맞추거나 설정 파일화
- 빌드 후 실행 working directory 명시
- 누락된 프로젝트 파일 항목 정리
- 자동 빌드 검증 스크립트 추가
- Debug/Release별 산출물 정리 정책 문서화
