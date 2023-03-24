// jeremie

#ifndef TESTIMGUI_IMGUISERVICE_H
#define TESTIMGUI_IMGUISERVICE_H

#include "../MemoryBoost.h"
#include <chrono>
#include <utility>
#include <boost/asio.hpp>
#include "../OwlLog/OwlLog.h"

#include "../MultiCast/ControlMulticastMail.h"
#include "../ConfigLoader/ConfigLoader.h"
#include "./ControlImGuiMailBox.h"

namespace OwlImGuiService {

    void safe_exit();

    struct ImGuiServiceImpl;

    class ImGuiService : public boost::enable_shared_from_this<ImGuiService> {
    public:
        ImGuiService(
                boost::asio::io_context &ioc,
                boost::shared_ptr<OwlConfigLoader::ConfigLoader> config,
                OwlMailDefine::ControlImGuiMailBox &&mailbox_ig,
                OwlMailDefine::ControlMulticastMailbox &&mailbox_mc
        );

    private:
        boost::asio::io_context &ioc_;
        boost::shared_ptr<OwlConfigLoader::ConfigLoader> config_;
        OwlMailDefine::ControlMulticastMailbox mailbox_mc_;
        OwlMailDefine::ControlImGuiMailBox mailbox_ig_;

        boost::shared_ptr<ImGuiServiceImpl> impl;

    public:
        void start();

        void clear();

    };

} // OwlImGuiService

#endif //TESTIMGUI_IMGUISERVICE_H
