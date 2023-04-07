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

    void UdpControl::do_receive_json(
            std::size_t length, std::array<char, UDP_Package_Max_Size> &data_,
            const boost::shared_ptr<boost::asio::ip::udp::endpoint> &receiver_endpoint,
            bool updateOnly) {
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
            BOOST_LOG_OWL(warning) << "UdpControl do_receive_json() invalid package " << ecc.what();
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
            auto packageId = get(o, "packageId", 0);
            auto clientId = get(o, "clientId", 0);
            auto result = get(o, "result", false);
            if (result) {

                auto m = boost::make_shared<OwlMailDefine::MailUdpControl2Control::element_type>();

                auto controlCmdData = boost::make_shared<OwlDiscoverState::DiscoverStateItem>(
                        receiver_endpoint->address().to_string(),
                        static_cast<int>(receiver_endpoint->port())
                );

                auto programVersion = get<std::string>(o, "Version", "");
                if (!programVersion.empty()) {
                    auto gitRev = get<std::string>(o, "GitRev", "");
                    auto buildTime = get<std::string>(o, "BuildTime", "");

                    controlCmdData->programVersion = programVersion;
                    controlCmdData->gitRev = gitRev;
                    controlCmdData->buildTime = buildTime;
                }

                {
                    m->packageSendInfo = boost::make_shared<OwlDiscoverState::PackageSendInfo>(
                            receiver_endpoint->address().to_string(),
                            packageId,
                            cmdId,
                            clientId,
                            OwlDiscoverState::PackageSendInfoDirectEnum::in
                    );
                    m->port = receiver_endpoint->port();
                }

                m->discoverStateItem = std::move(controlCmdData);
                m->updateOnly = updateOnly;

                send_back_result(std::move(m));
                return;
            }
        }

        // ignore

    }

    void UdpControl::do_receive() {
        auto receiver_endpoint = boost::make_shared<boost::asio::ip::udp::endpoint>();
        auto receive_data = boost::make_shared<std::array<char, UDP_Package_Max_Size>>();
        udp_socket_.async_receive_from(
                boost::asio::buffer(*receive_data), *receiver_endpoint,
                [
                        this, sef = shared_from_this(), receiver_endpoint, receive_data
                ](boost::system::error_code ec, std::size_t length) {
                    if (!ec) {
                        if (length > UDP_Package_Max_Size) {
                            // bad length, ignore it
                            BOOST_LOG_OWL(error) << "UdpControl do_receive() bad length : " << length;
                            do_receive();
                            return;
                        }
                        auto data = std::string_view{receive_data->data(), receive_data->data() + length};
                        BOOST_LOG_OWL(trace_multicast) << "UdpControl do_receive() some : " << data;
                        do_receive_json(length, *receive_data, receiver_endpoint, true);
                        do_receive();
                        return;
                    }
                    if (ec == boost::asio::error::operation_aborted) {
                        BOOST_LOG_OWL(trace_multicast) << "UdpControl do_receive() ec operation_aborted";
                        return;
                    }
                    if (ec) {
                        BOOST_LOG_OWL(error) << "UdpControl do_receive() ec " << ec.what();
                        do_receive();
                        return;
                    }
                });
    }

    void UdpControl::do_receive_broadcast() {
        auto receiver_endpoint = boost::make_shared<boost::asio::ip::udp::endpoint>();
        auto receive_data = boost::make_shared<std::array<char, UDP_Package_Max_Size>>();
        udp_broadcast_socket_.async_receive_from(
                boost::asio::buffer(*receive_data), *receiver_endpoint,
                [
                        this, sef = shared_from_this(), receiver_endpoint, receive_data
                ](boost::system::error_code ec, std::size_t length) {
                    if (!ec) {
                        if (length > UDP_Package_Max_Size) {
                            // bad length, ignore it
                            BOOST_LOG_OWL(error) << "UdpControl do_receive_broadcast() bad length : " << length;
                            do_receive_broadcast();
                            return;
                        }
                        auto data = std::string_view{receive_data->data(), receive_data->data() + length};
                        BOOST_LOG_OWL(trace_multicast) << "UdpControl do_receive_broadcast() some : " << data;
                        do_receive_json(length, *receive_data, receiver_endpoint, false);
                        do_receive_broadcast();
                        return;
                    }
                    if (ec == boost::asio::error::operation_aborted) {
                        BOOST_LOG_OWL(trace_multicast) << "UdpControl do_receive_broadcast() ec operation_aborted";
                        return;
                    }
                    if (ec) {
                        BOOST_LOG_OWL(error) << "UdpControl do_receive_broadcast() ec " << ec.what();
                        return;
                    }
                });
    }

    void UdpControl::receiveMailUdp(OwlMailDefine::MailControl2UdpControl &&data) {
        boost::asio::dispatch(ioc_, [
                this, self = shared_from_this(), data]() {
            auto m = boost::make_shared<OwlMailDefine::MailUdpControl2Control::element_type>();
            m->runner = data->callbackRunner;

            if (data->controlCmdData) {
                switch (data->controlCmdData->cmd) {
                    case OwlMailDefine::ControlCmd::ping:
                    case OwlMailDefine::ControlCmd::stop:
                    case OwlMailDefine::ControlCmd::keep:
                    case OwlMailDefine::ControlCmd::land:
                    case OwlMailDefine::ControlCmd::takeoff:
                    case OwlMailDefine::ControlCmd::cw:
                    case OwlMailDefine::ControlCmd::ccw:
                    case OwlMailDefine::ControlCmd::up:
                    case OwlMailDefine::ControlCmd::down:
                    case OwlMailDefine::ControlCmd::left:
                    case OwlMailDefine::ControlCmd::right:
                    case OwlMailDefine::ControlCmd::forward:
                    case OwlMailDefine::ControlCmd::back:
                    case OwlMailDefine::ControlCmd::calibrate:
                    case OwlMailDefine::ControlCmd::query:
                        sendCmd(data->controlCmdData);
                        break;
                    case OwlMailDefine::ControlCmd::broadcast:
                        sendBroadcast(data->controlCmdData);
                        break;
                    default:
                        break;
                }
            }

            mailbox_->sendB2A(std::move(m));
        });
    }

    void UdpControl::sendBroadcast(const boost::shared_ptr<OwlMailDefine::ControlCmdData> &data) {
        BOOST_ASSERT(data);
        BOOST_LOG_OWL(trace_udp) << "UdpControl sendBroadcast data " << data->ip << " " << static_cast<int>(data->cmd);

        boost::shared_ptr<std::string> S = boost::make_shared<std::string>(
                boost::json::serialize(boost::json::value{
                        {"cmdId",     static_cast<int>(OwlCmd::OwlCmdEnum::ping)},
                        {"packageId", ++OwlCmd::packageId},
                        {"clientId",  OwlLog::globalClientId},
                }));

        udp_broadcast_socket_.async_send_to(
                boost::asio::buffer(*S), broadcast_endpoint_,
                [
                        this, self = shared_from_this(), S
                ](boost::system::error_code ec, std::size_t /*length*/) {
                    if (ec) {
                        BOOST_LOG_OWL(error) << "UdpControl sendBroadcast udp_broadcast_socket_.async_send_to ec "
                                             << ec.what();
                        return;
                    }
                    BOOST_LOG_OWL(trace_udp) << "UdpControl sendBroadcast udp_broadcast_socket_.async_send_to ok";
                });


    }

    void UdpControl::sendCmd(const boost::shared_ptr<OwlMailDefine::ControlCmdData> &data) {
        BOOST_ASSERT(data);
        BOOST_LOG_OWL(trace_udp) << "UdpControl sendCmd data " << data->ip << " " << static_cast<int>(data->cmd);

        boost::shared_ptr <boost::json::value> V;
        switch (data->cmd) {
            case OwlMailDefine::ControlCmd::ping:
                V = boost::make_shared<boost::json::value>(boost::json::value{
                        {"cmdId",     static_cast<int>(OwlCmd::OwlCmdEnum::ping)},
                        {"packageId", ++OwlCmd::packageId},
                        {"clientId",  OwlLog::globalClientId},
                });
                break;
            case OwlMailDefine::ControlCmd::stop:
                V = boost::make_shared<boost::json::value>(boost::json::value{
                        {"cmdId",     static_cast<int>(OwlCmd::OwlCmdEnum::emergencyStop)},
                        {"packageId", ++OwlCmd::packageId},
                        {"clientId",  OwlLog::globalClientId},
                });
                break;
            case OwlMailDefine::ControlCmd::calibrate:
                V = boost::make_shared<boost::json::value>(boost::json::value{
                        {"cmdId",     static_cast<int>(OwlCmd::OwlCmdEnum::calibrate)},
                        {"packageId", ++OwlCmd::packageId},
                        {"clientId",  OwlLog::globalClientId},
                });
                break;
            case OwlMailDefine::ControlCmd::keep:
                V = boost::make_shared<boost::json::value>(boost::json::value{
                        {"cmdId",     static_cast<int>(OwlCmd::OwlCmdEnum::keep)},
                        {"packageId", ++OwlCmd::packageId},
                        {"clientId",  OwlLog::globalClientId},
                });
                break;
            case OwlMailDefine::ControlCmd::land:
                V = boost::make_shared<boost::json::value>(boost::json::value{
                        {"cmdId",     static_cast<int>(OwlCmd::OwlCmdEnum::land)},
                        {"packageId", ++OwlCmd::packageId},
                        {"clientId",  OwlLog::globalClientId},
                });
                break;
            case OwlMailDefine::ControlCmd::takeoff:
                V = boost::make_shared<boost::json::value>(boost::json::value{
                        {"cmdId",     static_cast<int>(OwlCmd::OwlCmdEnum::takeoff)},
                        {"distance",  50},
                        {"packageId", ++OwlCmd::packageId},
                        {"clientId",  OwlLog::globalClientId},
                });
                break;
            case OwlMailDefine::ControlCmd::cw:
                V = boost::make_shared<boost::json::value>(boost::json::value{
                        {"cmdId",     static_cast<int>(OwlCmd::OwlCmdEnum::rotate)},
                        {"rotate",    static_cast<int>(OwlCmd::OwlCmdRotateEnum::cw)},
                        {"rote",      45},
                        {"packageId", ++OwlCmd::packageId},
                        {"clientId",  OwlLog::globalClientId},
                });
                break;
            case OwlMailDefine::ControlCmd::ccw:
                V = boost::make_shared<boost::json::value>(boost::json::value{
                        {"cmdId",     static_cast<int>(OwlCmd::OwlCmdEnum::rotate)},
                        {"rotate",    static_cast<int>(OwlCmd::OwlCmdRotateEnum::ccw)},
                        {"rote",      45},
                        {"packageId", ++OwlCmd::packageId},
                        {"clientId",  OwlLog::globalClientId},
                });
                break;
            case OwlMailDefine::ControlCmd::up:
                V = boost::make_shared<boost::json::value>(boost::json::value{
                        {"cmdId",     static_cast<int>(OwlCmd::OwlCmdEnum::move)},
                        {"forward",   static_cast<int>(OwlCmd::OwlCmdMoveEnum::up)},
                        {"distance",  50},
                        {"packageId", ++OwlCmd::packageId},
                        {"clientId",  OwlLog::globalClientId},
                });
                break;
            case OwlMailDefine::ControlCmd::down:
                V = boost::make_shared<boost::json::value>(boost::json::value{
                        {"cmdId",     static_cast<int>(OwlCmd::OwlCmdEnum::move)},
                        {"forward",   static_cast<int>(OwlCmd::OwlCmdMoveEnum::down)},
                        {"distance",  50},
                        {"packageId", ++OwlCmd::packageId},
                        {"clientId",  OwlLog::globalClientId},
                });
                break;
            case OwlMailDefine::ControlCmd::left:
                V = boost::make_shared<boost::json::value>(boost::json::value{
                        {"cmdId",     static_cast<int>(OwlCmd::OwlCmdEnum::move)},
                        {"forward",   static_cast<int>(OwlCmd::OwlCmdMoveEnum::left)},
                        {"distance",  50},
                        {"packageId", ++OwlCmd::packageId},
                        {"clientId",  OwlLog::globalClientId},
                });
                break;
            case OwlMailDefine::ControlCmd::right:
                V = boost::make_shared<boost::json::value>(boost::json::value{
                        {"cmdId",     static_cast<int>(OwlCmd::OwlCmdEnum::move)},
                        {"forward",   static_cast<int>(OwlCmd::OwlCmdMoveEnum::right)},
                        {"distance",  50},
                        {"packageId", ++OwlCmd::packageId},
                        {"clientId",  OwlLog::globalClientId},
                });
                break;
            case OwlMailDefine::ControlCmd::forward:
                V = boost::make_shared<boost::json::value>(boost::json::value{
                        {"cmdId",     static_cast<int>(OwlCmd::OwlCmdEnum::move)},
                        {"forward",   static_cast<int>(OwlCmd::OwlCmdMoveEnum::forward)},
                        {"distance",  50},
                        {"packageId", ++OwlCmd::packageId},
                        {"clientId",  OwlLog::globalClientId},
                });
                break;
            case OwlMailDefine::ControlCmd::back:
                V = boost::make_shared<boost::json::value>(boost::json::value{
                        {"cmdId",     static_cast<int>(OwlCmd::OwlCmdEnum::move)},
                        {"forward",   static_cast<int>(OwlCmd::OwlCmdMoveEnum::back)},
                        {"distance",  50},
                        {"packageId", ++OwlCmd::packageId},
                        {"clientId",  OwlLog::globalClientId},
                });
                break;
            case OwlMailDefine::ControlCmd::query:
                break;
            default:
                BOOST_LOG_OWL(error) << "UdpControl sendCmd switch (data->cmd)  default";
                return;
        }

        boost::asio::ip::udp::endpoint remote_endpoint{
                boost::asio::ip::make_address(data->ip),
                boost::lexical_cast<boost::asio::ip::port_type>(config_->config().CommandServiceUdpPort),
        };

        boost::shared_ptr <std::string> S;

        if (data->cmd == OwlMailDefine::ControlCmd::query) {
            S = boost::make_shared<std::string>(boost::json::serialize(boost::json::value{
                    {"MultiCast", "Query"},
            }));
            remote_endpoint.port(config_->config().multicast_port);
        } else {
            BOOST_ASSERT(V);
            S = boost::make_shared<std::string>(boost::json::serialize(*V));
        }

        BOOST_ASSERT(S);
        udp_socket_.async_send_to(
                boost::asio::buffer(*S), remote_endpoint,
                [
                        this, self = shared_from_this(), S, V, remote_endpoint
                ](boost::system::error_code ec, std::size_t /*length*/) {
                    if (ec) {
                        BOOST_LOG_OWL(error) << "UdpControl sendCmd udp_socket_.async_send_to ec " << ec.what();
                        return;
                    }

                    if (V) {
                        auto mm = boost::make_shared<OwlMailDefine::MailUdpControl2Control::element_type>();
                        auto &ooo = V->as_object();
                        mm->packageSendInfo = boost::make_shared<OwlDiscoverState::PackageSendInfo>(
                                remote_endpoint.address().to_string(),
                                get(ooo, "packageId", 0),
                                get(ooo, "cmdId", 0),
                                get(ooo, "clientId", 0),
                                OwlDiscoverState::PackageSendInfoDirectEnum::out
                        );
                        mm->port = remote_endpoint.port();
                        mailbox_->sendB2A(std::move(mm));
                    }
                    BOOST_LOG_OWL(trace_udp) << "UdpControl sendCmd udp_socket_.async_send_to ok";
                });
    }
} // OwlControlService
