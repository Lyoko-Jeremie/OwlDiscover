// jeremie

#ifndef OWLACCESSTERMINAL_MULTICAST_H
#define OWLACCESSTERMINAL_MULTICAST_H

#include "../MemoryBoost.h"
#include <chrono>
#include <utility>
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include "../OwlLog/OwlLog.h"
#include "../ConfigLoader/ConfigLoader.h"
#include "./ControlMulticastMail.h"
#include "../ImGuiService/ControlImGuiMailBox.h"

namespace OwlMultiCast {

    enum {
        UDP_Package_Max_Size = (1024 * 1024 * 6), // 6M
        JSON_Package_Max_Size = (1024 * 1024 * 6), // 6M
    };

    class MultiCast : public boost::enable_shared_from_this<MultiCast> {
    public:
        MultiCast(
                boost::asio::io_context &ioc,
                boost::shared_ptr<OwlConfigLoader::ConfigLoader> config,
                OwlMailDefine::ControlMulticastMailbox &&mailbox_mc,
                OwlMailDefine::ControlImGuiMailBox &&mailbox_ig
        ) : ioc_(ioc),
            config_(std::move(config)),
            mailbox_mc_(mailbox_mc),
            mailbox_ig_(mailbox_ig),
            sender_socket_(ioc_),
            multicast_listen_socket_(ioc_),
            json_storage_(std::make_unique<decltype(json_storage_)::element_type>(JSON_Package_Max_Size, 0)),
            json_storage_resource_(std::make_unique<decltype(json_storage_resource_)::element_type>
                                           (json_storage_->data(), json_storage_->size())) {

            const auto &c = config_->config();
            multicast_address_.from_string(c.multicast_address);
            multicast_listen_address_.from_string(c.multicast_listen_address);
            multicast_port_ = c.multicast_port;

            sender_address_.from_string(c.sender_address);
            sender_port_ = c.sender_port;
            multicast_listen_endpoint_ = decltype(multicast_listen_endpoint_){multicast_address_, multicast_port_};
            multicast_listen_socket_.open(multicast_listen_endpoint_.protocol());
            multicast_listen_socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
            multicast_listen_socket_.bind(
                    decltype(multicast_listen_endpoint_){multicast_listen_address_, multicast_port_});

            target_multicast_endpoint_ = decltype(target_multicast_endpoint_){multicast_address_, multicast_port_};

            sender_endpoint_ = decltype(sender_endpoint_){sender_address_, sender_port_};
            sender_socket_.open(sender_endpoint_.protocol());
//            sender_socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
            sender_socket_.bind(sender_endpoint_);


            // Join the multicast group.
            multicast_listen_socket_.set_option(boost::asio::ip::multicast::join_group(multicast_address_));

            json_parse_options_.allow_comments = true;
            json_parse_options_.allow_trailing_commas = true;
            json_parse_options_.max_depth = 5;

            // Query all client
            static_send_message_ = R"({"MultiCast":"Query","T":"1"})";


            mailbox_mc_->receiveA2B([this](OwlMailDefine::MailControl2Multicast &&data) {
                BOOST_LOG_OWL(trace) << "MultiCast mailbox_mc_->receiveA2B";
                mailControl(std::move(data));
            });
            mailbox_ig_->receiveB2A([this](OwlMailDefine::MailImGui2Control &&data) {
                if (data->runner) {
                    data->runner(data);
                }
            });

        }

        ~MultiCast() {
            mailbox_mc_->receiveA2B(nullptr);
            mailbox_ig_->receiveB2A(nullptr);
        }

        void start() {
            BOOST_LOG_OWL(trace_multicast) << "MultiCast start";
            do_receive();
        }

    private:

        void mailControl(OwlMailDefine::MailControl2Multicast &&data);

    private:
        boost::asio::io_context &ioc_;
        boost::shared_ptr<OwlConfigLoader::ConfigLoader> config_;
        OwlMailDefine::ControlMulticastMailbox mailbox_mc_;
        OwlMailDefine::ControlImGuiMailBox mailbox_ig_;

        boost::asio::ip::address multicast_address_{boost::asio::ip::make_address("239.255.0.1")};
        boost::asio::ip::port_type multicast_port_ = 30003;


        //
        boost::asio::ip::udp::endpoint target_multicast_endpoint_{multicast_address_, multicast_port_};

        // send package
        boost::asio::ip::address sender_address_{boost::asio::ip::make_address("0.0.0.0")};
        boost::asio::ip::port_type sender_port_ = 0;
        boost::asio::ip::udp::endpoint sender_endpoint_{sender_address_, sender_port_};
        boost::asio::ip::udp::socket sender_socket_;
        std::array<char, UDP_Package_Max_Size> back_data_{};

        // listen package
        boost::asio::ip::udp::endpoint multicast_listen_endpoint_{multicast_address_, multicast_port_};
        boost::asio::ip::address multicast_listen_address_{boost::asio::ip::make_address("0.0.0.0")};
        boost::asio::ip::udp::socket multicast_listen_socket_;

        // where the package come
        std::array<char, UDP_Package_Max_Size> receive_data_{};

        boost::json::parse_options json_parse_options_;
        std::unique_ptr<std::vector<unsigned char>> json_storage_;
        std::unique_ptr<boost::json::static_resource> json_storage_resource_;

        std::string static_send_message_;

    public:

    private:

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

        // ======================================================================================================

        void do_receive_json(
                std::size_t length,
                std::array<char, UDP_Package_Max_Size> &data_,
                boost::shared_ptr<boost::asio::ip::udp::endpoint> receiver_endpoint);

        void do_receive();

        // ======================================================================================================

        void do_send();

        void do_send_back();


    };

} // OwlMultiCast

#endif //OWLACCESSTERMINAL_MULTICAST_H
