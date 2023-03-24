// jeremie

#include "MultiCast.h"
#include <string_view>

namespace OwlMultiCast {

    // ======================================================================================================

    void MultiCast::do_receive() {
        listen_socket_.async_receive_from(
                boost::asio::buffer(receive_data_), receiver_endpoint_,
                [this, sef = shared_from_this()](boost::system::error_code ec, std::size_t length) {
                    if (!ec) {
                        if (length > UDP_Package_Max_Size) {
                            // bad length, ignore it
                            BOOST_LOG_OWL(error) << "MultiCast do_receive() bad length : " << length;
                            do_receive();
                            return;
                        }
                        do_receive_json(length);
                        return;
                    }
                    if (ec == boost::asio::error::operation_aborted) {
                        BOOST_LOG_OWL(trace_multicast) << "MultiCast do_receive() ec operation_aborted";
                        return;
                    }
                    if (ec) {
                        BOOST_LOG_OWL(error) << "MultiCast do_receive() ec " << ec;
                        return;
                    }
                });
    }

    void MultiCast::do_receive_json(std::size_t length) {
        auto data = std::string_view{receive_data_.data(), receive_data_.data() + length};
        BOOST_LOG_OWL(trace_multicast) << "MultiCast do_receive_json() data"
                                       << " receiver_endpoint_ "
                                       << receiver_endpoint_.address() << ":" << receiver_endpoint_.port()
                                       << " " << data;
        boost::system::error_code ecc;
        boost::json::value json_v = boost::json::parse(
                data,
                ecc,
//                &*json_storage_resource_,
                {},
                json_parse_options_
        );
        if (ecc) {
            BOOST_LOG_OWL(warning) << "MultiCast do_receive_json() invalid package " << ecc;
            do_receive();
            return;
        }

        BOOST_LOG_OWL(trace_multicast) << "MultiCast do_receive_json() "
                                       << "receiver_endpoint_ "
                                       << receiver_endpoint_.address() << ":" << receiver_endpoint_.port()
                                       << " " << boost::json::serialize(json_v);

        auto multiCastFlag = get(json_v.as_object(), "MultiCast", std::string{});
        if (multiCastFlag == "Query") {
            // simple ignore self loop package, it many come from self or neighbor
            // ignore it
            do_receive();
            return;
        }
        if (!(multiCastFlag == "Response" || multiCastFlag == "Notice")) {
            // some other unknown package
            BOOST_LOG_OWL(trace_multicast) << "MultiCast do_receive_json() some other unknown package , ignore";
            do_receive();
            return;
        }
        // now we receive a package
        BOOST_LOG_OWL(trace) << "MultiCast do_receive_json() we receive a " << multiCastFlag << " package come from: "
                             << "receiver_endpoint_ "
                             << receiver_endpoint_.address() << ":" << receiver_endpoint_.port();

        {
            auto state = boost::make_shared<OwlDiscoverState::DiscoverState>();
            auto m = boost::make_shared<OwlMailDefine::MailControl2ImGui::element_type>();

            state->items.emplace_back(
                    receiver_endpoint_.address().to_string(),
                    receiver_endpoint_.port()
            );

            m->state = std::move(state);
            mailbox_ig_->sendA2B(std::move(m));
        }

        do_receive();
    }


    // ======================================================================================================


    void MultiCast::do_send() {
        BOOST_LOG_OWL(trace_multicast) << "MultiCast::do_send()";

        sender_socket_.async_send_to(
                boost::asio::buffer(static_send_message_), sender_endpoint_,
                [this, sef = shared_from_this()](boost::system::error_code ec, std::size_t /*length*/) {
                    if (!ec) {
                        BOOST_LOG_OWL(trace_multicast) << "MultiCast do_send() wait to send next";
                        return;
                    }
                    if (ec == boost::asio::error::operation_aborted) {
                        BOOST_LOG_OWL(trace_multicast) << "MultiCast do_send() ec operation_aborted";
                        return;
                    }
                    if (ec) {
                        BOOST_LOG_OWL(error) << "MultiCast do_send() ec " << ec;
                        return;
                    }
                });
    }

    void MultiCast::mailControl(OwlMailDefine::MailControl2Multicast &&data) {
        auto m = boost::make_shared<OwlMailDefine::MailMulticast2Control::element_type>();
        m->runner = data->callbackRunner;

        BOOST_LOG_OWL(trace) << "MultiCast::mailControl ";

        switch (data->cmd) {
            case OwlMailDefine::MulticastCmd::query:
                boost::asio::dispatch(ioc_, [this, self = shared_from_this()]() {
                    BOOST_LOG_OWL(trace) << "MultiCast::mailControl OwlMailDefine::MulticastCmd::query";
                    do_send();
                });
                break;
            default:
                BOOST_LOG_OWL(error) << "MultiCast::mailControl switch (data->cmd) default";
                break;
        }

        mailbox_mc_->sendB2A(std::move(m));
    }


} // OwlMultiCast
