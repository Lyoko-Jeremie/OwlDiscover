// jeremie

#ifndef OWLDISCOVER_DISCOVERSTATE_H
#define OWLDISCOVER_DISCOVERSTATE_H

#include <string>
#include <deque>
#include <utility>
#include "../MemoryBoost.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>

namespace OwlDiscoverState {

    struct DiscoverStateItem {
        struct IP {
        };
        std::string ip;
        int port;
        boost::posix_time::ptime firstTime;
        boost::posix_time::ptime lastTime;

        std::string programVersion;
        std::string gitRev;
        std::string buildTime;

        std::string cacheFirstTime;
        std::string cacheLastTime;

        auto operator<=>(const DiscoverStateItem &o) const {
            return ip <=> o.ip;
        }

        DiscoverStateItem(
                std::string ip_,
                int port_
        ) : ip(std::move(ip_)), port(port_) {
            firstTime = boost::posix_time::microsec_clock::local_time();
            lastTime = firstTime;
            updateCache();
        }

        void updateCache() {
            cacheFirstTime = getTimeString(firstTime);
            cacheLastTime = getTimeString(lastTime);
        }

        [[nodiscard]] std::string calcDuration(boost::posix_time::ptime nowTime) const {
            auto a = (nowTime - lastTime);
            auto n = a.total_seconds();
            return boost::lexical_cast<std::string>(std::abs(n));
        }

        [[nodiscard]] long long calcDurationSecond(boost::posix_time::ptime nowTime) const {
            auto a = (nowTime - lastTime);
            return a.total_seconds();
        }

        [[nodiscard]] std::string nowDuration() const {
            return calcDuration(now());
        }

        [[nodiscard]] static std::string nowTimeString() {
            return getTimeString(now());
        }

        [[nodiscard]] static boost::posix_time::ptime now() {
            return boost::posix_time::microsec_clock::local_time();
        }

        [[nodiscard]] static std::string getTimeString(boost::posix_time::ptime t) {
            auto s = boost::posix_time::to_iso_extended_string(t);
            if (s.size() > 10) {
                s.at(10) = '_';
            }
            return s;
        }
    };

    struct DiscoverState : public boost::enable_shared_from_this<DiscoverState> {
        std::deque<DiscoverStateItem> items;
    };

}

#endif //OWLDISCOVER_DISCOVERSTATE_H
