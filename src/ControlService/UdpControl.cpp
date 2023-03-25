// jeremie

#include "UdpControl.h"

namespace OwlControlService {

    template<typename T>
    std::remove_cvref_t<T> get(const boost::json::object &v, boost::string_view key, T &&d) {
        try {
            if (!v.contains(key)) {
                return d;
            }
            auto rr = boost::json::try_value_to<std::remove_cvref_t<T>>(v.at(key));
            return rr.has_value() ? rr.value() : d;
        } catch (std::exception &e) {
            return d;
        }
    }

    void UdpControl::do_receive_json(std::size_t length, std::array<char, UDP_Package_Max_Size> &data_,
                                     const boost::shared_ptr<boost::asio::ip::udp::endpoint> &receiver_endpoint) {
        auto data = std::string_view{data_.data(), data_.data() + length};
        BOOST_LOG_OWL(trace_multicast) << "UdpControl do_receive_json() data"
                                       << " receiver_endpoint_ "
                                       << receiver_endpoint->address() << ":" << receiver_endpoint->port()
                                       << " " << data;
        boost::system::error_code ecc;
        boost::json::value json_v = boost::json::parse(
                data,
                ecc,
                {},
                json_parse_options_
        );
        if (ecc) {
            BOOST_LOG_OWL(warning) << "UdpControl do_receive_json() invalid package " << ecc;
            return;
        }

        BOOST_LOG_OWL(trace_multicast) << "UdpControl do_receive_json() "
                                       << "receiver_endpoint_ "
                                       << receiver_endpoint->address() << ":" << receiver_endpoint->port()
                                       << " " << boost::json::serialize(json_v);

        auto &o = json_v.as_object();
        if (o.contains("MultiCast")) {
            auto multiCastResponse = get(o, "MultiCast", std::string{});
            if (multiCastResponse == "Response") {

                auto m = boost::make_shared<OwlMailDefine::MailUdpControl2Control::element_type>();

                auto controlCmdData = boost::make_shared<OwlDiscoverState::DiscoverStateItem>(
                        receiver_endpoint->address().to_string(),
                        static_cast<int>(receiver_endpoint->port())
                );

                m->discoverStateItem = std::move(controlCmdData);

                send_back_result(std::move(m));
                return;
            }
        }
        if (o.contains("cmdId") &&
            o.contains("packageId") &&
            o.contains("msg") &&
            o.contains("result")) {
            auto cmdId = get(o, "cmdId", 0);
            auto result = get(o, "result", false);
            if (result) {

                auto m = boost::make_shared<OwlMailDefine::MailUdpControl2Control::element_type>();

                auto controlCmdData = boost::make_shared<OwlDiscoverState::DiscoverStateItem>(
                        receiver_endpoint->address().to_string(),
                        static_cast<int>(receiver_endpoint->port())
                );

                m->discoverStateItem = std::move(controlCmdData);

                send_back_result(std::move(m));
                return;
            }
        }

        // ignore

    }
} // OwlControlService
