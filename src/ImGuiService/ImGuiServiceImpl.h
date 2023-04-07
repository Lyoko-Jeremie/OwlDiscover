// jeremie

#ifndef OWLDISCOVER_IMGUISERVICEIMPL_H
#define OWLDISCOVER_IMGUISERVICEIMPL_H


#include <boost/asio.hpp>
#include "../OwlLog/OwlLog.h"
#include "../MemoryBoost.h"

#include "../VERSION/ProgramVersion.h"
#include "../VERSION/CodeVersion.h"

#include "../MultiCast/ControlMulticastMail.h"
#include "../ConfigLoader/ConfigLoader.h"
#include "./ControlImGuiMailBox.h"
#include "../ControlService/ControlServiceUdpMailBox.h"
#include "../ControlService/ControlServiceHttpMailBox.h"

#include "../DiscoverState/DiscoverState.h"
#include "../GetImage/GetImage.h"


#include <sstream>
#include <utility>
#include <vector>
#include <list>
#include <array>
#include <boost/lexical_cast.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/json.hpp>
#include <boost/algorithm/string/find.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/composite_key.hpp>


#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <cstdio>
#include <utility>
#include <SDL.h>
#include <SDL_syswm.h>


namespace OwlImGuiServiceImpl {

    struct DiscoverStateItemContainerRandomAccess {
    };
    struct DiscoverStateItemContainerSequencedAccess {
    };

    typedef boost::multi_index_container<
            OwlDiscoverState::DiscoverStateItem,
            boost::multi_index::indexed_by<
                    boost::multi_index::sequenced<
                            boost::multi_index::tag<DiscoverStateItemContainerSequencedAccess>
                    >,
                    boost::multi_index::ordered_unique<
                            boost::multi_index::identity<OwlDiscoverState::DiscoverStateItem>
                    >,
                    boost::multi_index::hashed_unique<
                            boost::multi_index::tag<OwlDiscoverState::DiscoverStateItem::IP>,
                            boost::multi_index::member<OwlDiscoverState::DiscoverStateItem, std::string, &OwlDiscoverState::DiscoverStateItem::ip>
                    >,
                    boost::multi_index::random_access<
                            boost::multi_index::tag<DiscoverStateItemContainerRandomAccess>
                    >
            >
    > DiscoverStateItemContainer;

}

namespace OwlImGuiService {
    class ImGuiService;
}

namespace OwlImGuiServiceImpl {

    std::string operator "" _S(const char8_t *str, std::size_t);

    char const *operator "" _C(const char8_t *str, std::size_t);

    struct ImGuiServiceImpl : public boost::enable_shared_from_this<ImGuiServiceImpl> {
    public:
        ImGuiServiceImpl(
                boost::asio::io_context &ioc,
                boost::shared_ptr<OwlConfigLoader::ConfigLoader> config,
                boost::weak_ptr<OwlImGuiService::ImGuiService> parentPtr
        ) : ioc_(ioc),
            config_(std::move(config)),
            parentPtr_(std::move(parentPtr)),
            getImageService(boost::make_shared<OwlGetImage::GetImage>(ioc_)) {
        }

    private:
        boost::asio::io_context &ioc_;
        boost::shared_ptr<OwlConfigLoader::ConfigLoader> config_;
        boost::weak_ptr<OwlImGuiService::ImGuiService> parentPtr_;

        boost::shared_ptr<OwlGetImage::GetImage> getImageService;

        bool isCleaned = false;

    private:
        ID3D11Device *g_pd3dDevice = nullptr;
        ID3D11DeviceContext *g_pd3dDeviceContext = nullptr;
        IDXGISwapChain *g_pSwapChain = nullptr;
        ID3D11RenderTargetView *g_mainRenderTargetView = nullptr;

        DiscoverStateItemContainer items;

        OwlDiscoverState::PackageSendInfoContainer timeoutInfo;

    private:

        bool CreateDeviceD3D(HWND hWnd);

        void CleanupDeviceD3D();

        void CreateRenderTarget();

        void CleanupRenderTarget();

    public:
        void start();

        void clear();

        void update_state(const boost::shared_ptr<OwlDiscoverState::DiscoverStateItem> &a);

        void update_ota(std::string ip, std::string version);

        void new_state(const boost::shared_ptr<OwlDiscoverState::DiscoverStateItem> &a);

        void new_state(const boost::shared_ptr<OwlDiscoverState::DiscoverState> &s);

        void remove_old_package_send_info();

        void update_package_send_info(const boost::shared_ptr<OwlDiscoverState::PackageSendInfo> &s,int port);

    private:

        void sendCmdHttpReadAllOTA();

        void sendCmdHttpReadOTA(std::string ip);

        void scanSubnet();

        void scanSubnetPingHttp();

        void scanSubnetPingUdp();

        void do_all(OwlMailDefine::ControlCmd cmd);

        void do_ip(OwlMailDefine::ControlCmd cmd, std::string ip);


        void sortItem();

        void sortItemByDuration30();

        void sortItemByDuration();

        void deleteItem(const std::string &s);

        void cleanItem();


    private:
        SDL_Window *window = nullptr;

        std::string MainWindowTitle{std::string("OwlDiscover ") + ProgramVersion};

        int init();

        void start_main_loop();

    private:

        enum class CmdType {
            Udp,
            Http
        };

        int cmdType = static_cast<int>(CmdType::Udp);

        bool show_demo_window = false;
        bool show_about_window = false;
        bool show_test_cmd_window = false;
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        bool open = true;
        ImGuiWindowFlags gui_window_flags = 0;

        struct TableConfig {
            ImGuiTableFlags table_flags =
                    ImGuiTableFlags_Reorderable
                    | ImGuiTableFlags_RowBg
                    | ImGuiTableFlags_Borders
                    | ImGuiTableFlags_BordersH
                    | ImGuiTableFlags_BordersOuterH
                    | ImGuiTableFlags_BordersInnerH
                    | ImGuiTableFlags_BordersV
                    | ImGuiTableFlags_BordersOuterV
                    | ImGuiTableFlags_BordersInnerV
                    | ImGuiTableFlags_BordersOuter
                    | ImGuiTableFlags_BordersInner
                    | ImGuiTableFlags_ScrollX
                    | ImGuiTableFlags_ScrollY
                    | ImGuiTableFlags_SizingFixedFit;
            int freeze_cols = 2;
            int freeze_rows = 1;
        };
        TableConfig table_config;

        void HelpMarker(const char *desc);

        boost::asio::awaitable<bool> co_main_loop(boost::shared_ptr<ImGuiServiceImpl> self);

        struct TestCmdItem {
            std::string name;
            std::function<void()> callback;

            TestCmdItem(
                    std::string name_,
                    std::function<void()> callback_
            ) : name(std::move(name_)), callback(std::move(callback_)) {}
        };

        std::vector<TestCmdItem> testCmdList;

        void test_cmd_do_all(OwlMailDefine::ControlCmd cmd);

        void init_test_cmd_data();

        std::array<int, 3> goto_pos{0, 0, 0};

        void show_test_cmd();

        void show_camera(const OwlDiscoverState::DiscoverStateItem &a);

    };

} // OwlImGuiServiceImpl

#endif //OWLDISCOVER_IMGUISERVICEIMPL_H
