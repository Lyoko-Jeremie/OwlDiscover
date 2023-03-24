#include <vector>
#include <boost/assert.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/core/ignore_unused.hpp>

#include <SDL_main.h>

#include "./OwlLog/OwlLog.h"
#include "./MemoryBoost.h"
#include <vector>
#include <utility>
#include <functional>
#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/thread.hpp>
#include <boost/program_options.hpp>

#include "./ImGuiService/ImGuiService.h"

struct ThreadCallee {
    boost::asio::io_context &ioc;
    boost::thread_group &tg;
    std::string thisThreadName;

    int operator()() {
        try {
            OwlLog::threadName = thisThreadName;
            BOOST_LOG_OWL(info) << ">>>" << OwlLog::threadName << "<<< running thread <<< <<<";
            // use work to keep ioc run
            auto work_guard_ = boost::asio::make_work_guard(ioc);
            ioc.run();
            boost::ignore_unused(work_guard_);
            BOOST_LOG_OWL(warning) << "ThreadCallee ioc exit. thread: " << OwlLog::threadName;
        } catch (int e) {
            tg.interrupt_all();
            BOOST_LOG_OWL(error) << "ThreadCallee catch (int) exception: " << e;
            return -1;
        } catch (boost::exception &e) {
            tg.interrupt_all();
            // https://www.boost.org/doc/libs/1_81_0/libs/exception/doc/diagnostic_information.html
            BOOST_LOG_OWL(error) << "ThreadCallee catch std::exception :"
                                 << "\n diag: " << boost::diagnostic_information(e)
                                 << "\n what: " << dynamic_cast<std::exception const &>(e).what();
            return -1;
        } catch (const std::exception &e) {
            tg.interrupt_all();
            BOOST_LOG_OWL(error) << "ThreadCallee catch std::exception: " << e.what();
            return -1;
        } catch (...) {
            tg.interrupt_all();
            // https://www.boost.org/doc/libs/1_81_0/libs/exception/doc/current_exception_diagnostic_information.html
            BOOST_LOG_OWL(error) << "ThreadCallee catch (...) exception"
                                 << "\n current_exception_diagnostic_information : "
                                 << boost::current_exception_diagnostic_information();
            return -1;
        }
        return 0;
    }
};

std::function<void()> safe_exit_signal;

void OwlImGuiService::safe_exit() {
    if (safe_exit_signal) {
        safe_exit_signal();
    }
}

#include <boost/log/trivial.hpp>
int main(int, char **) {

    OwlLog::threadName = "main";
    OwlLog::init_logging();
//    BOOST_LOG_TRIVIAL(info) << "Hello, World!";
    BOOST_LOG_OWL(info) << "Hello, World!";

    boost::asio::io_context ioc_im_gui_service;
    auto imGuiService = boost::make_shared<OwlImGuiService::ImGuiService>(
            ioc_im_gui_service
    );
    imGuiService->start();


    boost::asio::io_context ioc_keyboard;
    boost::asio::signal_set sig(ioc_keyboard);
    sig.add(SIGINT);
    sig.add(SIGTERM);
    sig.async_wait([&](const boost::system::error_code error, int signum) {
        if (error) {
            BOOST_LOG_OWL(error) << "got signal error: " << error.what() << " signum " << signum;
            return;
        }
        BOOST_LOG_OWL(error) << "got signal: " << signum;
        switch (signum) {
            case SIGINT:
            case SIGTERM: {
                // stop all service on there
                BOOST_LOG_OWL(info) << "stopping all service. ";
                ioc_im_gui_service.stop();
                ioc_keyboard.stop();
            }
                break;
            default:
                BOOST_LOG_OWL(warning) << "sig switch default.";
                break;
        }
    });
    safe_exit_signal = [&]() {
        BOOST_LOG_OWL(info) << "safe_exit_signal ";
        sig.clear();
        ioc_im_gui_service.stop();
        ioc_keyboard.stop();
    };

    size_t processor_count = boost::thread::hardware_concurrency();
    BOOST_LOG_OWL(info) << "processor_count: " << processor_count;

    boost::thread_group tg;
    tg.create_thread(ThreadCallee{ioc_im_gui_service, tg, "ioc_im_gui_service"});
    tg.create_thread(ThreadCallee{ioc_keyboard, tg, "ioc_keyboard"});

    BOOST_LOG_OWL(info) << "boost::thread_group running";
    BOOST_LOG_OWL(info) << "boost::thread_group size : " << tg.size();
    tg.join_all();
    BOOST_LOG_OWL(info) << "boost::thread_group end";

    BOOST_LOG_OWL(info) << "boost::thread_group all clear.";
    return 0;
}



