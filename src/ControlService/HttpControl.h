// jeremie

#ifndef OWLDISCOVER_HTTPCONTROL_H
#define OWLDISCOVER_HTTPCONTROL_H

#include "../MemoryBoost.h"
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include "../OwlLog/OwlLog.h"
#include "../ConfigLoader/ConfigLoader.h"
#include "./ControlServiceHttpMailBox.h"
#include "./OwlCmd.h"

namespace OwlControlService {

    class HttpControl : public boost::enable_shared_from_this<HttpControl> {
    public:
        HttpControl(
                boost::asio::io_context &ioc,
                boost::shared_ptr<OwlConfigLoader::ConfigLoader> config,
                OwlMailDefine::ControlHttpMailBox &&mailbox
        ) : ioc_(ioc),
            config_(std::move(config)),
            mailbox_(std::move(mailbox)) {


            mailbox_->receiveA2B([this](OwlMailDefine::MailControl2HttpControl &&data) {
                receiveMailHttp(std::move(data));
            });
        }

        ~HttpControl() {
            mailbox_->receiveA2B(nullptr);
        }

    private:

        boost::asio::io_context &ioc_;
        boost::shared_ptr<OwlConfigLoader::ConfigLoader> config_;
        OwlMailDefine::ControlHttpMailBox mailbox_;


        std::atomic_int id_{1};

    public:

    private:

        void receiveMailHttp(OwlMailDefine::MailControl2HttpControl &&data) {
            boost::asio::dispatch(ioc_, [
                    this, self = shared_from_this(), data]() {

                if (data->controlCmdData) {
                    switch (data->controlCmdData->cmd) {
                        case OwlMailDefine::ControlCmd::ping:
                        case OwlMailDefine::ControlCmd::land:
                        case OwlMailDefine::ControlCmd::stop:
                        case OwlMailDefine::ControlCmd::calibrate:
                            sendCmd(data, data->controlCmdData);
                            break;
                        default:
                            break;
                    }
                } else if (data->httpRequestInfo) {
                    send_request(data,
                                 data->httpRequestInfo->data_send,
                                 data->httpRequestInfo->host,
                                 data->httpRequestInfo->port,
                                 data->httpRequestInfo->target);
                }

            });
        }

        void sendCmd(
                const OwlMailDefine::MailControl2HttpControl &data_m,
                const boost::shared_ptr<OwlMailDefine::ControlCmdData> &data);

        void send_request(
                const OwlMailDefine::MailControl2HttpControl &data,
                const std::string &data_send,
                const std::string &host,
                const std::string &port,
                const std::string &target,
                int version = 11);

    public:
    };

} // OwlControlService

#endif //OWLDISCOVER_HTTPCONTROL_H
