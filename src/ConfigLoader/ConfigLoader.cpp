// jeremie

#include "ConfigLoader.h"


namespace OwlConfigLoader {


    // https://www.boost.org/doc/libs/1_81_0/libs/json/doc/html/json/examples.html
    void pretty_print(std::ostream &os, boost::json::value const &jv, std::string *indent = nullptr) {
        std::string indent_;
        if (!indent)
            indent = &indent_;
        switch (jv.kind()) {
            case boost::json::kind::object: {
                os << "{\n";
                indent->append(4, ' ');
                auto const &obj = jv.get_object();
                if (!obj.empty()) {
                    auto it = obj.begin();
                    for (;;) {
                        os << *indent << boost::json::serialize(it->key()) << " : ";
                        pretty_print(os, it->value(), indent);
                        if (++it == obj.end())
                            break;
                        os << ",\n";
                    }
                }
                os << "\n";
                indent->resize(indent->size() - 4);
                os << *indent << "}";
                break;
            }

            case boost::json::kind::array: {
                os << "[\n";
                indent->append(4, ' ');
                auto const &arr = jv.get_array();
                if (!arr.empty()) {
                    auto it = arr.begin();
                    for (;;) {
                        os << *indent;
                        pretty_print(os, *it, indent);
                        if (++it == arr.end())
                            break;
                        os << ",\n";
                    }
                }
                os << "\n";
                indent->resize(indent->size() - 4);
                os << *indent << "]";
                break;
            }

            case boost::json::kind::string: {
                os << boost::json::serialize(jv.get_string());
                break;
            }

            case boost::json::kind::uint64:
                os << jv.get_uint64();
                break;

            case boost::json::kind::int64:
                os << jv.get_int64();
                break;

            case boost::json::kind::double_:
                os << jv.get_double();
                break;

            case boost::json::kind::bool_:
                if (jv.get_bool())
                    os << "true";
                else
                    os << "false";
                break;

            case boost::json::kind::null:
                os << "null";
                break;
        }

        if (indent->empty())
            os << "\n";
    }

    boost::json::value ConfigLoader::load_json_file(const std::string &filePath) {
        boost::system::error_code ec;
        std::stringstream ss;
        boost::filesystem::ifstream f(filePath);
        if (!f) {
            BOOST_LOG_OWL(error) << "load_json_file (!f)";
            return nullptr;
        }
        ss << f.rdbuf();
        f.close();
        boost::json::stream_parser p;
        p.write(ss.str(), ec);
        if (ec) {
            BOOST_LOG_OWL(error) << "load_json_file (ec) " << ec.what();
            return nullptr;
        }
        p.finish(ec);
        if (ec) {
            BOOST_LOG_OWL(error) << "load_json_file (ec) " << ec.what();
            return nullptr;
        }
        return p.release();
    }

    boost::shared_ptr<Config> ConfigLoader::parse_json(const boost::json::value &&json_v) {
        const auto &root = json_v.as_object();

        {
            std::stringstream ss;
            pretty_print(ss, root);
            auto s = ss.str();
            BOOST_LOG_OWL(info) << "parse_json from : \n" << s;
            // BOOST_LOG_OWL(info) << "parse_json from : \n" << boost::json::serialize(root);
        }

        boost::shared_ptr<Config> _config_ = boost::make_shared<Config>();
        auto &config = *_config_;

        config.CommandServiceUdpPort = get(root, "CommandServiceUdpPort", config.CommandServiceUdpPort);
        config.CommandServiceHttpPort = get(root, "CommandServiceHttpPort", config.CommandServiceHttpPort);
        config.ImageServiceTcpPort = get(root, "ImageServiceTcpPort", config.ImageServiceTcpPort);
        config.ImageServiceHttpPort = get(root, "ImageServiceHttpPort", config.ImageServiceHttpPort);

        config.multicast_address = get(root, "multicast_address", config.multicast_address);
        config.multicast_port = get(root, "multicast_port", config.multicast_port);
        config.listen_address = get(root, "listen_address", config.listen_address);
        config.multicast_interval_seconds = get(root, "multicast_interval_seconds", config.multicast_interval_seconds);

        return _config_;
    }

    void ConfigLoader::print() {
        auto &config = *config_;
        BOOST_LOG_OWL(info)
            << "\n"
            << "\n" << "ConfigLoader config:"
            << "\n" << "CommandServiceUdpPort " << config.CommandServiceUdpPort
            << "\n" << "CommandServiceHttpPort " << config.CommandServiceHttpPort
            << "\n" << "ImageServiceTcpPort " << config.ImageServiceTcpPort
            << "\n" << "ImageServiceHttpPort " << config.ImageServiceHttpPort
            << "\n" << "multicast_address " << config.multicast_address
            << "\n" << "multicast_port " << config.multicast_port
            << "\n" << "listen_address " << config.listen_address
            << "\n" << "multicast_interval_seconds " << config.multicast_interval_seconds
            << "";
    }

} // OwlConfigLoader