// jeremie

#ifndef OWLDISCOVER_IMGUIDIRECTX11_H
#define OWLDISCOVER_IMGUIDIRECTX11_H

#include "../MemoryBoost.h"

#include <d3d11.h>
#include <opencv2/core.hpp>

namespace OwlImGuiDirectX11 {

    struct ImGuiD3D11Img : public boost::enable_shared_from_this<ImGuiD3D11Img> {

        int width = 0;
        int height = 0;
        boost::shared_ptr<ID3D11ShaderResourceView> texture;
//        std::unique_ptr<ID3D11ShaderResourceView, void (*)(ID3D11ShaderResourceView *)> texture;

        ImGuiD3D11Img() = delete;

        ImGuiD3D11Img(
                int width_,
                int height_,
                boost::shared_ptr<ID3D11ShaderResourceView> &&texture_
        ) : width(width_), height(height_), texture(std::move(texture_)) {}

    };

    // img must be cv::ColorConversionCodes::COLOR_BGR2BGRA
    boost::shared_ptr<ImGuiD3D11Img> loadTextureFromMat(
            const cv::Mat &img,
            ID3D11Device *g_pd3dDevice);

} // OwlImGuiDirectX11

#endif //OWLDISCOVER_IMGUIDIRECTX11_H
