// jeremie

#ifndef OWLAPRILTAGPROCESSOR_GETIMAGE_H
#define OWLAPRILTAGPROCESSOR_GETIMAGE_H

#include <string>
#include <memory>
#include <functional>

#include <boost/log/trivial.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <opencv2/opencv.hpp>

namespace OwlGetImage {

    class GetImageSession;

    using CallbackFunctionType = std::function<void(boost::beast::error_code ec, bool ok, cv::Mat img)>;

    class GetImage {
    public:
        explicit
        GetImage(boost::asio::io_context &ioc)
                : ioc_(ioc) {
        }

    private:
        boost::asio::io_context &ioc_;

    public:

        long timeoutMs = 1 * 1000;

        void test(
                const std::string &host,
                const std::string &port,
                const std::string &target,
                int version);

        std::shared_ptr<GetImageSession> get(
                const std::string &host,
                const std::string &port,
                const std::string &target,
                int version,
                CallbackFunctionType &&callback);
    };

} // OwlGetImage

#endif //OWLAPRILTAGPROCESSOR_GETIMAGE_H
