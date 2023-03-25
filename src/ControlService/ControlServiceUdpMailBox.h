// jermie

#ifndef OWLDISCOVER_CONTROLSERVICEUDPMAILBOX_H
#define OWLDISCOVER_CONTROLSERVICEUDPMAILBOX_H

#include "../MemoryBoost.h"
#include "../AsyncCallbackMailbox/AsyncCallbackMailbox.h"
#include "../DiscoverState/DiscoverState.h"

namespace OwlMailDefine {

    enum class ControlCmd {
        ping,
        stop,
        land,
        calibrate,
        query,
        broadcast,
    };

    struct ControlCmdData : public boost::enable_shared_from_this<ControlCmdData> {
        std::string ip;
        ControlCmd cmd;
    };

    struct UdpControl2Control;
    struct Control2UdpControl {

        boost::shared_ptr<ControlCmdData> controlCmdData;

        // UdpControl2Control.runner = Control2UdpControl.callbackRunner
        std::function<void(boost::shared_ptr<UdpControl2Control>)> callbackRunner;
    };
    struct UdpControl2Control {

        boost::shared_ptr<OwlDiscoverState::DiscoverStateItem> discoverStateItem;

        bool updateOnly = false;


        std::function<void(boost::shared_ptr<UdpControl2Control>)> runner;
    };

    using ControlUdpMailBox =
            boost::shared_ptr<
                    OwlAsyncCallbackMailbox::AsyncCallbackMailbox<
                            Control2UdpControl,
                            UdpControl2Control
                    >
            >;

    using MailControl2UdpControl = ControlUdpMailBox::element_type::A2B_t;
    using MailUdpControl2Control = ControlUdpMailBox::element_type::B2A_t;

}

#endif //OWLDISCOVER_CONTROLSERVICEUDPMAILBOX_H
