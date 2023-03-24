// jeremie

#ifndef TESTIMGUI_IMGUISERVICE_H
#define TESTIMGUI_IMGUISERVICE_H

#include "../MemoryBoost.h"
#include <chrono>
#include <utility>
#include <boost/asio.hpp>
#include "../OwlLog/OwlLog.h"


namespace OwlImGuiService {

    struct ImGuiServiceImpl;

    class ImGuiService : public boost::enable_shared_from_this<ImGuiService> {
    public:
        ImGuiService(
                boost::asio::io_context &ioc
        );

    private:
        boost::asio::io_context &ioc_;

        boost::shared_ptr<ImGuiServiceImpl> impl;

    public:
        void start();

        void clear();

    };

} // OwlImGuiService

#endif //TESTIMGUI_IMGUISERVICE_H
