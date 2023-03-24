// jeremie
#ifndef OWLDISCOVER_CONTROLIMGUIMAILBOX_H
#define OWLDISCOVER_CONTROLIMGUIMAILBOX_H

#include "../MemoryBoost.h"
#include "../AsyncCallbackMailbox/AsyncCallbackMailbox.h"
#include "../DiscoverState/DiscoverState.h"

namespace OwlMailDefine {

    struct ImGui2Control;
    struct Control2ImGui {

        boost::shared_ptr<OwlDiscoverState::DiscoverState> state;

        // ImGui2Control.runner = Control2ImGui.callbackRunner
        std::function<void(boost::shared_ptr<ImGui2Control>)> callbackRunner;
    };
    struct ImGui2Control {
        std::function<void(boost::shared_ptr<ImGui2Control>)> runner;
    };


    using ControlImGuiMailBox =
            boost::shared_ptr<
                    OwlAsyncCallbackMailbox::AsyncCallbackMailbox<
                            Control2ImGui,
                            ImGui2Control
                    >
            >;

    using MailControl2ImGui = ControlImGuiMailBox::element_type::A2B_t;
    using MailImGui2Control = ControlImGuiMailBox::element_type::B2A_t;


} // OwlMailDefine

#endif //OWLDISCOVER_CONTROLIMGUIMAILBOX_H
