// jeremie

#ifndef OWLACCESSTERMINAL_CONFIGLOADER_H
#define OWLACCESSTERMINAL_CONFIGLOADER_H

#include "../MemoryBoost.h"
#include <sstream>
#include <variant>
#include <functional>
#include <map>
#include <tuple>
#include <limits>
#include <atomic>
#include <boost/filesystem/fstream.hpp>
#include <boost/system.hpp>
#include <boost/json.hpp>
#include <boost/utility/string_view.hpp>
#include "../OwlLog/OwlLog.h"

namespace OwlConfigLoader {

    struct Config {

        int CommandServiceUdpPort = 23333;
        int CommandServiceHttpPort = 23338;
        int ImageServiceTcpPort = 23332;
        int ImageServiceHttpPort = 23331;

        std::string multicast_address = "239.255.0.1";
        int multicast_port = 30003;
        std::string listen_address = "0.0.0.0";
        int multicast_interval_seconds = 15;

    };


    class ConfigLoader : public boost::enable_shared_from_this<ConfigLoader> {
    public:

        ~ConfigLoader() {
            BOOST_LOG_OWL(trace_dtor) << "~ConfigLoader()";
        }

        void print();

        void init(const std::string &filePath) {
            auto j = load_json_file(filePath);
            BOOST_LOG_OWL(info) << "j.is_object() " << j.is_object() << "\t"
                                << "j.kind() " << boost::json::to_string(j.kind());
            if (j.is_object()) {
                config_ = parse_json(j.as_object());
            } else {
                BOOST_LOG_OWL(error)
                    << "ConfigLoader: config file not exit OR cannot load config file OR config file invalid.";
            }
        }

        Config &config() {
            return *config_;
        }

    private:
        boost::shared_ptr<Config> config_ = boost::make_shared<Config>();

    private:

        static boost::json::value load_json_file(const std::string &filePath);

        boost::shared_ptr<Config> parse_json(const boost::json::value &&json_v);

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

        template<typename AT, typename T = std::remove_cvref_t<typename std::remove_cvref_t<AT>::value_type>>
        AT &getAtomic(const boost::json::object &v, boost::string_view key, AT &d) {
            try {
                if (!v.contains(key)) {
                    return d;
                }
                auto rr = boost::json::try_value_to<std::remove_cvref_t<T>>(v.at(key));
                if (rr.has_value()) {
                    d.store(rr.value());
                }
                return d;
            } catch (std::exception &e) {
                return d;
            }
        }

        boost::json::object getObj(const boost::json::object &v, boost::string_view key) {
            try {
                if (!v.contains(key)) {
                    return {};
                }
                auto oo = v.at(key);
                return oo.as_object();
            } catch (std::exception &e) {
                return {};
            }
        }
    };


} // OwlConfigLoader

#endif //OWLACCESSTERMINAL_CONFIGLOADER_H
