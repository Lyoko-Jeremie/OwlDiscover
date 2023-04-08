// jeremie

#ifndef OWLDISCOVER_DISCOVERSTATE_H
#define OWLDISCOVER_DISCOVERSTATE_H

#include <string>
#include <deque>
#include <utility>
#include <memory>
#include <tuple>
#include "../MemoryBoost.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>
#include "../ImGuiService/ImGuiDirectX11.h"


#include <boost/multi_index_container.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/composite_key.hpp>


namespace OwlDiscoverState {

    enum class PackageSendInfoDirectEnum {
        out = 1,
        in = 2,
        state = 3,
    };

    struct PackageSendInfo {
        struct SequencedAccess {
        };
        struct RandomAccess {
        };

        struct IP {
        };
        const std::string ip;

        // send out (to network (request package))
        boost::posix_time::ptime outTime;
        // receive int (come from network (response package))
        boost::posix_time::ptime inTime;

        void setNowOutTime() {
            outTime = boost::posix_time::microsec_clock::local_time();
        }

        void setNowInTime() {
            inTime = boost::posix_time::microsec_clock::local_time();
        }

        struct PKG {
            static std::tuple<std::string, size_t, size_t, size_t> make_tuple(const PackageSendInfo &o) {
                return std::make_tuple(
                        o.ip,
                        o.packageId,
                        o.cmdId,
                        o.clientId
                );
            }
        };

        const size_t packageId;
        const size_t cmdId;
        const size_t clientId;

        PackageSendInfoDirectEnum direct;

        PackageSendInfo() = delete;

        PackageSendInfo(
                std::string ip_,
                size_t packageId_,
                size_t cmdId_,
                size_t clientId_,
                PackageSendInfoDirectEnum direct_
        ) : ip(std::move(ip_)),
            packageId(packageId_),
            cmdId(cmdId_),
            clientId(clientId_),
            direct(direct_) {
            if (direct == PackageSendInfoDirectEnum::out) {
                setNowOutTime();
            }
            if (direct == PackageSendInfoDirectEnum::in) {
                setNowInTime();
            }
        }

        size_t msDelay = 0;

        [[nodiscard]] bool needUpdateDelay() const {
            if (outTime == boost::posix_time::not_a_date_time) {
                return false;
            }
            if (inTime == boost::posix_time::not_a_date_time) {
                return false;
            }
            return msDelay == 0;
        }

        bool updateDelay() {
            if (outTime == boost::posix_time::not_a_date_time) {
                msDelay = 0;
                return false;
            }
            if (inTime == boost::posix_time::not_a_date_time) {
                msDelay = 0;
                return false;
            }
            auto a = (inTime - outTime);
            auto ms = a.total_milliseconds();
            if (ms > 0) {
                msDelay = ms;
                return true;
            }
            msDelay = 0;
            return false;
        }

        auto operator<=>(const PackageSendInfo &o) const {
            auto ipO = ip <=> o.ip;
            if (ipO != std::strong_ordering::equivalent) {
                return ipO;
            }
            if (outTime != o.outTime) {
                return outTime < o.outTime ? std::strong_ordering::less : std::strong_ordering::greater;
            }
            if (inTime != o.inTime) {
                return inTime < o.inTime ? std::strong_ordering::less : std::strong_ordering::greater;
            }
            return msDelay <=> o.msDelay;
        }
    };


    typedef boost::multi_index_container<
            OwlDiscoverState::PackageSendInfo,
            boost::multi_index::indexed_by<
                    boost::multi_index::sequenced<
                            boost::multi_index::tag<OwlDiscoverState::PackageSendInfo::SequencedAccess>
                    >,
                    boost::multi_index::ordered_unique<
                            boost::multi_index::identity<OwlDiscoverState::PackageSendInfo>
                    >,
                    boost::multi_index::hashed_non_unique<
                            boost::multi_index::tag<OwlDiscoverState::PackageSendInfo::IP>,
                            boost::multi_index::member<OwlDiscoverState::PackageSendInfo, const std::string, &OwlDiscoverState::PackageSendInfo::ip>
                    >,
                    boost::multi_index::hashed_unique<
                            boost::multi_index::tag<OwlDiscoverState::PackageSendInfo::PKG>,
                            boost::multi_index::composite_key<
                                    OwlDiscoverState::PackageSendInfo,
                                    boost::multi_index::member<OwlDiscoverState::PackageSendInfo, const std::string, &OwlDiscoverState::PackageSendInfo::ip>,
                                    boost::multi_index::member<OwlDiscoverState::PackageSendInfo, const size_t, &OwlDiscoverState::PackageSendInfo::packageId>,
                                    boost::multi_index::member<OwlDiscoverState::PackageSendInfo, const size_t, &OwlDiscoverState::PackageSendInfo::cmdId>,
                                    boost::multi_index::member<OwlDiscoverState::PackageSendInfo, const size_t, &OwlDiscoverState::PackageSendInfo::clientId>
                            >
                    >/*,
                    boost::multi_index::random_access<
                            boost::multi_index::tag<OwlDiscoverState::PackageSendInfo::RandomAccess>
                    >*/
            >
    > PackageSendInfoContainer;

    struct DiscoverStateItem {
        struct IP {
        };
        std::string ip;
        int port;
        boost::posix_time::ptime firstTime;
        boost::posix_time::ptime lastTime;

        boost::shared_ptr<bool> cmdTestSelected = boost::make_shared<bool>(false);
        boost::shared_ptr<bool> stateDebugSelected = boost::make_shared<bool>(false);
        boost::shared_ptr<bool> showCamera = boost::make_shared<bool>(false);

        std::string programVersion;
        std::string gitRev;
        std::string buildTime;
        std::string versionOTA;

        std::string cacheFirstTime;
        std::string cacheLastTime;

        boost::shared_ptr<OwlImGuiDirectX11::ImGuiD3D11Img> imgDataFont = boost::make_shared<OwlImGuiDirectX11::ImGuiD3D11Img>();
        boost::shared_ptr<OwlImGuiDirectX11::ImGuiD3D11Img> imgDataDown = boost::make_shared<OwlImGuiDirectX11::ImGuiD3D11Img>();

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
