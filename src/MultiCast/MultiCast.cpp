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
        if (multiCastFlag != "Query") {
            if (multiCastFlag == "Notice") {
                // simple ignore Notice package, it many come from self or neighbor
                do_response();
                return;
            }
            // ignore it
            BOOST_LOG_OWL(trace_multicast) << "MultiCast do_receive_json() (multiCastFlag != Query) , ignore";
            do_response();
            return;
        }
        // now we receive a Query package, so we need response it
        BOOST_LOG_OWL(trace) << "MultiCast do_receive_json() we receive a Query package come from: "
                             << "receiver_endpoint_ "
                             << receiver_endpoint_.address() << ":" << receiver_endpoint_.port();

        // TODO response_message_
        response_message_ = R"({"MultiCast":"Response"})";
        do_response();
    }

    void MultiCast::do_response() {

        BOOST_LOG_OWL(trace_multicast) << "MultiCast do_response() "
                                       << "receiver_endpoint_ "
                                       << receiver_endpoint_.address() << ":" << receiver_endpoint_.port()
                                       << " " << response_message_;

        sender_socket_.async_send_to(
                boost::asio::buffer(response_message_), receiver_endpoint_,
                [this, sef = shared_from_this()](boost::system::error_code ec, std::size_t /*length*/) {
                    if (!ec) {
                        do_receive();
                        return;
                    }
                    if (ec == boost::asio::error::operation_aborted) {
                        BOOST_LOG_OWL(trace_multicast) << "MultiCast do_response() ec operation_aborted";
                        return;
                    }
                    if (ec) {
                        BOOST_LOG_OWL(error) << "MultiCast do_response() ec " << ec;
                        return;
                    }
                });
    }


    // ======================================================================================================


    void MultiCast::do_send() {

        timer_.cancel();
        sender_socket_.async_send_to(
                boost::asio::buffer(static_send_message_), sender_endpoint_,
                [this, sef = shared_from_this()](boost::system::error_code ec, std::size_t /*length*/) {
                    if (!ec) {
                        BOOST_LOG_OWL(trace_multicast) << "MultiCast do_send() wait to send next";
                        do_timeout();
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

    void MultiCast::do_timeout() {
        timer_.expires_after(std::chrono::seconds(multicast_interval_));
        timer_.async_wait(
                [this, sef = shared_from_this()](boost::system::error_code ec) {
                    timer_.cancel();
                    if (!ec) {
                        BOOST_LOG_OWL(trace_multicast) << "MultiCast do_timeout() to send next";
                        do_send();
                    }
                    if (ec == boost::asio::error::operation_aborted) {
                        BOOST_LOG_OWL(trace_multicast) << "MultiCast do_timeout() ec operation_aborted";
                        return;
                    }
                    if (ec) {
                        BOOST_LOG_OWL(error) << "MultiCast do_timeout() ec " << ec;
                        return;
                    }
                });
    }

} // OwlMultiCast
