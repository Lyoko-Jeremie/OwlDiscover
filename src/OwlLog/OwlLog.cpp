// jeremie

#include "OwlLog.h"
#include <boost/core/null_deleter.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>


namespace OwlLog {
    thread_local std::string threadName;

    // https://zhuanlan.zhihu.com/p/389736260
    boost::log::sources::severity_logger<severity_level> slg;

    // https://stackoverflow.com/questions/23137637/linker-error-while-linking-boost-log-tutorial-undefined-references

    // https://stackoverflow.com/questions/17930553/in-boost-log-how-do-i-format-a-custom-severity-level-using-a-format-string
    const char *severity_level_str[severity_level::MAX] = {
            "trace_sp_tag",
            "trace_cmd_tag",
            "trace_cmd_http",
            "trace_cmd_sp_w",
            "trace_cmd_sp_r",
            "trace_map",
            "trace_dtor",
            "trace_json",
            "trace_multicast",
            "trace",
            "debug_sp_w",
            "debug",
            "info",
            "info_VSERION",
            "warning",
            "error",
            "fatal"
    };

    template<typename CharT, typename TraitsT>
    std::basic_ostream<CharT, TraitsT> &
    operator<<(std::basic_ostream<CharT, TraitsT> &strm, severity_level lvl) {
        const char *str = severity_level_str[lvl];
        if (lvl < severity_level::MAX && lvl >= 0)
            strm << str;
        else
            strm << static_cast< int >(lvl);
        return strm;
    }

    class thread_name_impl :
            public boost::log::attribute::impl {
    public:
        boost::log::attribute_value get_value() override {
            return boost::log::attributes::make_attribute_value(
                    OwlLog::threadName.empty() ? std::string("no name") : OwlLog::threadName);
        }

        using value_type = std::string;
    };

    class thread_name :
            public boost::log::attribute {
    public:
        thread_name() : boost::log::attribute(new thread_name_impl()) {
        }

        explicit thread_name(boost::log::attributes::cast_source const &source)
                : boost::log::attribute(source.as<thread_name_impl>()) {
        }

        using value_type = thread_name_impl::value_type;

    };

    BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", severity_level)

    void init_logging() {

        // https://stackoverflow.com/questions/15853981/boost-log-2-0-empty-severity-level-in-logs
        boost::log::register_simple_formatter_factory<severity_level, char>("Severity");

        boost::shared_ptr<boost::log::core> core = boost::log::core::get();

        typedef boost::log::sinks::synchronous_sink<boost::log::sinks::text_ostream_backend> sink_t;
        boost::shared_ptr<sink_t> sink(new sink_t());
        sink->locked_backend()->add_stream(boost::shared_ptr<std::ostream>(&std::cout, boost::null_deleter()));
        sink->set_formatter(
                boost::log::expressions::stream
                        // << "["
                        // << std::setw(5)
                        // << boost::log::expressions::attr<unsigned int>("LineID")
                        // << "]"
                        << "["
                        << boost::log::expressions::format_date_time<boost::posix_time::ptime>(
                                "TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
                        << "]"
                        // << "["
                        // << boost::log::expressions::attr<boost::log::attributes::current_process_id::value_type>(
                        //         "ProcessID")
                        // << "]"
                        // << "["
                        // << boost::log::expressions::attr<boost::log::attributes::current_process_name::value_type>(
                        //         "ProcessName")
                        // << "]"
                        << "["
                        << boost::log::expressions::attr<boost::log::attributes::current_thread_id::value_type>(
                                "ThreadID")
                        << "]"
                        << "["
                        << std::setw(20)
                        << boost::log::expressions::attr<thread_name::value_type>("ThreadName")
                        << "]"
                        //                        << "[" << boost::log::trivial::severity << "] "
                        << "[" << boost::log::expressions::attr<severity_level>("Severity") << "] "
                        << boost::log::expressions::smessage);
        core->add_sink(sink);

        // https://www.boost.org/doc/libs/1_81_0/libs/log/doc/html/log/detailed/attributes.html
        // core->add_global_attribute("LineID", boost::log::attributes::counter<size_t>(1));
        core->add_global_attribute("ThreadName", thread_name());
        core->add_global_attribute("ThreadID", boost::log::attributes::current_thread_id());
        core->add_global_attribute("TimeStamp", boost::log::attributes::local_clock());
        // core->add_global_attribute("ProcessID", boost::log::attributes::current_process_id());
        // core->add_global_attribute("ProcessName", boost::log::attributes::current_process_name());

        // https://stackoverflow.com/questions/69967084/how-to-set-the-severity-level-of-boost-log-library
        // core->get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::info);
        // core->get()->set_filter(severity >= severity_level::error);
        // core->get()->set_filter(severity != severity_level::error);
        // core->get()->set_filter(severity != severity_level::trace);

        core->get()->set_filter(
                true
                #ifndef DEBUG_log_sp_tag
                && severity != severity_level::trace_sp_tag
                #endif // DEBUG_log_sp_tag
                #ifndef DEBUG_log_dtor
                && severity != severity_level::trace_dtor
                #endif // DEBUG_log_dtor
                #ifndef DEBUG_log_MAP
                && severity != severity_level::trace_map
                #endif // DEBUG_log_MAP
                #ifndef DEBUG_log_TAG
                && severity != severity_level::trace_cmd_tag
                #endif // DEBUG_log_TAG
                #ifndef DEBUG_log_HTTP
                && severity != severity_level::trace_cmd_http
                #endif // DEBUG_log_HTTP
                #ifndef DEBUG_log_SerialPortWrite
                && severity != severity_level::trace_cmd_sp_w
                #endif // DEBUG_log_SerialPortWrite
                #ifndef DEBUG_log_SerialPortWrite_dbg
                && severity != severity_level::debug_sp_w
                #endif // DEBUG_log_SerialPortWrite_dbg
                #ifndef DEBUG_log_JSON
                && severity != severity_level::trace_json
                #endif // DEBUG_log_JSON
                #ifndef DEBUG_log_multicast
                && severity != severity_level::trace_multicast
                #endif // DEBUG_log_multicast
                #ifndef DEBUG_log_SerialPortRead
                && severity != severity_level::trace_cmd_sp_r
#endif // DEBUG_log_SerialPortRead
        );

//        BOOST_LOG_OWL(info_VSERION)
//            << "OwlAccessTerminal"
//            << "\n   ProgramVersion " << ProgramVersion
//            << "\n   CodeVersion_GIT_REV " << CodeVersion_GIT_REV
//            << "\n   CodeVersion_GIT_TAG " << CodeVersion_GIT_TAG
//            << "\n   CodeVersion_GIT_BRANCH " << CodeVersion_GIT_BRANCH
//            << "\n   Boost " << BOOST_LIB_VERSION
//            << "\n   ProtoBuf " << GOOGLE_PROTOBUF_VERSION
//            << "\n   OpenCV " << CV_VERSION
//            << "\n   BUILD_DATETIME " << CodeVersion_BUILD_DATETIME
//            << "\n ---------- OwlAccessTerminal  Copyright (C) 2023 ---------- ";

        BOOST_LOG_OWL(trace_sp_tag) << "BOOST_LOG_OWL(trace_sp_tag)";
        BOOST_LOG_OWL(trace_cmd_tag) << "BOOST_LOG_OWL(trace_cmd_tag)";
        BOOST_LOG_OWL(trace_cmd_http) << "BOOST_LOG_OWL(trace_cmd_http)";
        BOOST_LOG_OWL(trace_cmd_sp_w) << "BOOST_LOG_OWL(trace_cmd_sp_w)";
        BOOST_LOG_OWL(trace_cmd_sp_r) << "BOOST_LOG_OWL(trace_cmd_sp_r)";
        BOOST_LOG_OWL(trace_map) << "BOOST_LOG_OWL(trace_map)";
        BOOST_LOG_OWL(trace_dtor) << "BOOST_LOG_OWL(trace_dtor)";
        BOOST_LOG_OWL(trace_json) << "BOOST_LOG_OWL(trace_json)";
        BOOST_LOG_OWL(trace_multicast) << "BOOST_LOG_OWL(trace_multicast)";
        BOOST_LOG_OWL(trace) << "BOOST_LOG_OWL(trace)";
        BOOST_LOG_OWL(debug) << "BOOST_LOG_OWL(debug)";
        BOOST_LOG_OWL(debug_sp_w) << "BOOST_LOG_OWL(debug_sp_w)";
        BOOST_LOG_OWL(info) << "BOOST_LOG_OWL(info)";
        BOOST_LOG_OWL(info_VSERION) << "BOOST_LOG_OWL(info_VSERION)";
        BOOST_LOG_OWL(warning) << "BOOST_LOG_OWL(warning)";
        BOOST_LOG_OWL(error) << "BOOST_LOG_OWL(error)";
        BOOST_LOG_OWL(fatal) << "BOOST_LOG_OWL(fatal)";

    }
} // OwlLog
