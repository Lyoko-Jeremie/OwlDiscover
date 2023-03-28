// jeremie

#include "ImGuiDirectX11.h"

#include "imgui_impl_dx11.h"
#include <opencv2/opencv.hpp>

namespace OwlImGuiDirectX11 {

    // https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples

    // img must be cv::ColorConversionCodes::COLOR_BGR2BGRA
    boost::shared_ptr<ImGuiD3D11Img> loadTextureFromMat(
            cv::Mat img,
            ID3D11Device *g_pd3dDevice) {
        // Load from disk into a raw RGBA buffer
        if (img.empty())
            return {};

        if (img.channels() == 3) {
            cv::cvtColor(img, img, cv::ColorConversionCodes::COLOR_BGR2RGBA);
        }
        if (img.channels() == 1) {
            cv::cvtColor(img, img, cv::ColorConversionCodes::COLOR_GRAY2RGBA);
        }

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

        ID3D11Texture2D *pTexture = nullptr;
        D3D11_SUBRESOURCE_DATA subResource;
        subResource.pSysMem = img.data;
        subResource.SysMemPitch = desc.Width * 4;
        subResource.SysMemSlicePitch = 0;
        g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

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

        if (nullptr == texture) {
            return {};
        }

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

        auto data = boost::make_shared<ImGuiD3D11Img>(
                img.cols, img.rows, std::move(texturePtr)
        );

        return data;
    }


} // OwlImGuiDirectX11