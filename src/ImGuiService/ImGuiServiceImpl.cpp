// jeremie

#include "ImGuiServiceImpl.h"
#include "./ImGuiService.h"

namespace OwlImGuiServiceImpl {

    std::string operator "" _S(const char8_t *str, std::size_t) {
        return reinterpret_cast< const char * >(str);
    }

    char const *operator "" _C(const char8_t *str, std::size_t) {
        return reinterpret_cast< const char * >(str);
    }


    bool ImGuiServiceImpl::CreateDeviceD3D(HWND hWnd) {
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

    void ImGuiServiceImpl::CleanupDeviceD3D() {
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

    void ImGuiServiceImpl::CreateRenderTarget() {
        ID3D11Texture2D *pBackBuffer;
        g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }

    void ImGuiServiceImpl::CleanupRenderTarget() {
        if (g_mainRenderTargetView) {
            g_mainRenderTargetView->Release();
            g_mainRenderTargetView = nullptr;
        }
    }

    void ImGuiServiceImpl::clear() {

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

    void ImGuiServiceImpl::update_state(const boost::shared_ptr<OwlDiscoverState::DiscoverStateItem> &a) {
        BOOST_ASSERT(a);
        BOOST_ASSERT(!weak_from_this().expired());

        boost::asio::dispatch(ioc_, [this, a, self = shared_from_this()]() {
            BOOST_ASSERT(a);
            BOOST_ASSERT(self);

            auto &accIp = items.get<OwlDiscoverState::DiscoverStateItem::IP>();
            auto accIpEnd = accIp.end();
            BOOST_LOG_OWL(trace_gui) << "ImGuiServiceImpl::new_state DiscoverStateItem a " << a->ip;

            auto it = accIp.find(a->ip);
            if (it != accIpEnd) {
                // update
                accIp.modify(it, [&a](OwlDiscoverState::DiscoverStateItem &n) {
                    n.lastTime = a->lastTime;
                    n.port = a->port;
                    if (!a->programVersion.empty()) {
                        n.programVersion = a->programVersion;
                    }
                    if (!a->gitRev.empty()) {
                        n.gitRev = a->gitRev;
                    }
                    if (!a->buildTime.empty()) {
                        n.buildTime = a->buildTime;
                    }
                    n.updateCache();
                });
            } else {
                // insert
                // ignore
            }

            // re short it
            sortItem();
            remove_old_package_send_info();

        });
    }

    void ImGuiServiceImpl::update_ota(std::string ip, std::string version) {

        boost::asio::dispatch(ioc_, [this, ip, version, self = shared_from_this()]() {
            BOOST_ASSERT(self);

            auto &accIp = items.get<OwlDiscoverState::DiscoverStateItem::IP>();
            auto accIpEnd = accIp.end();
            BOOST_LOG_OWL(trace_gui) << "ImGuiServiceImpl::update_ota " << ip << " " << version;

            auto it = accIp.find(ip);
            if (it != accIpEnd) {
                // update
                accIp.modify(it, [&ip, &version](OwlDiscoverState::DiscoverStateItem &n) {
                    boost::ignore_unused(ip);
                    if (!version.empty()) {
                        n.versionOTA = version;
                    }
                    n.updateCache();
                });
            } else {
                // insert
                // ignore
            }

            // re short it
            // sortItem();
            remove_old_package_send_info();

        });
    }

    void ImGuiServiceImpl::new_state(const boost::shared_ptr<OwlDiscoverState::DiscoverStateItem> &a) {
        BOOST_ASSERT(a);
        BOOST_ASSERT(!weak_from_this().expired());

        boost::asio::dispatch(ioc_, [this, a, self = shared_from_this()]() {
            BOOST_ASSERT(a);
            BOOST_ASSERT(self);

            auto &accIp = items.get<OwlDiscoverState::DiscoverStateItem::IP>();
            auto accIpEnd = accIp.end();
            BOOST_LOG_OWL(trace_gui) << "ImGuiServiceImpl::new_state DiscoverStateItem a " << a->ip;

            auto it = accIp.find(a->ip);
            if (it != accIpEnd) {
                // update
                accIp.modify(it, [&a](OwlDiscoverState::DiscoverStateItem &n) {
                    n.lastTime = a->lastTime;
                    n.port = a->port;
                    if (!a->programVersion.empty()) {
                        n.programVersion = a->programVersion;
                    }
                    if (!a->gitRev.empty()) {
                        n.gitRev = a->gitRev;
                    }
                    if (!a->buildTime.empty()) {
                        n.buildTime = a->buildTime;
                    }
                    n.updateCache();
                });
            } else {
                // insert
                auto b = *a;
                b.updateCache();
                items.emplace_back(b);
            }

            // re short it
            sortItem();
            remove_old_package_send_info();

        });

    }

    void ImGuiServiceImpl::new_state(const boost::shared_ptr<OwlDiscoverState::DiscoverState> &s) {
        BOOST_ASSERT(s);
        BOOST_ASSERT(!weak_from_this().expired());

        boost::asio::dispatch(ioc_, [this, s, self = shared_from_this()]() {
            BOOST_ASSERT(s);
            BOOST_ASSERT(self);

            auto &accIp = items.get<OwlDiscoverState::DiscoverStateItem::IP>();
            auto accIpEnd = accIp.end();
            for (const auto &a: s->items) {
                BOOST_LOG_OWL(trace_gui) << "ImGuiServiceImpl::new_state DiscoverState a " << a.ip;

                auto it = accIp.find(a.ip);
                if (it != accIpEnd) {
                    // update
                    accIp.modify(it, [&a](OwlDiscoverState::DiscoverStateItem &n) {
                        n.lastTime = a.lastTime;
                        n.port = a.port;
                        if (!a.programVersion.empty()) {
                            n.programVersion = a.programVersion;
                        }
                        if (!a.gitRev.empty()) {
                            n.gitRev = a.gitRev;
                        }
                        if (!a.buildTime.empty()) {
                            n.buildTime = a.buildTime;
                        }
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
            remove_old_package_send_info();

        });

    }

    void ImGuiServiceImpl::remove_old_package_send_info() {
        BOOST_ASSERT(!weak_from_this().expired());

        auto &acc = timeoutInfo.get<OwlDiscoverState::PackageSendInfo::SequencedAccess>();
        auto &accIp = items.get<OwlDiscoverState::DiscoverStateItem::IP>();
        auto it = acc.begin();
        while (it != acc.end()) {
            if (accIp.find(it->ip) == accIp.end()) {
                // remove not valid item
                it = acc.erase(it);
                continue;
            }
            if (it->needUpdateDelay()) {
                // update delay
                acc.modify(it, [](OwlDiscoverState::PackageSendInfo &a) { ;
                    a.updateDelay();
                });
            }
            if (it->msDelay > 1000 * 60 * 2) {
                // remove timeout>1m item
                it = acc.erase(it);
                continue;
            } else {
                ++it;
                continue;
            }
        }

    }

    void ImGuiServiceImpl::update_package_send_info(const boost::shared_ptr<OwlDiscoverState::PackageSendInfo> &s) {
        BOOST_ASSERT(s);
        BOOST_ASSERT(!weak_from_this().expired());

        boost::asio::dispatch(ioc_, [this, s, self = shared_from_this()]() {
            BOOST_ASSERT(s);
            BOOST_ASSERT(self);

            auto &acc = timeoutInfo.get<OwlDiscoverState::PackageSendInfo::PKG>();
            const auto &it = acc.find(OwlDiscoverState::PackageSendInfo::PKG::make_tuple(*s));
            if (it == acc.end()) {
                if (s->direct == OwlDiscoverState::PackageSendInfoDirectEnum::in) {
                    BOOST_LOG_OWL(warning) << "ImGuiServiceImpl::update_package_send_info a in package nto have out ip "
                                           << s->ip
                                           << " cmdId " << s->cmdId
                                           << " packageId " << s->packageId
                                           << " clientId " << s->clientId;
                    // need have out package record first
                    // ignore
                    return;
                } else {
                    // a new out package, record it
                    acc.insert(*s);
                    BOOST_LOG_OWL(trace_gui) << "ImGuiServiceImpl::update_package_send_info new ip "
                                             << s->ip
                                             << " cmdId " << s->cmdId
                                             << " packageId " << s->packageId
                                             << " clientId " << s->clientId;
                    remove_old_package_send_info();
                    return;
                }
            }
            if (s->direct == OwlDiscoverState::PackageSendInfoDirectEnum::state) {
                BOOST_LOG_OWL(warning) << "ImGuiServiceImpl::update_package_send_info network wrong ip "
                                       << s->ip
                                       << " cmdId " << s->cmdId
                                       << " packageId " << s->packageId
                                       << " clientId " << s->clientId;
                // this package are end
                // network wrong
                // ignore
                return;
            }
            if (s->direct == OwlDiscoverState::PackageSendInfoDirectEnum::out) {
                BOOST_LOG_OWL(warning) << "ImGuiServiceImpl::update_package_send_info duplicate package ip "
                                       << s->ip
                                       << " cmdId " << s->cmdId
                                       << " packageId " << s->packageId
                                       << " clientId " << s->clientId;
                // duplicate package
                // ignore
                return;
            }
            // update the inTime & delay & state
            BOOST_LOG_OWL(trace_gui) << "ImGuiServiceImpl::update_package_send_info update ip "
                                     << s->ip
                                     << " cmdId " << s->cmdId
                                     << " packageId " << s->packageId
                                     << " clientId " << s->clientId;
            acc.modify(it, [s](OwlDiscoverState::PackageSendInfo &a) { ;
                a.direct = OwlDiscoverState::PackageSendInfoDirectEnum::state;
                a.inTime = s->inTime;
                a.updateDelay();
            });
            remove_old_package_send_info();

        });

    }

    void ImGuiServiceImpl::sendCmdHttpReadAllOTA() {
        auto p = parentPtr_.lock();
        if (p) {
            auto &accIp = items.get<OwlDiscoverState::DiscoverStateItem::IP>();
            auto accIpEnd = accIp.end();
            for (auto it = accIp.begin(); it != accIpEnd; ++it) {
                p->sendCmdHttpReadOTA(it->ip);
            }
        }
    }

    void ImGuiServiceImpl::sendCmdHttpReadOTA(std::string ip) {
        auto p = parentPtr_.lock();
        if (p) {
            p->sendCmdHttpReadOTA(std::move(ip));
        }
    }

    void ImGuiServiceImpl::scanSubnet() {
        auto p = parentPtr_.lock();
        if (p) {
            p->scanSubnet();
        }
    }

    void ImGuiServiceImpl::scanSubnetPingHttp() {
        auto p = parentPtr_.lock();
        if (p) {
            p->scanSubnetPingHttp();
        }
    }

    void ImGuiServiceImpl::scanSubnetPingUdp() {
        auto p = parentPtr_.lock();
        if (p) {
            p->scanSubnetPingUdp();
        }
    }

    void ImGuiServiceImpl::do_all(OwlMailDefine::ControlCmd cmd) {
        auto p = parentPtr_.lock();
        if (p) {
            if (cmdType == static_cast<int>(CmdType::Udp)
                || cmd == OwlMailDefine::ControlCmd::query) {
                auto &accIp = items.get<OwlDiscoverState::DiscoverStateItem::IP>();
                auto accIpEnd = accIp.end();
                for (auto it = accIp.begin(); it != accIpEnd; ++it) {
                    auto m = boost::make_shared<OwlMailDefine::ControlCmdData>();
                    m->cmd = cmd;
                    m->ip = it->ip;
                    p->sendCmdUdp(std::move(m));
                }
                return;
            } else if (cmdType == static_cast<int>(CmdType::Http)) {
                auto &accIp = items.get<OwlDiscoverState::DiscoverStateItem::IP>();
                auto accIpEnd = accIp.end();
                for (auto it = accIp.begin(); it != accIpEnd; ++it) {
                    auto m = boost::make_shared<OwlMailDefine::ControlCmdData>();
                    m->cmd = cmd;
                    m->ip = it->ip;
                    p->sendCmdHttp(std::move(m));
                }
                return;
            } else {
                BOOST_LOG_OWL(error) << "ImGuiServiceImpl do_all never go there.";
                return;
            }
        }
    }

    void ImGuiServiceImpl::do_ip(OwlMailDefine::ControlCmd cmd, std::string ip) {
        auto p = parentPtr_.lock();
        if (p) {
            if (cmdType == static_cast<int>(CmdType::Udp)
                || cmd == OwlMailDefine::ControlCmd::query) {
                auto m = boost::make_shared<OwlMailDefine::ControlCmdData>();
                m->cmd = cmd;
                m->ip = std::move(ip);
                p->sendCmdUdp(std::move(m));
                return;
            } else if (cmdType == static_cast<int>(CmdType::Http)) {
                auto m = boost::make_shared<OwlMailDefine::ControlCmdData>();
                m->cmd = cmd;
                m->ip = std::move(ip);
                p->sendCmdHttp(std::move(m));
                return;
            } else {
                BOOST_LOG_OWL(error) << "ImGuiServiceImpl do_ip never go there.";
                return;
            }
        }
    }

    void ImGuiServiceImpl::sortItem() {
        sortItemByDuration30();
    }

    void ImGuiServiceImpl::sortItemByDuration30() {
        auto now = OwlDiscoverState::DiscoverStateItem::now();
        auto &accIp = items.get<DiscoverStateItemContainerSequencedAccess>();
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

    void ImGuiServiceImpl::sortItemByDuration() {
        auto now = OwlDiscoverState::DiscoverStateItem::now();
        auto &accIp = items.get<DiscoverStateItemContainerSequencedAccess>();
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

    void ImGuiServiceImpl::deleteItem(const std::string &s) {
        boost::asio::post(ioc_, [this, self = shared_from_this(), s = s]() {
            // must run in a isolate context, detach from GUI draw
            BOOST_LOG_OWL(trace_gui) << "deleteItem " << s;
            auto &accIp = items.get<OwlDiscoverState::DiscoverStateItem::IP>();
            auto n = accIp.erase(s);
            BOOST_LOG_OWL(trace_gui) << "deleteItem " << n;
        });
    }

    void ImGuiServiceImpl::cleanItem() {
        boost::asio::post(ioc_, [this, self = shared_from_this()]() {
            // must run in a isolate context, detach from GUI draw
            items.clear();
        });
    }

    void ImGuiServiceImpl::HelpMarker(const char *desc) {
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) && ImGui::BeginTooltip()) {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    void ImGuiServiceImpl::test_cmd_do_all(OwlMailDefine::ControlCmd cmd) {
        auto &accRc = items.get<DiscoverStateItemContainerSequencedAccess>();
        for (const auto &a: accRc) {
            if (*a.selected) {
                do_ip(cmd, a.ip);
            }
        }
    }

    void ImGuiServiceImpl::init_test_cmd_data() {
        testCmdList.emplace_back("起飞##cmd_takeoff", [this]() {
            test_cmd_do_all(OwlMailDefine::ControlCmd::takeoff);
        });
        testCmdList.emplace_back("降落##cmd_land", [this]() {
            test_cmd_do_all(OwlMailDefine::ControlCmd::land);
        });
        testCmdList.emplace_back("校准##cmd_calibrate", [this]() {
            test_cmd_do_all(OwlMailDefine::ControlCmd::calibrate);
        });
        testCmdList.emplace_back("前##cmd_forward", [this]() {
            test_cmd_do_all(OwlMailDefine::ControlCmd::forward);
        });
        testCmdList.emplace_back("后##cmd_back", [this]() {
            test_cmd_do_all(OwlMailDefine::ControlCmd::back);
        });
        testCmdList.emplace_back("左##cmd_left", [this]() {
            test_cmd_do_all(OwlMailDefine::ControlCmd::left);
        });
        testCmdList.emplace_back("右##cmd_right", [this]() {
            test_cmd_do_all(OwlMailDefine::ControlCmd::right);
        });
        testCmdList.emplace_back("上##cmd_up", [this]() {
            test_cmd_do_all(OwlMailDefine::ControlCmd::up);
        });
        testCmdList.emplace_back("下##cmd_down", [this]() {
            test_cmd_do_all(OwlMailDefine::ControlCmd::down);
        });
        testCmdList.emplace_back("顺##cmd_cw", [this]() {
            test_cmd_do_all(OwlMailDefine::ControlCmd::cw);
        });
        testCmdList.emplace_back("逆##cmd_ccw", [this]() {
            test_cmd_do_all(OwlMailDefine::ControlCmd::ccw);
        });
    }

    void ImGuiServiceImpl::show_test_cmd() {
        ImGui::Begin("控制测试", &show_test_cmd_window);

        ImVec2 button_sz(60, 30);

//            ImGui::InputInt3("##cmd_Goto_inpot", goto_pos.data());
//            ImGui::SameLine();
//            if (ImGui::Button("Goto##cmd_Goto", button_sz)) {
//                // TODO
//                auto &accRc = items.get<DiscoverStateItemContainerSequencedAccess>();
//                for (const auto &a: accRc) {
//
//                }
//            }

//            ImGui::InputInt3("##cmd_mode_inpot", goto_pos.data());
//            ImGui::SameLine();
//            if (ImGui::Button("FlyMode##cmd_mode", button_sz)) {
//                // TODO
//                auto &accRc = items.get<DiscoverStateItemContainerSequencedAccess>();
//                for (const auto &a: accRc) {
//
//                }
//            }


        ImGuiStyle &style = ImGui::GetStyle();
        float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

        for (size_t i = 0; i != testCmdList.size(); ++i) {
            auto a = testCmdList.at(i);
//                ImGui::ImVec2 label_size = ImGui::CalcTextSize(a.name.c_str(), NULL, true);
            if (ImGui::Button(a.name.c_str(), button_sz)) {
                if (a.callback) {
                    a.callback();
                }
            }
            float last_button_x2 = ImGui::GetItemRectMax().x;
            float next_button_x2 = last_button_x2 + style.ItemSpacing.x + button_sz.x;
            if (i + 1 < testCmdList.size() && next_button_x2 < window_visible_x2) {
                ImGui::SameLine();
            }
        }


        if (ImGui::BeginTable("CmdSelectTable", 2,
                              ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings |
                              ImGuiTableFlags_Borders)) {
            auto &accRc = items.get<DiscoverStateItemContainerSequencedAccess>();
            for (const auto &a: accRc) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Selectable(a.ip.c_str(), a.selected.get(),
                                  ImGuiSelectableFlags_SpanAllColumns);
                ImGui::TableNextColumn();
                ImGui::Text(boost::lexical_cast<std::string>(a.port).c_str());
            }
            ImGui::EndTable();
        }


        ImGui::End();
    }

    void ImGuiServiceImpl::start() {
        boost::asio::post(ioc_, [
                this, self = shared_from_this()
        ]() {
            auto r = init();
            if (r != 0) {
                BOOST_LOG_OWL(error) << "ImGuiServiceImpl init error " << r;
                clear();
                OwlImGuiService::safe_exit();
                return;
            }
        });
    }

    void ImGuiServiceImpl::show_camera(const OwlDiscoverState::DiscoverStateItem &a) {
        ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiCond_FirstUseEver);
        ImGui::Begin((std::string{"相机测试 "} + a.ip + std::string{"##show_camera"} + a.ip).c_str(),
                     a.showCamera.get(), ImGuiWindowFlags_NoSavedSettings);

        if (ImGui::Button((std::string{"Front##show_camera_Front"} + a.ip).c_str())) {
            getImageService->get(
                    a.ip,
                    boost::lexical_cast<std::string>(config_->config().ImageServiceHttpPort),
                    "/front",
                    11,
                    [
                            this, self = shared_from_this(), a
                    ](boost::beast::error_code ec, bool ok, cv::Mat img) {
                        if (ec) {
                            BOOST_LOG_OWL(warning) << "show_camera /front ec " << ec.what();
                            return;
                        }
                        if (img.empty()) {
                            return;
                        }
                        auto rb = a.imgDataFont->updateTexture(img, g_pd3dDevice);
                        if (!rb) {
                            a.imgDataFont->cleanTexture();
                        }
                        img.release();
                        BOOST_LOG_OWL(trace_gui) << "show_camera /front rb " << rb;
//                            if (ok) {
//                                auto d = OwlImGuiDirectX11::loadTextureFromMat(img, g_pd3dDevice);
//                                if (d) {
//                                    (*a.imgDataFont).operator=(std::move(*d));
//                                }
//                            }
                    }
            );
        }
        ImGui::SameLine();
        if (ImGui::Button((std::string{"Down##show_camera_Down"} + a.ip).c_str())) {
            getImageService->get(
                    a.ip,
                    boost::lexical_cast<std::string>(config_->config().ImageServiceHttpPort),
                    "/down",
                    11,
                    [
                            this, self = shared_from_this(), a
                    ](boost::beast::error_code ec, bool ok, cv::Mat img) {
                        if (ec) {
                            BOOST_LOG_OWL(warning) << "show_camera /down ec " << ec.what();
                            return;
                        }
                        if (img.empty()) {
                            return;
                        }
                        auto rb = a.imgDataDown->updateTexture(img, g_pd3dDevice);
                        if (!rb) {
                            a.imgDataDown->cleanTexture();
                        }
                        img.release();
                        BOOST_LOG_OWL(trace_gui) << "show_camera /down rb " << rb;
//                            if (ok) {
//                                auto d = OwlImGuiDirectX11::loadTextureFromMat(img, g_pd3dDevice);
//                                if (d) {
//                                    (*a.imgDataDown).operator=(std::move(*d));
//                                }
//                            }
                    }
            );
        }

        static bool use_text_color_for_tint = false;
//            float w = ImGui::GetStyle().ItemSpacing.x / 2.f;
//            BOOST_LOG_OWL(trace_gui) << "show_camera ImGui::GetStyle().ItemSpacing.x " << ImGui::GetStyle().ItemSpacing.x;
//            BOOST_LOG_OWL(trace_gui) << "show_camera ImGui::GetStyle().ItemSpacing.y " << ImGui::GetStyle().ItemSpacing.y;
//            float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
//            BOOST_LOG_OWL(trace_gui) << "show_camera ImGui::GetWindowPos().x " << ImGui::GetWindowPos().x;
//            BOOST_LOG_OWL(trace_gui) << "show_camera ImGui::GetWindowPos().y " << ImGui::GetWindowPos().y;
//            BOOST_LOG_OWL(trace_gui) << "show_camera ImGui::GetWindowContentRegionMax().x " << ImGui::GetWindowContentRegionMax().x;
//            BOOST_LOG_OWL(trace_gui) << "show_camera ImGui::GetWindowContentRegionMax().y " << ImGui::GetWindowContentRegionMax().y;
//            BOOST_LOG_OWL(trace_gui) << "show_camera window_visible_x2 " << window_visible_x2;
        float w = ImGui::GetWindowContentRegionMax().x / 2.f - ImGui::GetStyle().ItemSpacing.y;
        ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
        ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
        ImVec4 tint_col = use_text_color_for_tint ?
                          ImGui::GetStyleColorVec4(ImGuiCol_Text) :
                          ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
        ImVec4 border_col = ImGui::GetStyleColorVec4(ImGuiCol_Border);

        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        if (a.imgDataFont->texture) {
            ImGui::Image(&*(a.imgDataFont->texture),
                         ImVec2{w, w / a.imgDataFont->width * a.imgDataFont->height},
//                             ImVec2{80, 60},
                         uv_min, uv_max, tint_col, border_col);
        } else {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + w);
            ImGui::Text("Front Camera No Data.");
            draw_list->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(255, 255, 0, 255));
            ImGui::PopTextWrapPos();
        }
        ImGui::SameLine();
        if (a.imgDataDown->texture) {
            ImGui::Image(&*(a.imgDataDown->texture),
                         ImVec2{w, w / a.imgDataDown->width * a.imgDataDown->height},
//                             ImVec2{80, 60},
                         uv_min, uv_max, tint_col, border_col);
        } else {
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + w);
            ImGui::Text("Down Camera No Data.");
            draw_list->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(255, 255, 0, 255));
            ImGui::PopTextWrapPos();
        }

        ImGui::End();
    }


} // OwlImGuiServiceImpl


