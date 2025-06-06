// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// NYI:
//   implement image cache
//   implement prefetch functionality
//   implement multi source (parse out most suitable image source from array of
//   sources)
#include "pch.h"

#include "ImageViewManagerModule.h"

#include <UI.Xaml.Media.Imaging.h>
#include <Views/Image/ReactImage.h>
#include <cxxreact/JsArgumentHelpers.h>
#ifdef USE_FABRIC
#include <Fabric/WindowsImageManager.h>
#include <Utils/Helpers.h>
#include <wincodec.h>
#include "XamlUtils.h"
#endif // USE_FABRIC
#include <winrt/Windows.Storage.Streams.h>
#include "Unicode.h"

namespace winrt {
using namespace Windows::Foundation;
using namespace Windows::Storage::Streams;
using namespace xaml::Media::Imaging;
} // namespace winrt

namespace Microsoft::ReactNative {

winrt::fire_and_forget GetImageSizeAsync(
    const winrt::Microsoft::ReactNative::IReactPropertyBag &properties,
    std::string uriString,
    winrt::Microsoft::ReactNative::JSValue &&headers,
    Mso::Functor<void(int32_t width, int32_t height)> successCallback,
    Mso::Functor<void()> errorCallback
#ifdef USE_FABRIC
    ,
    bool useFabric
#endif // USE_FABRIC
) {
  bool succeeded{false};

  try {
    ReactImageSource source;
    source.uri = uriString;
    if (!headers.IsNull()) {
      for (auto &header : headers.AsObject()) {
        source.headers.push_back(std::make_pair(header.first, header.second.AsString()));
      }
    }

    winrt::Uri uri{Microsoft::Common::Unicode::Utf8ToUtf16(uriString)};
    winrt::hstring scheme{uri.SchemeName()};
    bool needsDownload = (scheme == L"http") || (scheme == L"https");
    bool inlineData = scheme == L"data";

    winrt::IRandomAccessStream memoryStream;
    if (needsDownload) {
      memoryStream = co_await GetImageStreamAsync(properties, source);
    } else if (inlineData) {
      memoryStream = co_await GetImageInlineDataAsync(source);
    }

#ifdef USE_FABRIC
    if (!useFabric) {
#endif // USE_FABRIC
      winrt::BitmapImage bitmap;
      if (memoryStream) {
        co_await bitmap.SetSourceAsync(memoryStream);
      }
      if (bitmap) {
        successCallback(bitmap.PixelWidth(), bitmap.PixelHeight());
        succeeded = true;
      }
#ifdef USE_FABRIC
    } else {
      auto result = wicBitmapSourceFromStream(memoryStream);
      if (!std::get<std::shared_ptr<facebook::react::ImageErrorInfo>>(result)) {
        auto imagingFactory = std::get<winrt::com_ptr<IWICImagingFactory>>(result);
        auto wicBmpSource = std::get<winrt::com_ptr<IWICBitmapSource>>(result);
        UINT width, height;
        if (SUCCEEDED(wicBmpSource->GetSize(&width, &height))) {
          successCallback(width, height);
          succeeded = true;
        }
      }
    }
#endif // USE_FABRIC
  } catch (winrt::hresult_error const &) {
  }

  if (!succeeded)
    errorCallback();

  co_return;
}

void ImageLoader::Initialize(React::ReactContext const &reactContext) noexcept {
  m_context = reactContext;
}

void ImageLoader::getSize(std::string uri, React::ReactPromise<std::vector<double>> &&result) noexcept {
  m_context.UIDispatcher().Post(
      [context = m_context, uri = std::move(uri), result = std::move(result)]() mutable noexcept {
        GetImageSizeAsync(
            context.Properties().Handle(),
            std::move(uri),
            {},
            [result](double width, double height) noexcept {
              result.Resolve(std::vector<double>{width, height});
            },
            [result]() noexcept { result.Reject("Failed"); }
#ifdef USE_FABRIC
            ,
            IsFabricEnabled(context.Properties().Handle())
#endif // USE_FABRIC
        );
      });
}

void ImageLoader::getSizeWithHeaders(
    std::string uri,
    React::JSValue &&headers,
    React::ReactPromise<Microsoft::ReactNativeSpecs::ImageLoaderIOSSpec_getSizeWithHeaders_returnType>
        &&result) noexcept {
  m_context.UIDispatcher().Post([context = m_context,
                                 uri = std::move(uri),
                                 headers = std::move(headers),
                                 result = std::move(result)]() mutable noexcept {
    GetImageSizeAsync(
        context.Properties().Handle(),
        std::move(uri),
        std::move(headers),
        [result](double width, double height) noexcept {
          result.Resolve(Microsoft::ReactNativeSpecs::ImageLoaderIOSSpec_getSizeWithHeaders_returnType{width, height});
        },
        [result]() noexcept { result.Reject("Failed"); }
#ifdef USE_FABRIC
        ,
        IsFabricEnabled(context.Properties().Handle())
#endif // USE_FABRIC
    );
  });
}

void ImageLoader::prefetchImage(std::string uri, React::ReactPromise<bool> &&result) noexcept {
  // NYI
  result.Resolve(true);
}

void ImageLoader::prefetchImageWithMetadata(
    std::string uri,
    std::string queryRootName,
    double rootTag,
    React::ReactPromise<bool> &&result) noexcept {
  // NYI
  result.Resolve(true);
}

void ImageLoader::queryCache(
    std::vector<std::string> const &uris,
    React::ReactPromise<React::JSValue> &&result) noexcept {
  // NYI
  result.Resolve(React::JSValueObject{});
}

} // namespace Microsoft::ReactNative
