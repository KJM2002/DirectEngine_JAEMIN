#include "WICImageLoader.h"

#include "Engine/Core/Log.h"

#include <Windows.h>
#include <wincodec.h>
#include <wrl/client.h>

#pragma comment(lib, "windowscodecs.lib")

namespace Engine::Platform::Windows
{
    namespace
    {
        using Microsoft::WRL::ComPtr;

        std::string ToUtf8(const std::wstring& value)
        {
            if (value.empty())
            {
                return {};
            }

            const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
            if (size <= 1)
            {
                return {};
            }

            std::string result(static_cast<std::size_t>(size - 1), '\0');
            WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, result.data(), size, nullptr, nullptr);
            return result;
        }

        void LogLoadFailure(const std::wstring& path, const char* reason)
        {
            std::string narrowPath = ToUtf8(path);
            Engine::Core::Log::Error("Failed to load texture '" + narrowPath + "': " + reason);
        }
    }

    bool WICImageLoader::LoadFromFile(const std::wstring& path, Resource::ImageData& outImage)
    {
        outImage = {};

        HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        const bool shouldUninitialize = SUCCEEDED(comResult);
        if (comResult == RPC_E_CHANGED_MODE)
        {
            comResult = S_OK;
        }

        if (FAILED(comResult))
        {
            LogLoadFailure(path, "COM initialization failed.");
            return false;
        }

        ComPtr<IWICImagingFactory> factory;
        HRESULT result = CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(factory.GetAddressOf()));

        ComPtr<IWICBitmapDecoder> decoder;
        if (SUCCEEDED(result))
        {
            result = factory->CreateDecoderFromFilename(
                path.c_str(),
                nullptr,
                GENERIC_READ,
                WICDecodeMetadataCacheOnLoad,
                decoder.GetAddressOf());
        }

        ComPtr<IWICBitmapFrameDecode> frame;
        if (SUCCEEDED(result))
        {
            result = decoder->GetFrame(0, frame.GetAddressOf());
        }

        ComPtr<IWICFormatConverter> converter;
        if (SUCCEEDED(result))
        {
            result = factory->CreateFormatConverter(converter.GetAddressOf());
        }

        if (SUCCEEDED(result))
        {
            result = converter->Initialize(
                frame.Get(),
                GUID_WICPixelFormat32bppRGBA,
                WICBitmapDitherTypeNone,
                nullptr,
                0.0,
                WICBitmapPaletteTypeCustom);
        }

        UINT width = 0;
        UINT height = 0;
        if (SUCCEEDED(result))
        {
            result = converter->GetSize(&width, &height);
        }

        if (SUCCEEDED(result) && (width == 0 || height == 0))
        {
            result = E_FAIL;
        }

        if (SUCCEEDED(result))
        {
            outImage.width = static_cast<std::uint32_t>(width);
            outImage.height = static_cast<std::uint32_t>(height);
            outImage.channelCount = 4;
            outImage.format = RHI::TextureFormat::RGBA8;
            outImage.pixels.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * outImage.channelCount);

            result = converter->CopyPixels(
                nullptr,
                outImage.GetRowPitch(),
                static_cast<UINT>(outImage.pixels.size()),
                outImage.pixels.data());
        }

        if (shouldUninitialize)
        {
            CoUninitialize();
        }

        if (FAILED(result) || !outImage.IsValid())
        {
            outImage = {};
            LogLoadFailure(path, "WIC decode failed.");
            return false;
        }

        return true;
    }
}
