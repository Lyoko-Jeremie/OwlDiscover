// jeremie

#ifndef OWLDISCOVER_IMGUIDIRECTX11_H
#define OWLDISCOVER_IMGUIDIRECTX11_H

#include "../MemoryBoost.h"

#include <d3d11.h>
#include <opencv2/core.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace OwlImGuiDirectX11 {

    struct ImGuiD3D11Img : public boost::enable_shared_from_this<ImGuiD3D11Img> {

        int width = 0;
        int height = 0;
        boost::shared_ptr<ID3D11ShaderResourceView> texture;
//        std::unique_ptr<ID3D11ShaderResourceView, void (*)(ID3D11ShaderResourceView *)> texture;

        boost::posix_time::ptime time;
        std::string cacheTime;

        ImGuiD3D11Img() = delete;

        ImGuiD3D11Img(
                int width_,
                int height_,
                boost::shared_ptr<ID3D11ShaderResourceView> &&texture_
        ) : width(width_), height(height_), texture(std::move(texture_)) {
            time = boost::posix_time::microsec_clock::local_time();
            cacheTime = getTimeString(time);
        }

        [[nodiscard]] static std::string getTimeString(boost::posix_time::ptime t) {
            auto s = boost::posix_time::to_iso_extended_string(t);
            if (s.size() > 10) {
                s.at(10) = '_';
            }
            return s;
        }

    };

    // img must be cv::ColorConversionCodes::COLOR_BGR2BGRA
    boost::shared_ptr<ImGuiD3D11Img> loadTextureFromMat(
            const cv::Mat &img,
            ID3D11Device *g_pd3dDevice);

} // OwlImGuiDirectX11

#endif //OWLDISCOVER_IMGUIDIRECTX11_H
