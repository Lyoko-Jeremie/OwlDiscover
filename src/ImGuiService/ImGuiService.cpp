// jeremie

#include "ImGuiService.h"
#include "../OwlLog/OwlLog.h"

#include "../VERSION/ProgramVersion.h"
#include "../VERSION/CodeVersion.h"

#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/core/ignore_unused.hpp>


#include <boost/multi_index_container.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/composite_key.hpp>

#include "../DiscoverState/DiscoverState.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <cstdio>
#include <utility>
#include <SDL.h>
#include <SDL_syswm.h>

using boost::asio::use_awaitable;
#if defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
# define use_awaitable \
  boost::asio::use_awaitable_t(__FILE__, __LINE__, __PRETTY_FUNCTION__)
#endif

namespace OwlImGuiService {

    struct DiscoverStateItemContainerRandomAccess {
    };

    typedef boost::multi_index_container<
            OwlDiscoverState::DiscoverStateItem,
            boost::multi_index::indexed_by<
                    boost::multi_index::sequenced<>,
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

    std::string operator "" _S(const char8_t *str, std::size_t) {
        return reinterpret_cast< const char * >(str);
    }

    char const *operator "" _C(const char8_t *str, std::size_t) {
        return reinterpret_cast< const char * >(str);
    }

    struct ImGuiServiceImpl : public boost::enable_shared_from_this<ImGuiServiceImpl> {
    public:
        ImGuiServiceImpl(
                boost::asio::io_context &ioc,
                boost::weak_ptr<ImGuiService> parentPtr
        ) : ioc_(ioc), parentPtr_(std::move(parentPtr)) {

            std::stringstream ss;
            ss << "OwlDiscover"
               << "\n   ProgramVersion " << ProgramVersion
               << "\n   CodeVersion_GIT_REV " << CodeVersion_GIT_REV
               << "\n   CodeVersion_GIT_TAG " << CodeVersion_GIT_TAG
               << "\n   CodeVersion_GIT_BRANCH " << CodeVersion_GIT_BRANCH
               << "\n   Boost " << BOOST_LIB_VERSION
               << "\n   Dear ImGui " << IMGUI_VERSION
               << "\n   BUILD_DATETIME " << CodeVersion_BUILD_DATETIME
               << "\n ---------- OwlDiscover  Copyright (C) 2023 ---------- ";
            CopyrightString = ss.str();
        }

    private:
        boost::asio::io_context &ioc_;
        boost::weak_ptr<ImGuiService> parentPtr_;

        bool isCleaned = false;

    private:
        ID3D11Device *g_pd3dDevice = nullptr;
        ID3D11DeviceContext *g_pd3dDeviceContext = nullptr;
        IDXGISwapChain *g_pSwapChain = nullptr;
        ID3D11RenderTargetView *g_mainRenderTargetView = nullptr;

        DiscoverStateItemContainer items;

    private:

        bool CreateDeviceD3D(HWND hWnd) {
            // Setup swap chain
            DXGI_SWAP_CHAIN_DESC sd;
            ZeroMemory(&sd, sizeof(sd));
            sd.BufferCount = 2;
            sd.BufferDesc.Width = 0;
            sd.BufferDesc.Height = 0;
            sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            sd.BufferDesc.RefreshRate.Numerator = 60;
            sd.BufferDesc.RefreshRate.Denominator = 1;
            sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
            sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            sd.OutputWindow = hWnd;
            sd.SampleDesc.Count = 1;
            sd.SampleDesc.Quality = 0;
            sd.Windowed = TRUE;
            sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

            UINT createDeviceFlags = 0;
            //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
            D3D_FEATURE_LEVEL featureLevel;
            const D3D_FEATURE_LEVEL featureLevelArray[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0,};
            if (D3D11CreateDeviceAndSwapChain(
                    nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
                    featureLevelArray, 2,
                    D3D11_SDK_VERSION,
                    &sd, &g_pSwapChain, &g_pd3dDevice,
                    &featureLevel,
                    &g_pd3dDeviceContext) != S_OK) {
                return false;
            }

            CreateRenderTarget();
            return true;
        }

        void CleanupDeviceD3D() {
            CleanupRenderTarget();
            if (g_pSwapChain) {
                g_pSwapChain->Release();
                g_pSwapChain = nullptr;
            }
            if (g_pd3dDeviceContext) {
                g_pd3dDeviceContext->Release();
                g_pd3dDeviceContext = nullptr;
            }
            if (g_pd3dDevice) {
                g_pd3dDevice->Release();
                g_pd3dDevice = nullptr;
            }
        }

        void CreateRenderTarget() {
            ID3D11Texture2D *pBackBuffer;
            g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
            g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
            pBackBuffer->Release();
        }

        void CleanupRenderTarget() {
            if (g_mainRenderTargetView) {
                g_mainRenderTargetView->Release();
                g_mainRenderTargetView = nullptr;
            }
        }

    public:
        void start() {
            boost::asio::post(ioc_, [
                    this, self = shared_from_this()
            ]() {
                auto r = init();
                if (r) {
                    BOOST_LOG_OWL(error) << "ImGuiServiceImpl init error " << r;
                    clear();
                    safe_exit();
                    return;
                }
            });
        }

        void clear() {

            if (!isCleaned) {
                isCleaned = true;

                // Cleanup
                ImGui_ImplDX11_Shutdown();
                ImGui_ImplSDL2_Shutdown();
                ImGui::DestroyContext();

                CleanupDeviceD3D();
                SDL_DestroyWindow(window);
                window = nullptr;
                SDL_Quit();
            }


        }

        void new_state(const boost::shared_ptr<OwlDiscoverState::DiscoverState> &s) {
            BOOST_ASSERT(s);
            BOOST_ASSERT(!weak_from_this().expired());

            boost::asio::dispatch(ioc_, [this, s, self = shared_from_this()]() {
                BOOST_ASSERT(s);
                BOOST_ASSERT(self);

                auto &accIp = items.get<OwlDiscoverState::DiscoverStateItem::IP>();
                auto accIpEnd = accIp.end();
                for (const auto &a: s->items) {
                    BOOST_LOG_OWL(trace) << "ImGuiServiceImpl::new_state a " << a.ip;

                    auto it = accIp.find(a.ip);
                    if (it != accIpEnd) {
                        // update
                        accIp.modify(it, [&a](OwlDiscoverState::DiscoverStateItem &n) {
                            n.lastTime = a.lastTime;
                            n.port = a.port;
                            n.updateCache();
                        });
                    } else {
                        // insert
                        auto b = a;
                        b.updateCache();
                        items.emplace_back(b);
                    }
                }

                // re short it
                sortItem();

            });

        }

    private:

        void do_all(OwlMailDefine::ControlCmd cmd) {
            auto p = parentPtr_.lock();
            if (p) {
                auto &accIp = items.get<OwlDiscoverState::DiscoverStateItem::IP>();
                auto accIpEnd = accIp.end();
                for (auto it = accIp.begin(); it != accIpEnd; ++it) {
                    auto m = boost::make_shared<OwlMailDefine::ControlCmdData>();
                    m->cmd = cmd;
                    m->ip = it->ip;
                    p->sendCmdUdp(std::move(m));
                }
            }
        }

        void do_ip(OwlMailDefine::ControlCmd cmd, std::string ip) {
            auto p = parentPtr_.lock();
            if (p) {
                auto m = boost::make_shared<OwlMailDefine::ControlCmdData>();
                m->cmd = cmd;
                m->ip = std::move(ip);
                p->sendCmdUdp(std::move(m));
            }
        }


        void sortItem() {
            sortItemByDuration30();
        }

        void sortItemByDuration30() {
            auto now = OwlDiscoverState::DiscoverStateItem::now();
            auto &accIp = items.get<DiscoverStateItemContainerRandomAccess>();
            accIp.sort([&now](
                    const OwlDiscoverState::DiscoverStateItem &a,
                    const OwlDiscoverState::DiscoverStateItem &b
            ) {
                auto at = a.calcDurationSecond(now);
                auto bt = b.calcDurationSecond(now);
                if (at == bt) {
                    return a.ip < b.ip;
                }
                if (at > 30 && bt > 30) {
                    return a.ip < b.ip;
                }
                if (at <= 30 && bt <= 30) {
                    return a.ip < b.ip;
                }
                if (at <= 30 && bt > 30) {
                    return true;
                }
                if (at > 30 && bt <= 30) {
                    return false;
                }
                return false;
            });
        }

        void sortItemByDuration() {
            auto now = OwlDiscoverState::DiscoverStateItem::now();
            auto &accIp = items.get<DiscoverStateItemContainerRandomAccess>();
            accIp.sort([&now](
                    const OwlDiscoverState::DiscoverStateItem &a,
                    const OwlDiscoverState::DiscoverStateItem &b
            ) {
                auto at = a.calcDurationSecond(now);
                auto bt = b.calcDurationSecond(now);
                if (at < bt) {
                    return true;
                }
                if (at == bt) {
                    return a.ip < b.ip;
                }
                return false;
            });
        }

        void deleteItem(const std::string &s) {
            BOOST_LOG_OWL(trace) << "deleteItem " << s;
            auto &accIp = items.get<OwlDiscoverState::DiscoverStateItem::IP>();
            auto n = accIp.erase(s);
            BOOST_LOG_OWL(trace) << "deleteItem " << n;
        }

        void cleanItem() {
            items.clear();
        }


    private:
        SDL_Window *window = nullptr;

        std::string MainWindowTitle{std::string("OwlDiscover ") + ProgramVersion};

        int init() {

            auto ppp = parentPtr_.lock();
            if (!ppp) {
                BOOST_LOG_OWL(error) << "ImGuiServiceImpl init() (!ppp)";
                return -2;
            }

            // Setup SDL
            // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
            // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to the latest version of SDL is recommended!)
            if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
                printf("Error: %s\n", SDL_GetError());
                return -1;
            }

            // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
            SDL_SetHint(reinterpret_cast<const char *>(SDL_HINT_IME_SHOW_UI), "1");
#endif

            // Setup window
            SDL_WindowFlags window_flags = (SDL_WindowFlags) (SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
            window = SDL_CreateWindow(MainWindowTitle.c_str(),
                                      SDL_WINDOWPOS_CENTERED,
                                      SDL_WINDOWPOS_CENTERED,
                                      1280, 720, window_flags);
            SDL_SysWMinfo wmInfo;
            SDL_VERSION(&wmInfo.version);
            SDL_GetWindowWMInfo(window, &wmInfo);
            HWND hwnd = (HWND) wmInfo.info.win.window;

            // Initialize Direct3D
            if (!CreateDeviceD3D(hwnd)) {
                CleanupDeviceD3D();
                return 1;
            }

            // Setup Dear ImGui context
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO &io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

            // Setup Dear ImGui style
            ImGui::StyleColorsDark();
            // ImGui::StyleColorsLight();

            // Setup Platform/Renderer backends
            ImGui_ImplSDL2_InitForD3D(window);
            ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);


            ImFontConfig config;
            config.OversampleH = 2;
            config.OversampleV = 1;
            config.GlyphExtraSpacing.x = 1.0f;
            std::vector<ImFont *> fontsList{
//            io.Fonts->AddFontDefault(),
//            io.Fonts->AddFontFromFileTTF(R"(c:\Windows\Fonts\segoeui.ttf)", 18.0f,
//                                         &config, io.Fonts->GetGlyphRangesChineseFull()),
                    io.Fonts->AddFontFromFileTTF(ppp->config().font_path.c_str(), 21.0f,
                                                 &config, io.Fonts->GetGlyphRangesChineseFull()),
            };
            io.Fonts->Build();
            for (const auto &a: fontsList) {
                if (!a) {
                    // cannot read fonts
                    clear();
                    safe_exit();
                    return -2;
                }
                BOOST_LOG_OWL(info) << a->GetDebugName() << " " << a << " " << a->IsLoaded();
                a->ContainerAtlas->Build();
            }
            io.FontDefault = fontsList.at(0);


            // now start loop
            start_main_loop();
            return 0;
        }

        void start_main_loop() {

            boost::asio::co_spawn(
                    ioc_,
                    [this, self = shared_from_this()]() {
                        return co_main_loop(self);
                    },
                    [this, self = shared_from_this()](std::exception_ptr e, bool r) {

                        if (!e) {
                            if (r) {
                                BOOST_LOG_OWL(trace) << "ImGuiServiceImpl run() ok";
                                clear();
                                safe_exit();
                                return;
                            } else {
                                BOOST_LOG_OWL(trace) << "ImGuiServiceImpl run() error";
                                clear();
                                safe_exit();
                                return;
                            }
                        } else {
                            std::string what;
                            // https://stackoverflow.com/questions/14232814/how-do-i-make-a-call-to-what-on-stdexception-ptr
                            try { std::rethrow_exception(std::move(e)); }
                            catch (const std::exception &e) {
                                BOOST_LOG_OWL(error) << "ImGuiServiceImpl co_spawn catch std::exception "
                                                     << e.what();
                                what = e.what();
                            }
                            catch (const std::string &e) {
                                BOOST_LOG_OWL(error) << "ImGuiServiceImpl co_spawn catch std::string " << e;
                                what = e;
                            }
                            catch (const char *e) {
                                BOOST_LOG_OWL(error) << "ImGuiServiceImpl co_spawn catch char *e " << e;
                                what = std::string{e};
                            }
                            catch (...) {
                                BOOST_LOG_OWL(error) << "ImGuiServiceImpl co_spawn catch (...)"
                                                     << "\n" << boost::current_exception_diagnostic_information();
                                what = boost::current_exception_diagnostic_information();
                            }
                            BOOST_LOG_OWL(error) << "ImGuiServiceImpl::process_tag_info " << what;

                            clear();
                            safe_exit();
                        }

                    });
        }

    private:


        bool show_demo_window = false;
        bool show_about_window = false;
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
                    | ImGuiTableFlags_ScrollY
                    | ImGuiTableFlags_SizingFixedFit;
            int freeze_cols = 1;
            int freeze_rows = 1;
        };
        TableConfig table_config;

        std::string CopyrightString;

        boost::asio::awaitable<bool> co_main_loop(boost::shared_ptr<ImGuiServiceImpl> self) {
            boost::ignore_unused(self);

            BOOST_LOG_OWL(trace_cmd_tag) << "co_main_loop start";

            try {
                auto executor = co_await boost::asio::this_coro::executor;

                ImGuiIO &io = ImGui::GetIO();

                for (;;) {
//                    BOOST_LOG_OWL(trace) << "ImGuiServiceImpl co_main_loop loop";
                    SDL_Event event;
                    while (SDL_PollEvent(&event)) {
                        ImGui_ImplSDL2_ProcessEvent(&event);
                        if (event.type == SDL_QUIT) {
                            co_return true;
                        }
                        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                            event.window.windowID == SDL_GetWindowID(window)) {
                            co_return true;
                        }
                        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED &&
                            event.window.windowID == SDL_GetWindowID(window)) {
                            // Release all outstanding references to the swap chain's buffers before resizing.
                            CleanupRenderTarget();
                            g_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
                            CreateRenderTarget();
                        }
                    }

                    // Start the Dear ImGui frame
                    ImGui_ImplDX11_NewFrame();
                    ImGui_ImplSDL2_NewFrame();
                    ImGui::NewFrame();

                    if (show_demo_window) {
                        ImGui::ShowDemoWindow(&show_demo_window);
                    }

                    {
                        if (gui_window_flags == 0) {
                            gui_window_flags |= ImGuiWindowFlags_NoMove;
                            gui_window_flags |= ImGuiWindowFlags_MenuBar;
                            gui_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
                            gui_window_flags |= ImGuiWindowFlags_NoDecoration;
                            gui_window_flags |= ImGuiWindowFlags_NoSavedSettings;
                        }

                        const ImGuiViewport *main_viewport = ImGui::GetMainViewport();

                        ImGui::SetNextWindowPos(main_viewport->WorkPos);
                        ImGui::SetNextWindowSize(main_viewport->WorkSize);

                        if (ImGui::Begin("MainWindow", &open, gui_window_flags)) {
                            if (ImGui::BeginMenuBar()) {
                                if (ImGui::BeginMenu("菜单")) {
                                    ImGui::MenuItem("关于(About)", nullptr, &show_about_window);
                                    if (ImGui::MenuItem("退出", "Alt+F4")) {
                                        co_return true;
                                    }
                                    ImGui::EndMenu();
                                }
                                if (ImGui::BeginMenu("主题配色(Theme)")) {
                                    if (ImGui::MenuItem("夜间配色(Dark)")) {
                                        ImGui::StyleColorsDark();
                                    }
                                    if (ImGui::MenuItem("明亮配色(Light)")) {
                                        ImGui::StyleColorsLight();
                                    }
                                    if (ImGui::MenuItem("经典配色(Classic)")) {
                                        ImGui::StyleColorsClassic();
                                    }
                                    ImGui::EndMenu();
                                }
                                ImGui::EndMenuBar();
                            }

//                            ImGui::Text("有些有用的文字");
//                            ImGui::SameLine();
//                            ImGui::Text("有些有用的文字");


                            if (ImGui::Button("主动发现(Query)")) {
                                BOOST_LOG_OWL(trace) << R"((ImGui::Button("Query")))";
                                auto p = parentPtr_.lock();
                                if (p) {
                                    BOOST_LOG_OWL(trace) << R"(p->sendQuery())";
                                    p->sendQuery();
                                }
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("全部停止(EmergencyStop)")) {
                                do_all(OwlMailDefine::ControlCmd::stop);
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("全部降落(Land)")) {
                                do_all(OwlMailDefine::ControlCmd::land);
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("清空列表")) {
                                cleanItem();
                            }

                            const float footer_height_to_reserve =
                                    ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();

                            if constexpr (true) {
                                ImGui::BeginChild("AddrList", ImVec2(0, -footer_height_to_reserve),
                                                  true, ImGuiWindowFlags_HorizontalScrollbar);

                                auto &accRc = items.get<DiscoverStateItemContainerRandomAccess>();
                                if (accRc.empty()) {
                                    ImGui::Text("空列表");
                                } else {
                                    if (ImGui::BeginTable("AddrTable", 10,
                                                          table_config.table_flags,
                                                          ImVec2(0, 0))) {

                                        ImGui::TableSetupColumn("I", ImGuiTableColumnFlags_NoSort |
                                                                     ImGuiTableColumnFlags_WidthFixed |
                                                                     ImGuiTableColumnFlags_NoHide, 0.0f);
                                        ImGui::TableSetupColumn("IP", ImGuiTableColumnFlags_WidthFixed, 0.0f);
                                        ImGui::TableSetupColumn("PORT", ImGuiTableColumnFlags_WidthFixed, 0.0f);
                                        ImGui::TableSetupColumn("上次上线间隔", ImGuiTableColumnFlags_NoSort |
                                                                                ImGuiTableColumnFlags_WidthFixed, 0.0f);
                                        ImGui::TableSetupColumn("降落", ImGuiTableColumnFlags_NoSort |
                                                                        ImGuiTableColumnFlags_WidthFixed, 0.0f);
                                        ImGui::TableSetupColumn("停止", ImGuiTableColumnFlags_NoSort |
                                                                        ImGuiTableColumnFlags_WidthFixed, 0.0f);
                                        ImGui::TableSetupColumn("删除", ImGuiTableColumnFlags_NoSort |
                                                                        ImGuiTableColumnFlags_WidthFixed, 0.0f);
                                        ImGui::TableSetupColumn("校准", ImGuiTableColumnFlags_NoSort |
                                                                        ImGuiTableColumnFlags_WidthFixed, 0.0f);
                                        ImGui::TableSetupColumn("第一次发现时间", ImGuiTableColumnFlags_NoSort |
                                                                                  ImGuiTableColumnFlags_WidthFixed,
                                                                0.0f);
                                        ImGui::TableSetupColumn("上次上线时间", ImGuiTableColumnFlags_NoSort |
                                                                                ImGuiTableColumnFlags_WidthFixed, 0.0f);
                                        ImGui::TableSetupScrollFreeze(table_config.freeze_cols,
                                                                      table_config.freeze_rows);

                                        ImGui::TableHeadersRow();

                                        for (size_t i = 0; i < accRc.size(); ++i) {
                                            auto &n = accRc.at(i);
                                            ImGui::TableNextRow();
                                            ImGui::TableNextColumn();
                                            ImGui::Text(boost::lexical_cast<std::string>(i).c_str());
                                            ImGui::TableNextColumn();
                                            ImGui::Text(n.ip.c_str());
                                            ImGui::TableNextColumn();
                                            ImGui::Text(boost::lexical_cast<std::string>(n.port).c_str());
                                            ImGui::TableNextColumn();
                                            ImGui::Text(n.nowDuration().c_str());
                                            ImGui::TableNextColumn();
                                            if (ImGui::SmallButton(("Land##" + n.ip).c_str())) {
                                                do_ip(OwlMailDefine::ControlCmd::land, n.ip);
                                            }
                                            ImGui::TableNextColumn();
                                            if (ImGui::SmallButton(("Stop##" + n.ip).c_str())) {
                                                do_ip(OwlMailDefine::ControlCmd::stop, n.ip);
                                            }
                                            ImGui::TableNextColumn();
                                            if (ImGui::SmallButton(("Delete##" + n.ip).c_str())) {
                                                BOOST_LOG_OWL(trace) << R"((ImGui::Button("Delete")) )" << n.ip;
                                                deleteItem(n.ip);
                                            }
                                            ImGui::TableNextColumn();
                                            if (ImGui::SmallButton(("Calibrate##" + n.ip).c_str())) {
                                                do_ip(OwlMailDefine::ControlCmd::calibrate, n.ip);
                                            }
                                            ImGui::TableNextColumn();
                                            ImGui::Text(n.cacheFirstTime.c_str());
                                            ImGui::TableNextColumn();
                                            ImGui::Text(n.cacheLastTime.c_str());
                                        }
                                        ImGui::EndTable();
                                    }
                                }

                                ImGui::EndChild();
                            }

                            ImGui::Text(u8" %.3f ms/frame (%.1f FPS)"_C, 1000.0f / io.Framerate,
                                        io.Framerate);

                            ImGui::End();
                        }
                    }


                    if (show_about_window) {
                        ImGui::Begin("关于", &show_about_window, ImGuiWindowFlags_AlwaysAutoResize);
                        ImGui::Text(CopyrightString.c_str());
                        ImGui::Text("广州市鑫广飞信息科技有限公司 版权所有 2023");
                        ImGui::Text("");
                        if (ImGui::Button("关闭"))
                            show_about_window = false;
                        ImGui::End();
                    }


                    // Rendering
                    ImGui::Render();
                    ImGui::EndFrame();
                    const float clear_color_with_alpha[4] = {clear_color.x * clear_color.w,
                                                             clear_color.y * clear_color.w,
                                                             clear_color.z * clear_color.w, clear_color.w};
                    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
                    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
                    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());




                    // ================================ ............ ================================
                    // make a break for boost asio thread can handle another event
                    co_await boost::asio::post(executor, use_awaitable);
                    // ================================ ............ ================================

                    g_pSwapChain->Present(1, 0); // Present with vsync
                    //g_pSwapChain->Present(0, 0); // Present without vsync

                }

            } catch (const std::exception &e) {
                BOOST_LOG_OWL(error) << "co_process_tag_info catch (const std::exception &e)" << e.what();
                throw;
            }

            boost::ignore_unused(self);
            co_return true;

        }


    };

    ImGuiService::ImGuiService(
            boost::asio::io_context &ioc,
            boost::shared_ptr<OwlConfigLoader::ConfigLoader> config,
            OwlMailDefine::ControlImGuiMailBox &&mailbox_ig,
            OwlMailDefine::ControlMulticastMailbox &&mailbox_mc,
            OwlMailDefine::ControlUdpMailBox &&mailbox_udp,
            OwlMailDefine::ControlHttpMailBox &&mailbox_http
    ) : ioc_(ioc),
        config_(std::move(config)),
        mailbox_mc_(mailbox_mc),
        mailbox_ig_(mailbox_ig),
        mailbox_udp_(mailbox_udp),
        mailbox_http_(mailbox_http) {

        mailbox_ig_->receiveA2B([this](OwlMailDefine::MailControl2ImGui &&data) {
            boost::asio::dispatch(ioc_, [this, data, self = shared_from_this()]() {
                auto m = boost::make_shared<OwlMailDefine::MailImGui2Control::element_type>();
                m->runner = data->callbackRunner;

                BOOST_ASSERT(impl);
                if (data->state) {
                    impl->new_state(data->state);
                } else {
                }

                mailbox_ig_->sendB2A(std::move(m));
            });
        });

        mailbox_mc_->receiveB2A([this](OwlMailDefine::MailMulticast2Control &&data) {
            if (data->runner) {
                data->runner(data);
            }
        });
    }

    void ImGuiService::start() {
        impl = boost::make_shared<ImGuiServiceImpl>(ioc_, weak_from_this());
        impl->start();
    }

    void ImGuiService::clear() {
        impl->clear();
    }

    void ImGuiService::sendQuery() {
        boost::asio::post(ioc_, [this, self = shared_from_this()]() {
            auto m = boost::make_shared<OwlMailDefine::MailControl2Multicast::element_type>();

            m->cmd = OwlMailDefine::MulticastCmd::query;
            m->callbackRunner = [](OwlMailDefine::MailMulticast2Control &&data) {
                // ignore
                boost::ignore_unused(data);
            };

            mailbox_mc_->sendA2B(std::move(m));
        });
    }

    void ImGuiService::sendCmdUdp(const boost::shared_ptr<OwlMailDefine::ControlCmdData>& data) {
        auto m = boost::make_shared<OwlMailDefine::MailControl2UdpControl::element_type>();

        BOOST_ASSERT(data);
        m->controlCmdData = data;

        m->callbackRunner = [](OwlMailDefine::MailUdpControl2Control &&d) {
            // ignore
        };

        mailbox_udp_->sendA2B(std::move(m));
    }

    void ImGuiService::sendCmdHttp(const boost::shared_ptr<OwlMailDefine::ControlCmdData>& data) {
        auto m = boost::make_shared<OwlMailDefine::MailControl2HttpControl ::element_type>();

        BOOST_ASSERT(data);
        m->controlCmdData = data;

        m->callbackRunner = [](OwlMailDefine::MailHttpControl2Control &&d) {
            // ignore
        };

        mailbox_http_->sendA2B(std::move(m));
    }

} // OwlImGuiService