#include <vector>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/core/ignore_unused.hpp>

#include <SDL_main.h>


#include "./MemoryBoost.h"
#include "./OwlLog/OwlLog.h"

#include "./VERSION/ProgramVersion.h"
#include "./VERSION/CodeVersion.h"

#include <vector>
#include <utility>
#include <functional>
#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/thread.hpp>
#include <boost/program_options.hpp>

#include "./ImGuiService/ControlImGuiMailBox.h"
#include "./ImGuiService/ImGuiService.h"
#include "./ConfigLoader/ConfigLoader.h"
#include "./MultiCast/ControlMulticastMail.h"
#include "./MultiCast/MultiCast.h"
#include "./ControlService/ControlServiceUdpMailBox.h"
#include "./ControlService/ControlServiceHttpMailBox.h"
#include "./ControlService/UdpControl.h"
#include "./ControlService/HttpControl.h"

#ifndef DEFAULT_CONFIG
#define DEFAULT_CONFIG R"(config.json)"
#endif // DEFAULT_CONFIG

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
            OwlImGuiService::safe_exit();
            return -1;
        } catch (boost::exception &e) {
            tg.interrupt_all();
            // https://www.boost.org/doc/libs/1_81_0/libs/exception/doc/diagnostic_information.html
            BOOST_LOG_OWL(error) << "ThreadCallee catch std::exception :"
                                 << "\n diag: " << boost::diagnostic_information(e)
                                 << "\n what: " << dynamic_cast<std::exception const &>(e).what();
            OwlImGuiService::safe_exit();
            return -1;
        } catch (const std::exception &e) {
            tg.interrupt_all();
            BOOST_LOG_OWL(error) << "ThreadCallee catch std::exception: " << e.what();
            OwlImGuiService::safe_exit();
            return -1;
        } catch (...) {
            tg.interrupt_all();
            // https://www.boost.org/doc/libs/1_81_0/libs/exception/doc/current_exception_diagnostic_information.html
            BOOST_LOG_OWL(error) << "ThreadCallee catch (...) exception"
                                 << "\n current_exception_diagnostic_information : "
                                 << boost::current_exception_diagnostic_information();
            OwlImGuiService::safe_exit();
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

int main(int argc, char *argv[]) {

    OwlLog::threadName = "main";
    OwlLog::init_logging();
    BOOST_LOG_OWL(info) << "Hello, World!";

    // parse start params
    std::string config_file;
    boost::program_options::options_description desc("options");
    desc.add_options()
            ("config,c", boost::program_options::value<std::string>(&config_file)->
                    default_value(DEFAULT_CONFIG)->
                    value_name("CONFIG"), "specify config file")
            ("help,h", "print help message")
            ("version,v", "print version and build info");
    boost::program_options::positional_options_description pd;
    pd.add("config", 1);
    boost::program_options::variables_map vMap;
    boost::program_options::store(
            boost::program_options::command_line_parser(argc, argv)
                    .options(desc)
                    .positional(pd)
                    .run(), vMap);
    boost::program_options::notify(vMap);
    if (vMap.count("help")) {
        std::cout << "usage: " << argv[0] << " [[-c] CONFIG]" << "\n" << std::endl;

        std::cout << "    OwlAccessTerminal  Copyright (C) 2023 \n"
                  << "\n" << std::endl;

        std::cout << desc << std::endl;
        return 0;
    }
    if (vMap.count("version")) {
        std::cout <<
                  ", ProgramVersion " << ProgramVersion <<
                  ", CodeVersion_GIT_REV " << CodeVersion_GIT_REV <<
                  ", CodeVersion_GIT_TAG " << CodeVersion_GIT_TAG <<
                  ", CodeVersion_GIT_BRANCH " << CodeVersion_GIT_BRANCH <<
                  ", Boost " << BOOST_LIB_VERSION <<
                  ", BUILD_DATETIME " << CodeVersion_BUILD_DATETIME
                  << std::endl;
        return 0;
    }

    BOOST_LOG_OWL(info) << "config_file: " << config_file;



    // load config
    auto config = boost::make_shared<OwlConfigLoader::ConfigLoader>();
    config->init(config_file);
    config->print();

    boost::asio::io_context ioc_udp_control;
    boost::asio::io_context ioc_http_control;
    boost::asio::io_context ioc_multicast;
    boost::asio::io_context ioc_im_gui_service;

    auto mailbox_control_multicast = boost::make_shared<OwlMailDefine::ControlMulticastMailbox::element_type>(
            ioc_im_gui_service, ioc_multicast, "mailbox_control_multicast"
    );
    auto mailbox_control_im_gui = boost::make_shared<OwlMailDefine::ControlImGuiMailBox::element_type>(
            ioc_multicast, ioc_im_gui_service, "mailbox_control_im_gui"
    );
    auto mailbox_udp = boost::make_shared<OwlMailDefine::ControlUdpMailBox::element_type>(
            ioc_im_gui_service, ioc_udp_control, "mailbox_udp"
    );
    auto mailbox_http = boost::make_shared<OwlMailDefine::ControlHttpMailBox::element_type>(
            ioc_im_gui_service, ioc_http_control, "mailbox_http"
    );
    auto multiCastServer = boost::make_shared<OwlMultiCast::MultiCast>(
            ioc_multicast,
            config->shared_from_this(),
            mailbox_control_multicast->shared_from_this(),
            mailbox_control_im_gui->shared_from_this()
    );
    multiCastServer->start();
    auto udpControlService = boost::make_shared<OwlControlService::UdpControl>(
            ioc_udp_control,
            config->shared_from_this(),
            mailbox_udp->shared_from_this()
    );
    udpControlService->start();
    auto httpControlService = boost::make_shared<OwlControlService::HttpControl>(
            ioc_udp_control,
            config->shared_from_this(),
            mailbox_http->shared_from_this()
    );
    httpControlService->start();
    auto imGuiService = boost::make_shared<OwlImGuiService::ImGuiService>(
            ioc_im_gui_service,
            config->shared_from_this(),
            mailbox_control_im_gui->shared_from_this(),
            mailbox_control_multicast->shared_from_this(),
            mailbox_udp->shared_from_this(),
            mailbox_http->shared_from_this()
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
                ioc_multicast.stop();
                ioc_udp_control.stop();
                ioc_http_control.stop();
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
        ioc_multicast.stop();
        ioc_udp_control.stop();
        ioc_http_control.stop();
        ioc_keyboard.stop();
    };

    size_t processor_count = boost::thread::hardware_concurrency();
    BOOST_LOG_OWL(info) << "processor_count: " << processor_count;

    boost::thread_group tg;
    tg.create_thread(ThreadCallee{ioc_im_gui_service, tg, "ioc_im_gui_service"});
    tg.create_thread(ThreadCallee{ioc_multicast, tg, "ioc_multicast"});
    tg.create_thread(ThreadCallee{ioc_udp_control, tg, "ioc_udp_control"});
    tg.create_thread(ThreadCallee{ioc_http_control, tg, "ioc_http_control"});
    tg.create_thread(ThreadCallee{ioc_keyboard, tg, "ioc_keyboard"});

    BOOST_LOG_OWL(info) << "boost::thread_group running";
    BOOST_LOG_OWL(info) << "boost::thread_group size : " << tg.size();
    tg.join_all();
    BOOST_LOG_OWL(info) << "boost::thread_group end";

    BOOST_LOG_OWL(info) << "boost::thread_group all clear.";
    return 0;
}



