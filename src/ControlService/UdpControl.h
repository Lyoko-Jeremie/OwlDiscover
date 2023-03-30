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

        void do_receive();

        void do_receive_broadcast();

        void receiveMailUdp(OwlMailDefine::MailControl2UdpControl &&data);

        void sendBroadcast(const boost::shared_ptr<OwlMailDefine::ControlCmdData> &data);

        void sendCmd(const boost::shared_ptr<OwlMailDefine::ControlCmdData> &data);

    public:

    private:

        void send_back_result(OwlMailDefine::MailUdpControl2Control &&m) {
            mailbox_->sendB2A(std::move(m));
        }

    };

} // OwlControlService

#endif //OWLDISCOVER_UDPCONTROL_H
