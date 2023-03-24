// jeremie

#ifndef OWLDISCOVER_DISCOVERSTATE_H
#define OWLDISCOVER_DISCOVERSTATE_H

#include <string>
#include <deque>
#include <utility>
#include "../MemoryBoost.h"
#include <boost/date_time/posix_time/posix_time.hpp>

namespace OwlDiscoverState {

    struct DiscoverStateItem {
        std::string ip;
        int port;
        boost::posix_time::ptime lastTime;
        boost::posix_time::ptime firstTime;

        DiscoverStateItem(
                std::string ip_,
                int port_
        ) : ip(std::move(ip_)), port(port_) {
            firstTime = boost::posix_time::microsec_clock::local_time();
            lastTime = boost::posix_time::microsec_clock::local_time();
        }

        long long calcDuration() {
            auto a = (lastTime - firstTime);
            auto n = a.total_milliseconds();
            return std::abs(n);
        }

        std::string getTimeString(boost::posix_time::ptime t) {
            auto s = boost::posix_time::to_iso_extended_string(t);
            if (s.size() > 10) {
                s.at(10) = ' ';
            }
            return s;
        }
    };

    struct DiscoverState : public boost::enable_shared_from_this<DiscoverState> {
        std::deque<DiscoverStateItem> items;
    };

}

#endif //OWLDISCOVER_DISCOVERSTATE_H
