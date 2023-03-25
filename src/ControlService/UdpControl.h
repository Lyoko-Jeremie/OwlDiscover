// jeremie

#ifndef OWLDISCOVER_UDPCONTROL_H
#define OWLDISCOVER_UDPCONTROL_H

#include "../MemoryBoost.h"
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/json.hpp>
#include <atomic>
#include "./ControlServiceUdpMailBox.h"
#include "../ConfigLoader/ConfigLoader.h"
#include "./OwlCmd.h"

namespace OwlControlService {

    enum {
        UDP_Package_Max_Size = (1024 * 1024 * 6), // 6M
    };

    class UdpControl : public boost::enable_shared_from_this<UdpControl> {
    public:
        UdpControl(
                boost::asio::io_context &ioc,
                boost::shared_ptr<OwlConfigLoader::ConfigLoader> config,
                OwlMailDefine::ControlUdpMailBox &&mailbox
        ) : ioc_(ioc),
            config_(std::move(config)),
            mailbox_(std::move(mailbox)),
            udp_socket_(ioc_, boost::asio::ip::udp::endpoint{
                    boost::asio::ip::make_address("0.0.0.0"),
                    0
            }),
            udp_broadcast_socket_(ioc_, boost::asio::ip::udp::endpoint{
                    boost::asio::ip::udp::v4(),
                    0
            }) {

//            udp_socket_.open(boost::asio::ip::udp::v4());
//            udp_socket_.bind(boost::asio::ip::udp::endpoint{
//                    boost::asio::ip::make_address("0.0.0.0"),
//                    0
//            });

            udp_broadcast_socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
            udp_broadcast_socket_.set_option(boost::asio::socket_base::broadcast(true));

            broadcast_endpoint_ = decltype(broadcast_endpoint_){
                    boost::asio::ip::address_v4::broadcast(),
                    boost::lexical_cast<boost::asio::ip::port_type>(config_->config().CommandServiceUdpPort),
            };

            json_parse_options_.allow_comments = true;
            json_parse_options_.allow_trailing_commas = true;
            json_parse_options_.max_depth = 5;

            mailbox_->receiveA2B([this](OwlMailDefine::MailControl2UdpControl &&data) {
                receiveMailUdp(std::move(data));
            });
        }

        ~UdpControl() {
            mailbox_->receiveA2B(nullptr);
        }

    private:
        boost::asio::io_context &ioc_;
        boost::shared_ptr<OwlConfigLoader::ConfigLoader> config_;
        OwlMailDefine::ControlUdpMailBox mailbox_;

        boost::asio::ip::udp::socket udp_socket_;

        boost::asio::ip::udp::socket udp_broadcast_socket_;
        boost::asio::ip::udp::endpoint broadcast_endpoint_;

        boost::json::parse_options json_parse_options_;


        std::atomic_int id_{1};

    public:

        void start() {
            do_receive();
            do_receive_broadcast();
        }

    private:

        void do_receive_json(
                std::size_t length,
                std::array<char, UDP_Package_Max_Size> &data_,
                const boost::shared_ptr<boost::asio::ip::udp::endpoint> &receiver_endpoint,
                bool updateOnly = false);

        void do_receive() {
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
                            BOOST_LOG_OWL(error) << "UdpControl do_receive() some : " << data;
                            do_receive_json(length, *receive_data, receiver_endpoint, true);
                            do_receive();
                            return;
                        }
                        if (ec == boost::asio::error::operation_aborted) {
                            BOOST_LOG_OWL(trace_multicast) << "UdpControl do_receive() ec operation_aborted";
                            return;
                        }
                        if (ec) {
                            BOOST_LOG_OWL(error) << "UdpControl do_receive() ec " << ec;
                            return;
                        }
                    });
        }

        void do_receive_broadcast() {
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
                            BOOST_LOG_OWL(error) << "UdpControl do_receive_broadcast() some : " << data;
                            do_receive_json(length, *receive_data, receiver_endpoint, false);
                            do_receive_broadcast();
                            return;
                        }
                        if (ec == boost::asio::error::operation_aborted) {
                            BOOST_LOG_OWL(trace_multicast) << "UdpControl do_receive_broadcast() ec operation_aborted";
                            return;
                        }
                        if (ec) {
                            BOOST_LOG_OWL(error) << "UdpControl do_receive_broadcast() ec " << ec;
                            return;
                        }
                    });
        }

        void receiveMailUdp(OwlMailDefine::MailControl2UdpControl &&data) {
            boost::asio::dispatch(ioc_, [
                    this, self = shared_from_this(), data]() {
                auto m = boost::make_shared<OwlMailDefine::MailUdpControl2Control::element_type>();
                m->runner = data->callbackRunner;

                if (data->controlCmdData) {
                    switch (data->controlCmdData->cmd) {
                        case OwlMailDefine::ControlCmd::ping:
                        case OwlMailDefine::ControlCmd::land:
                        case OwlMailDefine::ControlCmd::stop:
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

        void sendBroadcast(const boost::shared_ptr<OwlMailDefine::ControlCmdData> &data) {
            BOOST_ASSERT(data);
            BOOST_LOG_OWL(trace) << "UdpControl sendBroadcast data " << data->ip << " " << static_cast<int>(data->cmd);

            boost::shared_ptr<std::string> S = boost::make_shared<std::string>(
                    boost::json::serialize(boost::json::value{
                            {"cmdId",     static_cast<int>(OwlCmd::OwlCmdEnum::ping)},
                            {"packageId", ++id_},
                    }));

            udp_broadcast_socket_.async_send_to(
                    boost::asio::buffer(*S), broadcast_endpoint_,
                    [
                            this, self = shared_from_this(), S
                    ](boost::system::error_code ec, std::size_t /*length*/) {
                        if (ec) {
                            BOOST_LOG_OWL(error) << "UdpControl sendBroadcast udp_broadcast_socket_.async_send_to ec "
                                                 << ec;
                            return;
                        }
                        BOOST_LOG_OWL(trace) << "UdpControl sendBroadcast udp_broadcast_socket_.async_send_to ok";
                    });


        }

        void sendCmd(const boost::shared_ptr<OwlMailDefine::ControlCmdData> &data) {
            BOOST_ASSERT(data);
            BOOST_LOG_OWL(trace) << "UdpControl sendCmd data " << data->ip << " " << static_cast<int>(data->cmd);

            boost::shared_ptr<std::string> S;
            switch (data->cmd) {
                case OwlMailDefine::ControlCmd::ping:
                    S = boost::make_shared<std::string>(boost::json::serialize(boost::json::value{
                            {"cmdId",     static_cast<int>(OwlCmd::OwlCmdEnum::ping)},
                            {"packageId", ++id_},
                    }));
                    break;
                case OwlMailDefine::ControlCmd::land:
                    S = boost::make_shared<std::string>(boost::json::serialize(boost::json::value{
                            {"cmdId",     static_cast<int>(OwlCmd::OwlCmdEnum::land)},
                            {"packageId", ++id_},
                    }));
                    break;
                case OwlMailDefine::ControlCmd::stop:
                    S = boost::make_shared<std::string>(boost::json::serialize(boost::json::value{
                            {"cmdId",     static_cast<int>(OwlCmd::OwlCmdEnum::emergencyStop)},
                            {"packageId", ++id_},
                    }));
                    break;
                case OwlMailDefine::ControlCmd::calibrate:
                    S = boost::make_shared<std::string>(boost::json::serialize(boost::json::value{
                            {"cmdId",     static_cast<int>(OwlCmd::OwlCmdEnum::calibrate)},
                            {"packageId", ++id_},
                    }));
                    break;
                case OwlMailDefine::ControlCmd::query:
                    S = boost::make_shared<std::string>(boost::json::serialize(boost::json::value{
                            {"MultiCast", "Query"},
                    }));
                    break;
                default:
                    BOOST_LOG_OWL(error) << "UdpControl sendCmd switch (data->cmd)  default";
                    return;
            }

            boost::asio::ip::udp::endpoint remote_endpoint{
                    boost::asio::ip::make_address(data->ip),
                    boost::lexical_cast<boost::asio::ip::port_type>(config_->config().CommandServiceUdpPort),
            };

            if (data->cmd == OwlMailDefine::ControlCmd::query) {
                remote_endpoint.port(config_->config().multicast_port);
            }

            BOOST_ASSERT(S);
            udp_socket_.async_send_to(
                    boost::asio::buffer(*S), remote_endpoint,
                    [
                            this, self = shared_from_this(), S
                    ](boost::system::error_code ec, std::size_t /*length*/) {
                        if (ec) {
                            BOOST_LOG_OWL(error) << "UdpControl sendCmd udp_socket_.async_send_to ec " << ec;
                            return;
                        }
                        BOOST_LOG_OWL(trace) << "UdpControl sendCmd udp_socket_.async_send_to ok";
                    });
        }

    public:

    private:

        void send_back_result(OwlMailDefine::MailUdpControl2Control &&m) {
            mailbox_->sendB2A(std::move(m));
        }

    };

} // OwlControlService

#endif //OWLDISCOVER_UDPCONTROL_H
