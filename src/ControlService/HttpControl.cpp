// jeremie

#include "HttpControl.h"

#include <utility>
#include <boost/core/ignore_unused.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/use_awaitable.hpp>

#include "./OwlCmd.h"

using boost::asio::use_awaitable;
#if defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
# define use_awaitable \
  boost::asio::use_awaitable_t(__FILE__, __LINE__, __PRETTY_FUNCTION__)
#endif

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

    boost::asio::awaitable<std::pair<bool, std::string>> do_session(
            boost::shared_ptr<HttpControl> selfPtr,
            std::string data_send,
            std::string host,
            std::string port,
            std::string target,
            boost::beast::http::verb method = boost::beast::http::verb::post,
            int version = 11) {

        boost::ignore_unused(selfPtr);

        auto executor = co_await boost::asio::this_coro::executor;

        // These objects perform our I/O
        // They use an executor with a default completion token of use_awaitable
        // This makes our code easy, but will use exceptions as the default error handling,
        // i.e. if the connection drops, we might see an exception.
        boost::system::error_code ec_{};
        auto resolver = boost::asio::ip::tcp::resolver(executor);

        auto stream = boost::beast::tcp_stream(executor);

        // Look up the domain name
        auto const results =
                co_await resolver.async_resolve(
                        host, port,
                        boost::asio::redirect_error(use_awaitable, ec_));

        if (ec_) {
            BOOST_LOG_OWL(trace_http_error) << "OwlControlService do_session ec_ " << ec_.what();
            co_return std::pair<bool, std::string>{false, std::string{}};
        }

        // Set the timeout.
        stream.expires_after(std::chrono::seconds(30));

        // Make the connection on the IP address we get from a lookup
        co_await stream.async_connect(results, boost::asio::redirect_error(use_awaitable, ec_));
        if (ec_) {
            BOOST_LOG_OWL(trace_http_error) << "OwlControlService do_session ec_ " << ec_.what();
            co_return std::pair<bool, std::string>{false, std::string{}};
        }

        // Set up an HTTP GET request message
        boost::beast::http::request<boost::beast::http::string_body> req{method, target, version};
        req.set(boost::beast::http::field::host, host);
        req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.body() = data_send;
        req.prepare_payload();

        // Set the timeout.
        stream.expires_after(std::chrono::seconds(30));

        // Send the HTTP request to the remote host
        co_await boost::beast::http::async_write(
                stream, req,
                boost::asio::redirect_error(use_awaitable, ec_));
        if (ec_) {
            BOOST_LOG_OWL(trace_http_error) << "OwlControlService do_session ec_ " << ec_.what();
            co_return std::pair<bool, std::string>{false, std::string{}};
        }

        // This buffer is used for reading and must be persisted
        boost::beast::flat_buffer b;

        // Declare a container to hold the response
        boost::beast::http::response<boost::beast::http::string_body> res;

        // Receive the HTTP response
        co_await boost::beast::http::async_read(
                stream, b, res,
                boost::asio::redirect_error(use_awaitable, ec_));
        if (ec_) {
            if (ec_ == boost::beast::http::error::end_of_stream) {
                // ignore, end_of_stream is not error, it only means peer closed
                // https://github.com/boostorg/beast/issues/1023
                // https://github.com/Socks5Balancer/Socks5BalancerAsio/blob/b58f7d13fc5554f563e0ba168fc479c8c88f3643/src/EmbedWebServer.cpp#L323
            } else {
                BOOST_LOG_OWL(trace_http_error) << "OwlControlService do_session ec_ " << ec_.what();
                co_return std::pair<bool, std::string>{false, std::string{}};
            }
        }

        // Write the message to standard out
        BOOST_LOG_OWL(trace_http) << "OwlControlService do_session res " << res;

        // Gracefully close the socket
        boost::beast::error_code ec;
        stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

        if (ec && ec != boost::beast::errc::not_connected) {
            BOOST_LOG_OWL(trace_http) << "OwlControlService do_session ec " << ec.what();
        }

        // If we get here then the connection is closed gracefully
        boost::ignore_unused(selfPtr);
        co_return std::pair<bool, std::string>{true, std::string{res.body()}};
    }

    void HttpControl::sendCmd(
            const OwlMailDefine::MailControl2HttpControl &data_m,
            const boost::shared_ptr<OwlMailDefine::ControlCmdData> &data) {
        BOOST_ASSERT(data);
        BOOST_LOG_OWL(trace_http) << "HttpControl sendCmd data " << data->ip << " " << static_cast<int>(data->cmd);

        boost::shared_ptr<boost::json::value> V;
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
                // ignore
                return;
            default:
                BOOST_LOG_OWL(trace_http_error) << "HttpControl sendCmd switch (data->cmd)  default";
                return;
        }
        boost::shared_ptr<std::string> S = boost::make_shared<std::string>(boost::json::serialize(*V));

        boost::asio::ip::udp::endpoint remote_endpoint{
                boost::asio::ip::make_address(data->ip),
                boost::lexical_cast<boost::asio::ip::port_type>(config_->config().CommandServiceHttpPort),
        };
        {
            auto mm = boost::make_shared<OwlMailDefine::MailHttpControl2Control::element_type>();
            if (V) {
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
        }

        BOOST_ASSERT(S);
        // send
        send_request([this, self = shared_from_this(),
                             data_m = data_m,
                             data = data,
                             V = V,
                             S,
                             remote_endpoint = remote_endpoint
                     ](std::pair<bool, std::string> r) {
                         BOOST_ASSERT(S);
                         BOOST_ASSERT(V);
                         BOOST_LOG_OWL(trace_http) << "HttpControl send_request resultCallback "
                                                   << " S " << *S;
                         if (r.first) {
                             auto ar = analysis_receive_json(
                                     r.second
                             );
                             BOOST_LOG_OWL(trace_http) << "HttpControl send_request resultCallback analysis_receive_json ar"
                                                       << " first " << ar.first << " second " << ar.second;
                             if (ar.first) {
                                 try {
                                     auto &o = ar.second.as_object();
                                     if (o.contains("cmdId") &&
                                         o.contains("packageId") &&
                                         o.contains("msg") &&
                                         o.contains("result")) {
                                         auto cmdId = get(o, "cmdId", 0);
                                         auto packageId = get(o, "packageId", 0);
                                         auto result = get(o, "result", false);
                                         if (result) {

                                             auto mm = boost::make_shared<OwlMailDefine::MailHttpControl2Control::element_type>();

                                             auto controlCmdData = boost::make_shared<OwlDiscoverState::DiscoverStateItem>(
                                                     remote_endpoint.address().to_string(),
                                                     boost::lexical_cast<int>(remote_endpoint.port())
                                             );

                                             if (V) {
                                                 auto &ooo = V->as_object();
                                                 mm->packageSendInfo = boost::make_shared<OwlDiscoverState::PackageSendInfo>(
                                                         remote_endpoint.address().to_string(),
                                                         packageId,
                                                         cmdId,
                                                         get(ooo, "clientId", 0),
                                                         OwlDiscoverState::PackageSendInfoDirectEnum::in
                                                 );
                                                 mm->port = remote_endpoint.port();
                                             }

                                             auto programVersion = get<std::string>(o, "Version", "");
                                             if (!programVersion.empty()) {
                                                 auto gitRev = get<std::string>(o, "GitRev", "");
                                                 auto buildTime = get<std::string>(o, "BuildTime", "");

                                                 controlCmdData->programVersion = programVersion;
                                                 controlCmdData->gitRev = gitRev;
                                                 controlCmdData->buildTime = buildTime;
                                             }

                                             mm->discoverStateItem = std::move(controlCmdData);
                                             // TODO impl updateOnly
                                             // mm->updateOnly = updateOnly;

                                             mailbox_->sendB2A(std::move(mm));
                                             return;
                                         }
                                     }
                                 }
                                 catch (const std::exception &e) {
                                     BOOST_LOG_OWL(trace_http_error) << "HttpControl sendCmd catch std::exception "
                                                                     << e.what();
                                 }
                                 catch (const std::string &e) {
                                     BOOST_LOG_OWL(trace_http_error) << "HttpControl sendCmd catch std::string " << e;
                                 }
                                 catch (const char *e) {
                                     BOOST_LOG_OWL(trace_http_error) << "HttpControl sendCmd catch char *e " << e;
                                 }
                                 catch (...) {
                                     BOOST_LOG_OWL(trace_http_error)
                                         << "HttpControl sendCmd catch (...)"
                                         << "\n" << boost::current_exception_diagnostic_information();
                                 }
                             }
                         }
                         BOOST_LOG_OWL(warning) << "HttpControl send_request resultCallback warning"
                                                << " S " << *S;
                         auto mm = boost::make_shared<OwlMailDefine::MailHttpControl2Control::element_type>();
                         mm->runner = data_m->callbackRunner;
                         mailbox_->sendB2A(std::move(mm));
                     },
                     data_m,
                     *S,
                     remote_endpoint.address().to_string(),
                     boost::lexical_cast<std::string>(remote_endpoint.port()),
                     "/cmd",
                     boost::beast::http::verb::post);
    }


    void HttpControl::send_request(
            std::function<void(std::pair<bool, std::string>)> &&resultCallback,
            const OwlMailDefine::MailControl2HttpControl &data,
            const std::string &data_send,
            const std::string &host,
            const std::string &port,
            const std::string &target,
            const boost::beast::http::verb &method,
            int version) {
        BOOST_ASSERT(resultCallback);

        boost::asio::co_spawn(ioc_, [=, this, self = shared_from_this()]() {
            return do_session(
                    self,
                    data_send,
                    host,
                    port,
                    target,
                    method,
                    version
            );
        }, [this, self = shared_from_this(),
                                      data = data,
                                      data_send = data_send,
                                      host = host,
                                      port = port,
                                      target = target,
                                      resultCallback = std::move(resultCallback)
                              ](
                std::exception_ptr e, std::pair<bool, std::string> r
        ) {
            if (!e) {
                if (r.first) {
                    BOOST_LOG_OWL(trace_http) << "HttpControl send_request() ok "
                                              << " host " << host
                                              << " port " << port
                                              << " target " << target
                                              << " data_send " << data_send;
                    if (resultCallback) {
                        resultCallback(std::move(r));
                    }
                    return;
                } else {
                    BOOST_LOG_OWL(trace_http_error) << "HttpControl send_request() error "
                                                    << " host " << host
                                                    << " port " << port
                                                    << " target " << target
                                                    << " data_send " << data_send;
                    if (resultCallback) {
                        resultCallback(std::move(r));
                    }
                    return;
                }
            } else {
                std::string what;
                // https://stackoverflow.com/questions/14232814/how-do-i-make-a-call-to-what-on-stdexception-ptr
                try { std::rethrow_exception(std::move(e)); }
                catch (const std::exception &e) {
                    BOOST_LOG_OWL(trace_http_error) << "HttpControl co_spawn catch std::exception "
                                                    << e.what();
                    what = e.what();
                }
                catch (const std::string &e) {
                    BOOST_LOG_OWL(trace_http_error) << "HttpControl co_spawn catch std::string " << e;
                    what = e;
                }
                catch (const char *e) {
                    BOOST_LOG_OWL(trace_http_error) << "HttpControl co_spawn catch char *e " << e;
                    what = std::string{e};
                }
                catch (...) {
                    BOOST_LOG_OWL(trace_http_error) << "HttpControl co_spawn catch (...)"
                                                    << "\n" << boost::current_exception_diagnostic_information();
                    what = boost::current_exception_diagnostic_information();
                }
                BOOST_LOG_OWL(trace_http_error) << "HttpControl send_request "
                                                << what
                                                << "\n host " << host
                                                << " port " << port
                                                << " target " << target
                                                << " data_send " << data_send;

                if (resultCallback) {
                    resultCallback(std::pair<bool, std::string>{false, {}});
                }
                return;
            }

        });

    }

    std::pair<bool, boost::json::value> HttpControl::analysis_receive_json(const std::string &data_) {
        auto data = std::string_view{data_};
        BOOST_LOG_OWL(trace_multicast) << "HttpControl analysis_receive_json() data " << data;
        boost::system::error_code ecc;
        boost::json::value json_v = boost::json::parse(
                data,
                ecc,
                {},
                json_parse_options_
        );
        if (ecc) {
            BOOST_LOG_OWL(warning) << "HttpControl analysis_receive_json() invalid package " << ecc.what();
            // ignore
            return std::pair<bool, boost::json::value>{false, {}};
        }

        BOOST_LOG_OWL(trace_multicast) << "HttpControl analysis_receive_json() " << boost::json::serialize(json_v);

        auto &o = json_v.as_object();
//        if (o.contains("MultiCast")) {
//            auto multiCastResponse = get(o, "MultiCast", std::string{});
//            if (multiCastResponse == "Response") {
//
//                auto m = boost::make_shared<OwlMailDefine::MailHttpControl2Control::element_type>();
//
//                auto controlCmdData = boost::make_shared<OwlDiscoverState::DiscoverStateItem>(
//                        receiver_endpoint->address().to_string(),
//                        static_cast<int>(receiver_endpoint->port())
//                );
//
//                m->discoverStateItem = std::move(controlCmdData);
//
//                mailbox_->sendB2A(std::move(m));
//                return;
//            }
//        }
        if (o.contains("cmdId") &&
            o.contains("packageId") &&
            o.contains("msg") &&
            o.contains("result")) {
            auto cmdId = get(o, "cmdId", 0);
            auto result = get(o, "result", false);
            if (result) {
                return std::pair<bool, boost::json::value>{true, json_v};
            }
        }

        // ignore
        return std::pair<bool, boost::json::value>{false, {}};
    }

    void HttpControl::receiveMailHttp(OwlMailDefine::MailControl2HttpControl &&data) {
        boost::asio::dispatch(ioc_, [
                this, self = shared_from_this(), data]() {

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
                        sendCmd(data, data->controlCmdData);
                        break;
                    default:
                        break;
                }
            } else if (data->httpRequestInfo) {
                send_request([this, self = shared_from_this(), data = data](std::pair<bool, std::string> r) {
                                 auto mm = boost::make_shared<OwlMailDefine::MailHttpControl2Control::element_type>();
                                 mm->runner = data->callbackRunner;
                                 mm->httpResponseData = boost::make_shared<std::string>(r.second);
                                 mailbox_->sendB2A(std::move(mm));
                             },
                             data,
                             data->httpRequestInfo->data_send,
                             data->httpRequestInfo->host,
                             data->httpRequestInfo->port,
                             data->httpRequestInfo->target,
                             data->httpRequestInfo->method);
            }

        });
    }

} // OwlControlService
