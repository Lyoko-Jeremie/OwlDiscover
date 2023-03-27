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
#include "../ControlService/ControlServiceUdpMailBox.h"
#include "../ControlService/ControlServiceHttpMailBox.h"

namespace OwlImGuiService {

    void safe_exit();

    struct ImGuiServiceImpl;

    class ImGuiService : public boost::enable_shared_from_this<ImGuiService> {
    public:
        ImGuiService(
                boost::asio::io_context &ioc,
                boost::shared_ptr<OwlConfigLoader::ConfigLoader> config,
                OwlMailDefine::ControlImGuiMailBox &&mailbox_ig,
                OwlMailDefine::ControlMulticastMailbox &&mailbox_mc,
                OwlMailDefine::ControlUdpMailBox &&mailBox_udp,
                OwlMailDefine::ControlHttpMailBox &&mailbox_http
        );

        ~ImGuiService() {
            mailbox_ig_->receiveA2B(nullptr);
            mailbox_mc_->receiveB2A(nullptr);
            mailbox_udp_->receiveB2A(nullptr);
            mailbox_http_->receiveB2A(nullptr);
        }

    private:
        boost::asio::io_context &ioc_;
        boost::shared_ptr<OwlConfigLoader::ConfigLoader> config_;
        OwlMailDefine::ControlMulticastMailbox mailbox_mc_;
        OwlMailDefine::ControlImGuiMailBox mailbox_ig_;
        OwlMailDefine::ControlUdpMailBox mailbox_udp_;
        OwlMailDefine::ControlHttpMailBox mailbox_http_;

        boost::shared_ptr<ImGuiServiceImpl> impl;

    public:
        void start();

        void clear();

        void sendQuery();

        void sendBroadcast();

        void sendCmdUdp(boost::shared_ptr<OwlMailDefine::ControlCmdData> data);

        void sendCmdHttp(boost::shared_ptr<OwlMailDefine::ControlCmdData> data);

        auto config() {
            return config_->config();
        }

    private:

    };

} // OwlImGuiService

#endif //TESTIMGUI_IMGUISERVICE_H
