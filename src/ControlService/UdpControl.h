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

namespace OwlControlService {

    // follow table come from OwlAccessTerminal
    enum class OwlCmdEnum {
        ping = 0,
        emergencyStop = 120,
        unlock = 92,

        calibrate = 90,
        breakCmd = 10,
        takeoff = 11,
        land = 12,
        move = 13,
        rotate = 14,
        keep = 15,
        gotoCmd = 16,

        led = 17,
        high = 18,
        speed = 19,
        flyMode = 20,

        joyCon = 60,
        joyConSimple = 61,
        joyConGyro = 62,

    };
    enum class OwlCmdRotateEnum {
        cw = 1,
        ccw = 2,
    };
    enum class OwlCmdMoveEnum {
        up = 1,
        down = 2,
        left = 3,
        right = 4,
        forward = 5,
        back = 6,
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
            udp_socket_(ioc_) {

            udp_socket_.open(boost::asio::ip::udp::v4());

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

        std::atomic_int id_{1};

    public:

        void start() {
            // noop
        }

    private:

        void receiveMailUdp(OwlMailDefine::MailControl2UdpControl &&data) {
            boost::asio::dispatch(ioc_, [
                    this, self = shared_from_this(), data]() {
                auto m = boost::make_shared<OwlMailDefine::MailUdpControl2Control::element_type>();
                m->runner = data->callbackRunner;

                if (data->controlCmdData) {
                    switch (data->controlCmdData->cmd) {
                        case OwlMailDefine::ControlCmd::land:
                        case OwlMailDefine::ControlCmd::stop:
                            sendCmd(data->controlCmdData);
                            break;
                        default:
                            break;
                    }
                }

                mailbox_->sendB2A(std::move(m));
            });
        }

        void sendCmd(const boost::shared_ptr<OwlMailDefine::ControlCmdData> &data) {
            BOOST_ASSERT(data);
            BOOST_LOG_OWL(trace) << "UdpControl sendCmd data " << data->ip << " " << static_cast<int>(data->cmd);

            boost::shared_ptr<std::string> S;
            switch (data->cmd) {
                case OwlMailDefine::ControlCmd::land:
                    S = boost::make_shared<std::string>(boost::json::serialize(boost::json::value{
                            {"cmdId",     static_cast<int>(OwlCmdEnum::land)},
                            {"packageId", ++id_},
                    }));
                    break;
                case OwlMailDefine::ControlCmd::stop:
                    S = boost::make_shared<std::string>(boost::json::serialize(boost::json::value{
                            {"cmdId",     static_cast<int>(OwlCmdEnum::emergencyStop)},
                            {"packageId", ++id_},
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

    };

} // OwlControlService

#endif //OWLDISCOVER_UDPCONTROL_H
