// jeremie

#ifndef OWLDISCOVER_HTTPCONTROL_H
#define OWLDISCOVER_HTTPCONTROL_H

#include "../MemoryBoost.h"
#include <functional>
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

            json_parse_options_.allow_comments = true;
            json_parse_options_.allow_trailing_commas = true;
            json_parse_options_.max_depth = 5;

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

        boost::json::parse_options json_parse_options_;

        std::atomic_int id_{1};

    public:

        void start() {
            // noop
        }

    private:

        void receiveMailHttp(OwlMailDefine::MailControl2HttpControl &&data);

        void sendCmd(
                const OwlMailDefine::MailControl2HttpControl &data_m,
                const boost::shared_ptr<OwlMailDefine::ControlCmdData> &data);

        void send_request(
                std::function<void(std::pair<bool, std::string>)> &&resultCallback,
                const OwlMailDefine::MailControl2HttpControl &data,
                const std::string &data_send,
                const std::string &host,
                const std::string &port,
                const std::string &target,
                int version = 11);

        std::pair<bool, boost::json::value> analysis_receive_json(const std::string &data_);

    public:
    };

} // OwlControlService

#endif //OWLDISCOVER_HTTPCONTROL_H
