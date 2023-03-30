// jeremie

#include "ImGuiDirectX11.h"

#include "imgui_impl_dx11.h"
#include "../OwlLog/OwlLog.h"
#include <opencv2/opencv.hpp>
#include <chrono>

namespace OwlImGuiDirectX11 {

    // https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples

    // img must be cv::ColorConversionCodes::COLOR_BGR2BGRA
    boost::shared_ptr<ImGuiD3D11Img> loadTextureFromMat(
            cv::Mat img,
            ID3D11Device *g_pd3dDevice) {
        // Load from disk into a raw RGBA buffer
        if (img.empty())
            return {};

        auto t1 = std::chrono::high_resolution_clock::now();

        if (img.channels() == 3) {
            cv::cvtColor(img, img, cv::ColorConversionCodes::COLOR_BGR2RGBA);
        }
        if (img.channels() == 1) {
            cv::cvtColor(img, img, cv::ColorConversionCodes::COLOR_GRAY2RGBA);
        }
        auto t2 = std::chrono::high_resolution_clock::now();

        // Create texture
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = img.cols;
        desc.Height = img.rows;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;

        auto t3 = std::chrono::high_resolution_clock::now();
        ID3D11Texture2D *pTexture = nullptr;
        D3D11_SUBRESOURCE_DATA subResource;
        subResource.pSysMem = img.data;
        subResource.SysMemPitch = desc.Width * 4;
        subResource.SysMemSlicePitch = 0;
        g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

        auto t4 = std::chrono::high_resolution_clock::now();
        // Create texture view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory(&srvDesc, sizeof(srvDesc));
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
        ID3D11ShaderResourceView *texture = nullptr;
        g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, &texture);
        pTexture->Release();

        auto t5 = std::chrono::high_resolution_clock::now();
        if (nullptr == texture) {
            return {};
        }

        auto t6 = std::chrono::high_resolution_clock::now();
        boost::shared_ptr<ID3D11ShaderResourceView> texturePtr{
                texture, [](ID3D11ShaderResourceView *ptr) {
                    ptr->Release();
                }
        };

//        std::unique_ptr<ID3D11ShaderResourceView, void (*)(ID3D11ShaderResourceView *)> texturePtr{
//                texture, [](ID3D11ShaderResourceView *ptr) {
//                    ptr->Release();
//                }
//        };

        auto t7 = std::chrono::high_resolution_clock::now();
        auto data = boost::make_shared<ImGuiD3D11Img>(
                img.cols, img.rows, std::move(texturePtr)
        );

        auto t8 = std::chrono::high_resolution_clock::now();
        BOOST_LOG_OWL(trace)
            << "loadTextureFromMat "
//            << " t1 " << std::chrono::time_point_cast<std::chrono::milliseconds>(t1).time_since_epoch().count()
            << " t2-t1 " << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count()
//            << " t2 " << std::chrono::time_point_cast<std::chrono::milliseconds>(t2).time_since_epoch().count()
            << " t3-t2 " << std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count()
//            << " t3 " << std::chrono::time_point_cast<std::chrono::milliseconds>(t3).time_since_epoch().count()
            << " t4-t3 " << std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count()
//            << " t4 " << std::chrono::time_point_cast<std::chrono::milliseconds>(t4).time_since_epoch().count()
            << " t5-t4 " << std::chrono::duration_cast<std::chrono::microseconds>(t5 - t4).count()
//            << " t5 " << std::chrono::time_point_cast<std::chrono::milliseconds>(t5).time_since_epoch().count()
            << " t6-t5 " << std::chrono::duration_cast<std::chrono::microseconds>(t6 - t5).count()
//            << " t6 " << std::chrono::time_point_cast<std::chrono::milliseconds>(t6).time_since_epoch().count()
            << " t7-t6 " << std::chrono::duration_cast<std::chrono::microseconds>(t7 - t6).count()
//            << " t7 " << std::chrono::time_point_cast<std::chrono::milliseconds>(t7).time_since_epoch().count()
;
        return data;
    }


} // OwlImGuiDirectX11
