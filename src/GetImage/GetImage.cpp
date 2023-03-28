// jeremie

#include "GetImage.h"

#include <string>


namespace OwlGetImage {
    // https://www.boost.org/doc/libs/1_81_0/libs/beast/example/http/client/async/http_client_async.cpp


    // Performs an HTTP GET and prints the response
    class GetImageSession : public std::enable_shared_from_this<GetImageSession> {
        boost::asio::ip::tcp::resolver resolver_;
        boost::beast::tcp_stream stream_;
        // ( buffer_ Must persist between reads)
        // [ NOTE: the `flat_buffer` default size limit is 1MB ]
        boost::beast::flat_buffer buffer_;
        boost::beast::http::request<boost::beast::http::empty_body> req_;
        // https://github.com/boostorg/beast/issues/1637
        boost::beast::http::response<boost::beast::http::vector_body<unsigned char>> res_;

        CallbackFunctionType callback_;

        long timeoutMs_ = 30 * 1000;
    public:
        // Objects are constructed with a strand to
        // ensure that handlers do not execute concurrently.
        explicit
        GetImageSession(boost::asio::io_context &ioc,
                        CallbackFunctionType &&callback,
                        long timeoutMs = 30 * 1000)
                : resolver_(boost::asio::make_strand(ioc)),
                  stream_(boost::asio::make_strand(ioc)),
                  callback_(callback),
                  timeoutMs_(timeoutMs) {
        }

        // Start the asynchronous operation
        void
        run(
                const std::string &host,
                const std::string &port,
                const std::string &target,
                int version) {
            // Set up an HTTP GET request message
            req_.version(version);
            req_.method(boost::beast::http::verb::get);
            req_.target(target);
            req_.set(boost::beast::http::field::host, host);
            req_.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // Look up the domain name
            resolver_.async_resolve(
                    host,
                    port,
                    boost::beast::bind_front_handler(
                            &GetImageSession::on_resolve,
                            shared_from_this()));
        }

    private:

        // Report a failure
        void
        fail(boost::beast::error_code ec, char const *what) {
            BOOST_LOG_TRIVIAL(error) << what << ": " << ec.message();
            callback_(ec, false, {});
        }


        void
        on_resolve(
                boost::beast::error_code ec,
                boost::asio::ip::tcp::resolver::results_type results) {
            if (ec)
                return fail(ec, "resolve");

            // Set a timeout on the operation
            stream_.expires_after(std::chrono::milliseconds(timeoutMs_));

            // Make the connection on the IP address we get from a lookup
            stream_.async_connect(
                    results,
                    boost::beast::bind_front_handler(
                            &GetImageSession::on_connect,
                            shared_from_this()));
        }

        void
        on_connect(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type::endpoint_type) {
            if (ec)
                return fail(ec, "connect");

            // Set a timeout on the operation
            stream_.expires_after(std::chrono::seconds(timeoutMs_));

            // Send the HTTP request to the remote host
            boost::beast::http::async_write(stream_, req_,
                                            boost::beast::bind_front_handler(
                                                    &GetImageSession::on_write,
                                                    shared_from_this()));
        }

        void
        on_write(
                boost::beast::error_code ec,
                std::size_t bytes_transferred) {
            boost::ignore_unused(bytes_transferred);

            if (ec)
                return fail(ec, "write");

            // Receive the HTTP response
            boost::beast::http::async_read(stream_, buffer_, res_,
                                           boost::beast::bind_front_handler(
                                                   &GetImageSession::on_read,
                                                   shared_from_this()));
        }

        void
        on_read(
                boost::beast::error_code ec,
                std::size_t bytes_transferred) {
            boost::ignore_unused(bytes_transferred);

            if (ec)
                return fail(ec, "read");

            stream_.expires_never();

            if (res_.result() == boost::beast::http::status::ok) {

//                // TODO this is debug
//                // Write the message to standard out
//                BOOST_LOG_TRIVIAL(trace)
//                    << std::string{reinterpret_cast<char *>(res_.body().data()), res_.body().size()};
//                BOOST_LOG_TRIVIAL(trace) << "[[[[[[all end.";

                if (res_.at(boost::beast::http::field::content_type) == "image/jpeg"
                    && res_.at("X-image-format") == "jpg") {

                    // this is out image
                    // https://github.com/boostorg/beast/issues/1637
                    auto img = cv::imdecode(
                            cv::InputArray{res_.body().data(), static_cast<int>(res_.body().size())},
                            cv::ImreadModes::IMREAD_UNCHANGED
                    );
                    if (callback_) {
                        callback_({}, true, img);
                    }

                } else {
                    BOOST_LOG_TRIVIAL(error) << "bad response. :"
                                             << "\ncontent_type:" << res_.at(boost::beast::http::field::content_type)
                                             << "\nX-image-format:" << res_.at("X-image-format");
                    if (callback_) {
                        callback_({}, false, {});
                    }
                }

            } else {
                BOOST_LOG_TRIVIAL(error)
                    << "response not ok . " << " res_.result(): " << res_.result() << "\n"
                    << std::string{reinterpret_cast<char *>(res_.body().data()), res_.body().size()};
                if (callback_) {
                    callback_({}, false, {});
                }
            }


            // Gracefully close the socket
            stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

            // not_connected happens sometimes so don't bother reporting it.
            if (ec && ec != boost::beast::errc::not_connected)
                return fail(ec, "shutdown");

            // If we get here then the connection is closed gracefully
        }
    };

    void GetImage::test(const std::string &host, const std::string &port, const std::string &target, int version) {
        std::make_shared<GetImageSession>(ioc_, [](boost::beast::error_code ec, bool ok, cv::Mat img) {
            boost::ignore_unused(ec);
            boost::ignore_unused(ok);
            boost::ignore_unused(img);
        }, timeoutMs)
                ->run(host, port, target, version);
    }

    std::shared_ptr<GetImageSession>
    GetImage::get(const std::string &host,
                  const std::string &port,
                  const std::string &target,
                  int version,
                  CallbackFunctionType &&callback) {
        auto p = std::make_shared<GetImageSession>(ioc_, std::move(callback), timeoutMs);
        p->run(host, port, target, version);
        return p;
    }

} // OwlGetImage