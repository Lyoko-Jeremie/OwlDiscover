// jeremie

#include "HttpControl.h"

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

using boost::asio::use_awaitable;
#if defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
# define use_awaitable \
  boost::asio::use_awaitable_t(__FILE__, __LINE__, __PRETTY_FUNCTION__)
#endif

namespace OwlControlService {

    boost::asio::awaitable<bool> do_session(
            boost::shared_ptr<HttpControl> selfPtr,
            std::string data_send,
            std::string host,
            std::string port,
            std::string target,
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
            BOOST_LOG_OWL(trace) << "OwlControlService do_session ec_ " << ec_;
            co_return false;
        }

        // Set the timeout.
        stream.expires_after(std::chrono::seconds(30));

        // Make the connection on the IP address we get from a lookup
        co_await stream.async_connect(results, boost::asio::redirect_error(use_awaitable, ec_));
        if (ec_) {
            BOOST_LOG_OWL(trace) << "OwlControlService do_session ec_ " << ec_;
            co_return false;
        }

        // Set up an HTTP GET request message
        boost::beast::http::request<boost::beast::http::string_body> req{
                boost::beast::http::verb::post,
                target, version};
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
            BOOST_LOG_OWL(trace) << "OwlControlService do_session ec_ " << ec_;
            co_return false;
        }

        // This buffer is used for reading and must be persisted
        boost::beast::flat_buffer b;

        // Declare a container to hold the response
        boost::beast::http::response<boost::beast::http::dynamic_body> res;

        // Receive the HTTP response
        co_await boost::beast::http::async_read(
                stream, b, res,
                boost::asio::redirect_error(use_awaitable, ec_));
        if (ec_) {
            BOOST_LOG_OWL(trace) << "OwlControlService do_session ec_ " << ec_;
            co_return false;
        }

        // Write the message to standard out
        BOOST_LOG_OWL(trace) << "OwlControlService do_session res " << res;

        // Gracefully close the socket
        boost::beast::error_code ec;
        stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

        if (ec && ec != boost::beast::errc::not_connected) {
            BOOST_LOG_OWL(trace) << "OwlControlService do_session ec " << ec;
        }

        // If we get here then the connection is closed gracefully
        boost::ignore_unused(selfPtr);
        co_return true;
    }

    void HttpControl::sendCmd(
            const OwlMailDefine::MailControl2HttpControl& data_m,
            const boost::shared_ptr<OwlMailDefine::ControlCmdData> &data) {
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
                // ignore
                return;
            default:
                BOOST_LOG_OWL(error) << "UdpControl sendCmd switch (data->cmd)  default";
                return;
        }

        boost::asio::ip::udp::endpoint remote_endpoint{
                boost::asio::ip::make_address(data->ip),
                boost::lexical_cast<boost::asio::ip::port_type>(config_->config().CommandServiceUdpPort),
        };

        BOOST_ASSERT(S);
        // send
        send_request(data_m,
                     *S,
                     data->ip,
                     boost::lexical_cast<std::string>(config_->config().CommandServiceHttpPort),
                     "/cmd");
    }


    void HttpControl::send_request(
            const OwlMailDefine::MailControl2HttpControl& data,
            const std::string &data_send,
            const std::string &host,
            const std::string &port,
            const std::string &target,
            int version) {

        boost::asio::co_spawn(ioc_, [=, this, self = shared_from_this()]() {
            return do_session(
                    self,
                    data_send,
                    host,
                    port,
                    target,
                    version
            );
        }, [=, this, self = shared_from_this()](std::exception_ptr e, bool r) {
            auto m = boost::make_shared<OwlMailDefine::MailHttpControl2Control::element_type>();
            m->runner = data->callbackRunner;
            if (!e) {
                if (r) {
                    BOOST_LOG_OWL(trace) << "HttpControl send_request() ok "
                                         << " host " << host
                                         << " port " << port
                                         << " target " << target
                                         << " data_send " << data_send;
                    mailbox_->sendB2A(std::move(m));
                    return;
                } else {
                    BOOST_LOG_OWL(error) << "HttpControl send_request() error "
                                         << " host " << host
                                         << " port " << port
                                         << " target " << target
                                         << " data_send " << data_send;
                    mailbox_->sendB2A(std::move(m));
                    return;
                }
            } else {
                std::string what;
                // https://stackoverflow.com/questions/14232814/how-do-i-make-a-call-to-what-on-stdexception-ptr
                try { std::rethrow_exception(std::move(e)); }
                catch (const std::exception &e) {
                    BOOST_LOG_OWL(error) << "HttpControl co_spawn catch std::exception "
                                         << e.what();
                    what = e.what();
                }
                catch (const std::string &e) {
                    BOOST_LOG_OWL(error) << "HttpControl co_spawn catch std::string " << e;
                    what = e;
                }
                catch (const char *e) {
                    BOOST_LOG_OWL(error) << "HttpControl co_spawn catch char *e " << e;
                    what = std::string{e};
                }
                catch (...) {
                    BOOST_LOG_OWL(error) << "HttpControl co_spawn catch (...)"
                                         << "\n" << boost::current_exception_diagnostic_information();
                    what = boost::current_exception_diagnostic_information();
                }
                BOOST_LOG_OWL(error) << "HttpControl send_request "
                                     << what
                                     << "\n host " << host
                                     << " port " << port
                                     << " target " << target
                                     << " data_send " << data_send;

                mailbox_->sendB2A(std::move(m));
                return;
            }

        });

    }

} // OwlControlService
