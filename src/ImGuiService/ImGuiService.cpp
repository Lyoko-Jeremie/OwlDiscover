// jeremie

#include "ImGuiService.h"
#include "../OwlLog/OwlLog.h"

#include <sstream>
#include <utility>
#include <vector>
#include <list>
#include <array>
#include <boost/lexical_cast.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/json.hpp>
#include <boost/algorithm/string/find.hpp>


#include "./ImGuiServiceImpl.h"


namespace OwlImGuiService {

    ImGuiService::ImGuiService(
            boost::asio::io_context &ioc,
            boost::shared_ptr<OwlConfigLoader::ConfigLoader> config,
            OwlMailDefine::ControlImGuiMailBox &&mailbox_ig,
            OwlMailDefine::ControlMulticastMailbox &&mailbox_mc,
            OwlMailDefine::ControlUdpMailBox &&mailbox_udp,
            OwlMailDefine::ControlHttpMailBox &&mailbox_http
    ) : ioc_(ioc),
        config_(std::move(config)),
        mailbox_mc_(mailbox_mc),
        mailbox_ig_(mailbox_ig),
        mailbox_udp_(mailbox_udp),
        mailbox_http_(mailbox_http) {

        mailbox_ig_->receiveA2B([this](OwlMailDefine::MailControl2ImGui &&data) {
            boost::asio::dispatch(ioc_, [this, data, self = shared_from_this()]() {
                auto m = boost::make_shared<OwlMailDefine::MailImGui2Control::element_type>();
                m->runner = data->callbackRunner;

                BOOST_ASSERT(impl);
                if (data->state) {
                    impl->new_state(data->state);
                } else {
                    BOOST_LOG_OWL(trace_gui) << "ImGuiService mailbox_ig_->receiveA2B (data->state) else";
                }

                mailbox_ig_->sendB2A(std::move(m));
            });
        });

        mailbox_mc_->receiveB2A([this](OwlMailDefine::MailMulticast2Control &&data) {
            if (data->runner) {
                data->runner(data);
            }
        });

        mailbox_udp_->receiveB2A([this](OwlMailDefine::MailUdpControl2Control &&data) {
            if (data->runner) {
                data->runner(data);
            } else if (data->discoverStateItem) {
                if (data->updateOnly) {
                    auto &item = data->discoverStateItem;
                    impl->update_state(item);
                } else {
                    auto &item = data->discoverStateItem;
                    impl->new_state(item);
                }
            }
            if (data->packageSendInfo) {
                impl->update_package_send_info(data->packageSendInfo);
            }
        });

        mailbox_http_->receiveB2A([this](OwlMailDefine::MailHttpControl2Control &&data) {
            if (data->discoverStateItem) {
                if (data->updateOnly) {
                    auto &item = data->discoverStateItem;
                    impl->update_state(item);
                } else {
                    auto &item = data->discoverStateItem;
                    impl->new_state(item);
                }
                if (data->runner) {
                    data->runner(data);
                }
            }
            if (data->packageSendInfo) {
                impl->update_package_send_info(data->packageSendInfo);
            }
        });

        json_parse_options_.allow_comments = true;
        json_parse_options_.allow_trailing_commas = true;
        json_parse_options_.max_depth = 5;

    }

    void ImGuiService::start() {
        impl = boost::make_shared<OwlImGuiServiceImpl::ImGuiServiceImpl>(ioc_, config_, weak_from_this());
        impl->start();
    }

    void ImGuiService::clear() {
        impl->clear();
    }

    void ImGuiService::sendBroadcast() {
        boost::asio::post(ioc_, [this, self = shared_from_this()]() {
            auto m = boost::make_shared<OwlMailDefine::MailControl2UdpControl::element_type>();

            m->controlCmdData = boost::make_shared<OwlMailDefine::ControlCmdData>();
            m->controlCmdData->cmd = OwlMailDefine::ControlCmd::broadcast;
            m->callbackRunner = [](OwlMailDefine::MailUdpControl2Control &&data) {
                // ignore
                boost::ignore_unused(data);
            };

            mailbox_udp_->sendA2B(std::move(m));
        });
    }

    void ImGuiService::sendQuery() {
        boost::asio::post(ioc_, [this, self = shared_from_this()]() {
            auto m = boost::make_shared<OwlMailDefine::MailControl2Multicast::element_type>();

            m->cmd = OwlMailDefine::MulticastCmd::query;
            m->callbackRunner = [](OwlMailDefine::MailMulticast2Control &&data) {
                // ignore
                boost::ignore_unused(data);
            };

            mailbox_mc_->sendA2B(std::move(m));
        });
    }

    void ImGuiService::sendCmdUdp(boost::shared_ptr<OwlMailDefine::ControlCmdData> data) {
        auto m = boost::make_shared<OwlMailDefine::MailControl2UdpControl::element_type>();

        BOOST_ASSERT(data);
        m->controlCmdData = std::move(data);

        m->callbackRunner = [](OwlMailDefine::MailUdpControl2Control &&d) {
            boost::ignore_unused(d);
            // ignore
        };

        mailbox_udp_->sendA2B(std::move(m));
    }

    void ImGuiService::sendCmdHttp(boost::shared_ptr<OwlMailDefine::ControlCmdData> data) {
        auto m = boost::make_shared<OwlMailDefine::MailControl2HttpControl::element_type>();

        auto a = boost::make_shared<OwlDiscoverState::DiscoverStateItem>(
                data->ip,
                0
        );

        BOOST_ASSERT(data);
        m->controlCmdData = std::move(data);

        m->callbackRunner = [this, self = shared_from_this(), a](OwlMailDefine::MailHttpControl2Control &&d) {
            boost::ignore_unused(d);
//            impl->new_state(a);
        };

        mailbox_http_->sendA2B(std::move(m));
    };

    void ImGuiService::sendCmdHttpReadOTA(std::string ip) {
        auto m = boost::make_shared<OwlMailDefine::MailControl2HttpControl::element_type>();

        auto a = boost::make_shared<OwlMailDefine::HttpRequestInfo>(
                ip,
                "8080",
                "/VERSION",
                "",
                boost::beast::http::verb::get
        );

        m->httpRequestInfo = std::move(a);

        m->callbackRunner = [this, self = shared_from_this(), ip = std::move(ip)](
                OwlMailDefine::MailHttpControl2Control &&d) {
            BOOST_ASSERT(d->httpResponseData);

            BOOST_LOG_OWL(warning) << "ImGuiService sendCmdHttpReadOTA() d->httpResponseData "
                                   << *(d->httpResponseData);
            if (d->httpResponseData && !d->httpResponseData->empty()) {
                analysisOtaReturn(*d->httpResponseData, ip);
            }
        };

        mailbox_http_->sendA2B(std::move(m));
    }

    void ImGuiService::analysisOtaReturn(const std::string &httpResponseData, std::string ip) {

        boost::system::error_code ec;
        boost::json::value json_v = boost::json::parse(
                httpResponseData,
                ec,
                {},
                json_parse_options_
        );
        if (ec) {
            BOOST_LOG_OWL(warning) << "ImGuiService analysisOtaReturn() invalid package " << ec.what();
            return;
        }

        try {
            auto rr = boost::json::try_value_to<std::string>(json_v.as_object().at("version"));
            if (rr.has_value()) {
                auto version = rr.value();
                if (impl && !version.empty()) {
                    impl->update_ota(ip, version);
                    return;
                }
            }
            BOOST_LOG_OWL(warning) << "ImGuiService analysisOtaReturn() version value , ip" << ip;
        } catch (...) {
            BOOST_LOG_OWL(warning) << "ImGuiService analysisOtaReturn() invalid json, catch "
                                   << boost::current_exception_diagnostic_information();
        }

    }

    void ImGuiService::listSubnetIp(std::function<void(std::list<std::string>)> callback) {
        // https://stackoverflow.com/questions/2674314/get-local-ip-address-using-boost-asio
        auto resolver = boost::make_shared<boost::asio::ip::tcp::resolver>(ioc_);
        boost::system::error_code ec;
        auto host_name = boost::make_shared<std::string>(boost::asio::ip::host_name(ec));
        if (ec) {
            BOOST_LOG_OWL(error) << "ImGuiService listSubnetIp host_name ec " << ec.what();
            return;
        }
        resolver->async_resolve(*host_name, "", [
                this, self = shared_from_this(), resolver, host_name, callback
        ](const boost::system::error_code &ec, const boost::asio::ip::tcp::resolver::results_type &results) {
            if (ec) {
                BOOST_LOG_OWL(error) << "ImGuiService listSubnetIp resolver.async_resolve ec " << ec << " what "
                                     << ec.what();
                if (callback) {
                    callback({});
                }
                return;
            }
            std::vector<std::string> addrList;
            for (auto it = results.begin(); it != results.end(); ++it) {
                auto ep = it->endpoint();
                if (ep.protocol() == boost::asio::ip::tcp::v4()) {
                    auto addr = ep.address();
                    if (addr.is_v4()) {
                        boost::system::error_code ecc;
                        auto as = addr.to_string(ecc);
                        if (ecc) {
                            BOOST_LOG_OWL(error) << "ImGuiService listSubnetIp address to_string ec " << ec.what();
                            continue;
                        }
                        addrList.emplace_back(as);
                    }
                }
            }
            if (addrList.empty()) {
                BOOST_LOG_OWL(error) << "ImGuiService listSubnetIp (addrList.empty()) ";
                if (callback) {
                    callback({});
                }
            }
            std::string ss{"."};
            std::list<std::string> ipList;
            for (const auto &a: addrList) {
                BOOST_LOG_OWL(trace) << "addrList a " << a;
//                {
//                    auto n = std::find_end(a.begin(), a.end(), ss.begin(), ss.end());
//                    n.base();
//                }
                {
                    auto n = boost::find_last(a, ss);
                    std::string prefix{a.begin(), n.begin() + 1};
                    for (int i = 1; i != 254; ++i) {
                        ipList.push_back(prefix + boost::lexical_cast<std::string>(i));
                    }
                }
            }
            if (callback) {
                callback(ipList);
            }
        });
    }

    void ImGuiService::scanSubnet() {
        listSubnetIp([this, self = shared_from_this()](const auto &ipList) {
            for (const auto &ip: ipList) {
//                BOOST_LOG_OWL(trace) << "send scan to " << ip;
                auto m = boost::make_shared<OwlMailDefine::MailControl2HttpControl::element_type>();

                auto a = boost::make_shared<OwlMailDefine::HttpRequestInfo>(
                        ip,
                        boost::lexical_cast<std::string>(8080),
                        R"(/VERSION)",
                        "",
                        boost::beast::http::verb::get
                );

                m->httpRequestInfo = std::move(a);

                m->callbackRunner = [
                        this, self = shared_from_this(), ip = std::move(ip)
                ](OwlMailDefine::MailHttpControl2Control &&d) {
                    BOOST_ASSERT(d->httpResponseData);
                    if (d->httpResponseData && !d->httpResponseData->empty()) {
                        analysisOtaReturn(*d->httpResponseData, ip);
                    }
                };

                mailbox_http_->sendA2B(std::move(m));
            }
        });
    }

    void ImGuiService::scanSubnetPingUdp() {
        listSubnetIp([this, self = shared_from_this()](const auto &ipList) {
            for (const auto &ip: ipList) {
//                BOOST_LOG_OWL(trace) << "send scan to " << ip;
                auto data = boost::make_shared<OwlMailDefine::ControlCmdData>();
                data->ip = ip;
                data->cmd = OwlMailDefine::ControlCmd::ping;
                sendCmdUdp(std::move(data));
            }
        });
    }

    void ImGuiService::scanSubnetPingHttp() {
        listSubnetIp([this, self = shared_from_this()](const auto &ipList) {
            for (const auto &ip: ipList) {
//                BOOST_LOG_OWL(trace) << "send scan to " << ip;
                auto data = boost::make_shared<OwlMailDefine::ControlCmdData>();
                data->ip = ip;
                data->cmd = OwlMailDefine::ControlCmd::ping;
                sendCmdHttp(std::move(data));
            }
        });
    }


} // OwlImGuiService
