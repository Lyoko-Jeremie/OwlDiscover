// jeremie

#ifndef OWLDISCOVER_CONTROLSERVICEHTTPMAILBOX_H
#define OWLDISCOVER_CONTROLSERVICEHTTPMAILBOX_H

#include <utility>

#include "../MemoryBoost.h"
#include "./ControlServiceUdpMailBox.h"


namespace OwlMailDefine {

    struct HttpRequestInfo : public boost::enable_shared_from_this<HttpRequestInfo> {
        std::string host;
        std::string port;
        std::string target;
        std::string data_send;

        HttpRequestInfo(
                std::string host_,
                std::string port_,
                std::string target_,
                std::string data_send_
        ) : host(std::move(host_)),
            port(std::move(port_)),
            target(std::move(target_)),
            data_send(std::move(data_send_)) {}
    };

    struct HttpControl2Control;
    struct Control2HttpControl {

        boost::shared_ptr<HttpRequestInfo> httpRequestInfo;

        boost::shared_ptr<ControlCmdData> controlCmdData;

        // HttpControl2Control.runner = Control2HttpControl.callbackRunner
        std::function<void(boost::shared_ptr<HttpControl2Control>)> callbackRunner;
    };
    struct HttpControl2Control {

        boost::shared_ptr<OwlDiscoverState::DiscoverStateItem> discoverStateItem;
        boost::shared_ptr<std::string> httpResponseData;

        bool updateOnly = false;

        std::function<void(boost::shared_ptr<HttpControl2Control>)> runner;
    };

    using ControlHttpMailBox =
            boost::shared_ptr<
                    OwlAsyncCallbackMailbox::AsyncCallbackMailbox<
                            Control2HttpControl,
                            HttpControl2Control
                    >
            >;

    using MailControl2HttpControl = ControlHttpMailBox::element_type::A2B_t;
    using MailHttpControl2Control = ControlHttpMailBox::element_type::B2A_t;


}

#endif //OWLDISCOVER_CONTROLSERVICEHTTPMAILBOX_H
