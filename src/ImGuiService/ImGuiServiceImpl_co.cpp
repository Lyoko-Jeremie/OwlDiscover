// jeremie


#include "ImGuiServiceImpl.h"

#include "./ImGuiService.h"
#include <boost/asio/co_spawn.hpp>
#include <boost/exception_ptr.hpp>

using boost::asio::use_awaitable;
#if defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
# define use_awaitable \
  boost::asio::use_awaitable_t(__FILE__, __LINE__, __PRETTY_FUNCTION__)
#endif

namespace OwlImGuiServiceImpl {

    int ImGuiServiceImpl::init() {

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
                OwlImGuiService::safe_exit();
                return -2;
            }
            BOOST_LOG_OWL(info) << a->GetDebugName() << " " << a << " " << a->IsLoaded();
            a->ContainerAtlas->Build();
        }
        io.FontDefault = fontsList.at(0);


        init_test_cmd_data();

        // now start loop
        start_main_loop();
        return 0;
    }


    void ImGuiServiceImpl::start_main_loop() {

        boost::asio::co_spawn(
                ioc_,
                [this, self = shared_from_this()]() {
                    return co_main_loop(self);
                },
                [this, self = shared_from_this()](std::exception_ptr e, bool r) {

                    if (!e) {
                        if (r) {
                            BOOST_LOG_OWL(trace_gui) << "ImGuiServiceImpl run() ok";
                            clear();
                            OwlImGuiService::safe_exit();
                            return;
                        } else {
                            BOOST_LOG_OWL(error) << "ImGuiServiceImpl run() error";
                            clear();
                            OwlImGuiService::safe_exit();
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
                        OwlImGuiService::safe_exit();
                    }

                });
    }


    boost::asio::awaitable<bool> ImGuiServiceImpl::co_main_loop(boost::shared_ptr<ImGuiServiceImpl> self) {
        boost::ignore_unused(self);

        BOOST_LOG_OWL(trace_cmd_tag) << "co_main_loop start";

        try {
            auto executor = co_await boost::asio::this_coro::executor;

            ImGuiIO &io = ImGui::GetIO();

            for (;;) {
//                    BOOST_LOG_OWL(trace_gui) << "ImGuiServiceImpl co_main_loop loop";
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
                            if (ImGui::BeginMenu("测试")) {
                                ImGui::MenuItem("控制测试", nullptr, &show_test_cmd_window);
//                                ImGui::MenuItem("相机测试", nullptr, &show_camera_window);
                                ImGui::EndMenu();
                            }
                            ImGui::EndMenuBar();
                        }


                        if (ImGui::Button("主动发现(Multicast Query)")) {
                            auto p = parentPtr_.lock();
                            if (p) {
                                p->sendQuery();
                            }
                        }
                        ImGui::SameLine();
                        HelpMarker(
                                "使用Multicast组播功能主动搜索未知设备，\n需要(6.1.11.23.03.23d.e2b087f8)以上固件支持。\n可能被限速或屏蔽，请勿频繁点击发送"
                        );
                        ImGui::SameLine();
                        if (ImGui::Button("广播发现(Broadcast Query)")) {
                            auto p = parentPtr_.lock();
                            if (p) {
                                p->sendBroadcast();
                            }
                        }
//                            if (ImGui::IsItemHovered()) {
//                                ImGui::SetTooltip(
//                                        "使用Broadcast广播功能主动搜索，对固件版本无要求");
//                            }
                        ImGui::SameLine();
                        HelpMarker(
                                "使用Broadcast广播功能主动搜索未知设备，对固件版本无要求。\n对网络影响较大，可能被限速或屏蔽，请勿频繁点击发送。"
                        );
                        ImGui::SameLine();
                        if (ImGui::Button("全部查询(Unicast Query)")) {
                            do_all(OwlMailDefine::ControlCmd::query);
                        }
                        ImGui::SameLine();
                        HelpMarker("查询并刷新已列出设备是否在线\n需要(6.1.11.23.03.23d.e2b087f8)以上固件支持。");

                        if (ImGui::Button("扫描整个内网(Http Scan)")) {
                            scanSubnet();
                        }
                        ImGui::SameLine();
                        HelpMarker(
                                "使用HTTP逐个扫描内网IP，扫描速度很慢，对网络影响很大，"
                        );
                        ImGui::SameLine();
                        if (ImGui::Button("Http Ping扫描整个内网(Http Ping Scan)")) {
                            scanSubnetPingHttp();
                        }
                        ImGui::SameLine();
                        HelpMarker(
                                "使用HTTP-Ping逐个扫描内网IP，扫描速度很慢，对网络影响很大，"
                        );
                        ImGui::SameLine();
                        if (ImGui::Button("Udp Ping扫描整个内网(Udp Ping Scan)")) {
                            scanSubnetPingUdp();
                        }
                        ImGui::SameLine();
                        HelpMarker(
                                "使用Udp-Ping逐个扫描内网IP，扫描速度很慢，对网络影响很大，"
                        );
                        ImGui::SameLine();
                        if (ImGui::Button("读取OTA版本")) {
                            sendCmdHttpReadAllOTA();
                        }


                        if (ImGui::Button("全部停止(EmergencyStop)")) {
                            do_all(OwlMailDefine::ControlCmd::stop);
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("紧急停桨");
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("全部降落(Land)")) {
                            do_all(OwlMailDefine::ControlCmd::land);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("全部测试(Ping)")) {
                            do_all(OwlMailDefine::ControlCmd::ping);
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("测试所有已列出设备是否在线，并检查网络延迟，对固件版本无要求");
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("全部校准(Calibrate)")) {
                            do_all(OwlMailDefine::ControlCmd::calibrate);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("清空列表")) {
                            cleanItem();
                        }
                        ImGui::SameLine();
                        ImGui::Text("指令模式：");
                        ImGui::SameLine();
                        ImGui::RadioButton("UDP", &cmdType, static_cast<int>(CmdType::Udp));
                        ImGui::SameLine();
                        ImGui::RadioButton("HTTP", &cmdType, static_cast<int>(CmdType::Http));
                        ImGui::SameLine();
                        HelpMarker(
                                "选择控制指令所使用的模式。\nHTTP模式更可靠但更慢，网络不稳定时更加明显。\nUDP在不稳定的网络下可能控制失败，需要多次点击发送指令。"
                        );

                        const float footer_height_to_reserve =
                                ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();

                        if constexpr (true) {
                            ImGui::BeginChild("AddrList", ImVec2(0, -footer_height_to_reserve),
                                              true, ImGuiWindowFlags_HorizontalScrollbar);

                            auto &accRc = items.get<DiscoverStateItemContainerSequencedAccess>();
                            auto &accT = timeoutInfo.get<OwlDiscoverState::PackageSendInfo::IP>();
                            if (accRc.empty()) {
                                ImGui::Text("空列表");
                            } else {
                                int col = 18;
                                if (ImGui::BeginTable("AddrTable", col,
                                                      table_config.table_flags,
                                                      ImVec2(0, 0))) {

                                    ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_NoSort |
                                                                 ImGuiTableColumnFlags_WidthFixed |
                                                                 ImGuiTableColumnFlags_NoHide, 0.0f);
                                    ImGui::TableSetupColumn("IP", ImGuiTableColumnFlags_NoSort |
                                                                  ImGuiTableColumnFlags_WidthFixed |
                                                                  ImGuiTableColumnFlags_NoHide, 0.0f);
                                    ImGui::TableSetupColumn("PORT", ImGuiTableColumnFlags_NoSort |
                                                                    ImGuiTableColumnFlags_WidthFixed, 0.0f);
                                    ImGui::TableSetupColumn("Timeout", ImGuiTableColumnFlags_NoSort |
                                                                       ImGuiTableColumnFlags_WidthFixed, 0.0f);
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
                                    ImGui::TableSetupColumn("联通性测试", ImGuiTableColumnFlags_NoSort |
                                                                          ImGuiTableColumnFlags_WidthFixed,
                                                            0.0f);
                                    ImGui::TableSetupColumn("查询", ImGuiTableColumnFlags_NoSort |
                                                                    ImGuiTableColumnFlags_WidthFixed, 0.0f);
                                    ImGui::TableSetupColumn("第一次发现时间", ImGuiTableColumnFlags_NoSort |
                                                                              ImGuiTableColumnFlags_WidthFixed,
                                                            0.0f);
                                    ImGui::TableSetupColumn("上次上线时间", ImGuiTableColumnFlags_NoSort |
                                                                            ImGuiTableColumnFlags_WidthFixed, 0.0f);
                                    ImGui::TableSetupColumn("ProgramVersion", ImGuiTableColumnFlags_NoSort |
                                                                              ImGuiTableColumnFlags_WidthFixed,
                                                            0.0f);
                                    ImGui::TableSetupColumn("ProgramGitRev", ImGuiTableColumnFlags_NoSort |
                                                                             ImGuiTableColumnFlags_WidthFixed,
                                                            0.0f);
                                    ImGui::TableSetupColumn("ProgramBuildTime", ImGuiTableColumnFlags_NoSort |
                                                                                ImGuiTableColumnFlags_WidthFixed,
                                                            0.0f);
                                    ImGui::TableSetupColumn("OTAVersion", ImGuiTableColumnFlags_NoSort |
                                                                          ImGuiTableColumnFlags_WidthFixed,
                                                            0.0f);
                                    ImGui::TableSetupColumn("相机测试", ImGuiTableColumnFlags_NoSort |
                                                                        ImGuiTableColumnFlags_WidthFixed,
                                                            0.0f);
                                    ImGui::TableSetupScrollFreeze(table_config.freeze_cols,
                                                                  table_config.freeze_rows);

                                    ImGui::TableHeadersRow();

                                    auto itN = accRc.begin();
                                    size_t i = 0;
                                    for (; i < accRc.size(); ++itN, ++i) {
                                        auto &n = *itN;
                                        ImGui::TableNextRow();
                                        ImGui::TableNextColumn();
                                        ImGui::Text(boost::lexical_cast<std::string>(i).c_str());
                                        ImGui::TableNextColumn();
                                        ImGui::Text(n.ip.c_str());
                                        ImGui::TableNextColumn();
                                        ImGui::Text(boost::lexical_cast<std::string>(n.port).c_str());
                                        ImGui::TableNextColumn();
                                        {
                                            auto nn = accT.find(n.ip);
                                            if (nn != accT.end()) {
                                                ImGui::Text(boost::lexical_cast<std::string>(nn->msDelay).c_str());
                                            } else {
                                                ImGui::Text("0");
                                            }
                                            ImGui::TableNextColumn();
                                        }
                                        ImGui::Text(n.nowDuration().c_str());
                                        if (ImGui::IsItemHovered()) {
                                            ImGui::SetTooltip("最近一次在线时间是%s秒前", n.nowDuration().c_str());
                                        }
                                        ImGui::TableNextColumn();
                                        if (ImGui::SmallButton(("Land##Land" + n.ip).c_str())) {
                                            do_ip(OwlMailDefine::ControlCmd::land, n.ip);
                                        }
                                        ImGui::TableNextColumn();
                                        if (ImGui::SmallButton(("Stop##Stop" + n.ip).c_str())) {
                                            do_ip(OwlMailDefine::ControlCmd::stop, n.ip);
                                        }
                                        if (ImGui::IsItemHovered()) {
                                            ImGui::SetTooltip("紧急停桨");
                                        }
                                        ImGui::TableNextColumn();
                                        if (ImGui::SmallButton(("Delete##Delete" + n.ip).c_str())) {
                                            deleteItem(n.ip);
                                        }
                                        if (ImGui::IsItemHovered()) {
                                            ImGui::SetTooltip("从列表中删除");
                                        }
                                        ImGui::TableNextColumn();
                                        if (ImGui::SmallButton(("Calibrate##Calibrate" + n.ip).c_str())) {
                                            do_ip(OwlMailDefine::ControlCmd::calibrate, n.ip);
                                        }
                                        ImGui::TableNextColumn();
                                        if (ImGui::SmallButton(("Ping##Ping" + n.ip).c_str())) {
                                            do_ip(OwlMailDefine::ControlCmd::ping, n.ip);
                                        }
                                        if (ImGui::IsItemHovered()) {
                                            ImGui::SetTooltip("测试此设备是否在线，并检查网络延迟");
                                        }
                                        ImGui::TableNextColumn();
                                        if (ImGui::SmallButton(("Query##" + n.ip).c_str())) {
                                            do_ip(OwlMailDefine::ControlCmd::query, n.ip);
                                        }
                                        ImGui::TableNextColumn();
                                        ImGui::Text(n.cacheFirstTime.c_str());
                                        ImGui::TableNextColumn();
                                        ImGui::Text(n.cacheLastTime.c_str());
                                        ImGui::TableNextColumn();
                                        ImGui::Text(n.programVersion.c_str());
                                        ImGui::TableNextColumn();
                                        ImGui::Text(n.gitRev.c_str());
                                        ImGui::TableNextColumn();
                                        ImGui::Text(n.buildTime.c_str());
                                        ImGui::TableNextColumn();
                                        if (n.versionOTA.empty()) {
                                            if (ImGui::SmallButton(("读取OTA版本##read_ota" + n.ip).c_str())) {
                                                sendCmdHttpReadOTA(n.ip);
                                            }
                                        } else {
                                            ImGui::Text(n.versionOTA.c_str());
                                        }
                                        ImGui::TableNextColumn();
                                        if (!*(n.showCamera)) {
                                            if (ImGui::SmallButton(("相机测试##camera_test_f" + n.ip).c_str())) {
                                                *(n.showCamera) = true;
                                            }
                                        } else {
                                            if (ImGui::SmallButton(("相机测试##camera_test_t" + n.ip).c_str())) {
                                                *(n.showCamera) = false;
                                            }
                                        }
                                    }
                                    ImGui::EndTable();
                                }
                            }

                            ImGui::EndChild();
                        }

                        ImGui::Text(u8" %.3f ms/frame (%.1f FPS)"_C, 1000.0f / io.Framerate,
                                    io.Framerate);
                        ImGui::SameLine();
                        ImGui::Text("     ");
                        ImGui::SameLine();
                        ImGui::Text("当前时间：%s", OwlDiscoverState::DiscoverStateItem::nowTimeString().c_str());

                        ImGui::End();
                    }
                }


                if (show_about_window) {
                    ImGui::Begin("关于", &show_about_window, ImGuiWindowFlags_AlwaysAutoResize);
                    ImGui::Text(OwlLog::allCopyrightVersionString.c_str());
                    ImGui::Text("广州市鑫广飞信息科技有限公司 版权所有 2023");
                    ImGui::Text("  ");
                    if (ImGui::Button("关闭"))
                        show_about_window = false;
                    ImGui::End();
                }

                /*if (show_camera_window)*/ {
                    auto &accRc = items.get<DiscoverStateItemContainerSequencedAccess>();
                    for (const auto &a: accRc) {
                        if (*(a.showCamera)) {
                            show_camera(a);
                        } else {
                            // clean camera image data
                            if (a.imgDataFont->texture) {
                                a.imgDataFont->cleanTexture();
                            }
                            if (a.imgDataDown->texture) {
                                a.imgDataDown->cleanTexture();
                            }
                        }
                    }
                }

                if (show_test_cmd_window) {
                    show_test_cmd();
                }

                if (show_state_window) {
                    show_debug_state();
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


} // OwlImGuiServiceImpl

