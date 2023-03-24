// jeremie

#ifndef OWLACCESSTERMINAL_CONTROLMULTICASTMAIL_H
#define OWLACCESSTERMINAL_CONTROLMULTICASTMAIL_H

#include "../MemoryBoost.h"
#include "../AsyncCallbackMailbox/AsyncCallbackMailbox.h"

namespace OwlMailDefine {

    enum class MulticastCmd {
        query
    };

    struct Multicast2Control;
    struct Control2Multicast {

        MulticastCmd cmd;

        // Multicast2Control.runner = Control2Multicast.callbackRunner
        std::function<void(boost::shared_ptr<Multicast2Control>)> callbackRunner;
    };
    struct Multicast2Control {
        std::function<void(boost::shared_ptr<Multicast2Control>)> runner;
    };


    using ControlMulticastMailbox =
            boost::shared_ptr<
                    OwlAsyncCallbackMailbox::AsyncCallbackMailbox<
                            Control2Multicast,
                            Multicast2Control
                    >
            >;

    using MailControl2Multicast = ControlMulticastMailbox::element_type::A2B_t;
    using MailMulticast2Control = ControlMulticastMailbox::element_type::B2A_t;


} // OwlMailDefine

#endif //OWLACCESSTERMINAL_CONTROLMULTICASTMAIL_H
