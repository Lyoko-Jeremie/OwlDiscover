// jeremie

#include "ImGuiService.h"
#include "../OwlLog/OwlLog.h"
#include <boost/lexical_cast.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/core/ignore_unused.hpp>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <cstdio>
#include <SDL.h>
#include <SDL_syswm.h>

using boost::asio::use_awaitable;
#if defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
# define use_awaitable \
  boost::asio::use_awaitable_t(__FILE__, __LINE__, __PRETTY_FUNCTION__)
#endif


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
                boost::asio::io_context &ioc
        ) : ioc_(ioc) {
        }

    private:
        boost::asio::io_context &ioc_;

    private:
        ID3D11Device *g_pd3dDevice = nullptr;
        ID3D11DeviceContext *g_pd3dDeviceContext = nullptr;
        IDXGISwapChain *g_pSwapChain = nullptr;
        ID3D11RenderTargetView *g_mainRenderTargetView = nullptr;

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

            // Cleanup
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplSDL2_Shutdown();
            ImGui::DestroyContext();

            CleanupDeviceD3D();
            SDL_DestroyWindow(window);
            window = nullptr;
            SDL_Quit();

        }

    private:
        SDL_Window *window = nullptr;

        int init() {

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
            window = SDL_CreateWindow("OwlDiscover", SDL_WINDOWPOS_CENTERED,
                                      SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
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
            //    ImGui::StyleColorsDark();
            ImGui::StyleColorsLight();

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
//            io.Fonts->AddFontFromFileTTF(R"(../SourceHanSansCN/SourceHanSans.ttc)", 18.0f,
//                                         &config, io.Fonts->GetGlyphRangesChineseFull()),
//            io.Fonts->AddFontFromFileTTF(R"(../SourceHanSansCN/SourceHanSansCN-Bold.otf)", 18.0f,
//                                         &config, io.Fonts->GetGlyphRangesChineseFull()),
//            io.Fonts->AddFontFromFileTTF(R"(../SourceHanSansCN/SourceHanSansCN-ExtraLight.otf)", 18.0f,
//                                         &config, io.Fonts->GetGlyphRangesChineseFull()),
//            io.Fonts->AddFontFromFileTTF(R"(../SourceHanSansCN/SourceHanSansCN-Heavy.otf)", 18.0f,
//                                         &config, io.Fonts->GetGlyphRangesChineseFull()),
//            io.Fonts->AddFontFromFileTTF(R"(../SourceHanSansCN/SourceHanSansCN-Light.otf)", 18.0f,
//                                         &config, io.Fonts->GetGlyphRangesChineseFull()),
//            io.Fonts->AddFontFromFileTTF(R"(../SourceHanSansCN/SourceHanSansCN-Medium.otf)", 18.0f,
//                                         &config, io.Fonts->GetGlyphRangesChineseFull()),
                    io.Fonts->AddFontFromFileTTF(R"(../SourceHanSansCN/SourceHanSansCN-Normal.otf)", 21.0f,
                                                 &config, io.Fonts->GetGlyphRangesChineseFull()),
//            io.Fonts->AddFontFromFileTTF(R"(../SourceHanSansCN/SourceHanSansCN-Regular.otf)", 18.0f,
//                                         NULL, io.Fonts->GetGlyphRangesChineseFull()),
            };
            io.Fonts->Build();
            for (const auto &a: fontsList) {
                if (!a) {
                    // cannot read fonts
                    clear();
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
                                BOOST_LOG_OWL(trace_cmd_tag) << "ImGuiServiceImpl run() ok";
                                clear();
                                safe_exit();
                                return;
                            } else {
                                BOOST_LOG_OWL(trace_cmd_tag) << "ImGuiServiceImpl run() error";
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


        bool show_demo_window = true;
        bool show_another_window = false;
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        boost::asio::awaitable<bool> co_main_loop(boost::shared_ptr<ImGuiServiceImpl> self) {
            boost::ignore_unused(self);

            BOOST_LOG_OWL(trace_cmd_tag) << "co_main_loop start";

            try {
                auto executor = co_await boost::asio::this_coro::executor;

                ImGuiIO &io = ImGui::GetIO();

                for (;;) {
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

                    if (show_demo_window)
                        ImGui::ShowDemoWindow(&show_demo_window);

                    {
                        static float f = 0.0f;
                        static int counter = 0;
                        static bool open = true;
                        static ImGuiWindowFlags window_flags = 0;
                        if (window_flags == 0) {
                            window_flags |= ImGuiWindowFlags_NoMove;
                            window_flags |= ImGuiWindowFlags_MenuBar;
                            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
                            window_flags |= ImGuiWindowFlags_NoDecoration;
                            window_flags |= ImGuiWindowFlags_NoSavedSettings;
                        }

                        // We specify a default position/size in case there's no data in the .ini file.
                        const ImGuiViewport *main_viewport = ImGui::GetMainViewport();
//            BOOST_LOG_TRIVIAL(trace) << "main_viewport "
//                                     << main_viewport->Pos.x << " "
//                                     << main_viewport->Pos.y << " "
//                                     << main_viewport->Size.x << " "
//                                     << main_viewport->Size.y << " "
//                                     << main_viewport->WorkPos.x << " "
//                                     << main_viewport->WorkPos.y << " "
//                                     << main_viewport->WorkSize.x << " "
//                                     << main_viewport->WorkSize.y << " "
//                                     << "";

                        ImGui::SetNextWindowPos(main_viewport->WorkPos);
                        ImGui::SetNextWindowSize(main_viewport->WorkSize);

                        // Create a window called "Hello, world!" and append into it.
                        if (ImGui::Begin("Hello, world!", &open, window_flags)) {
                            if (ImGui::BeginMenuBar()) {
                                if (ImGui::BeginMenu("Menu")) {
                                    if (ImGui::MenuItem("Quit", "Alt+F4")) {}
                                    ImGui::EndMenu();
                                }
                                ImGui::EndMenuBar();
                            }

//            ImGui::Text("有些有用的文字");   // Display some text (you can use a format strings too)
//            ImGui::Checkbox(u8"Demo 窗口"_C, &show_demo_window);      // Edit bools storing our window open/close state
//            ImGui::SameLine();
//            ImGui::Checkbox(u8"Another 窗口"_C, &show_another_window);
//
//            ImGui::SliderFloat(u8"浮点"_C, &f, 0.0f, 1.0f);  // Edit 1 float using a slider from 0.0f to 1.0f
//            ImGui::ColorEdit3(u8"色彩"_C, (float *) &clear_color); // Edit 3 floats representing a color
//
//            // Buttons return true when clicked (most widgets return true when edited/activated)
//            if (ImGui::Button(u8"按钮"_C))
//                counter++;
//            ImGui::SameLine();
//            ImGui::Text(u8"计数 = %d"_C, counter);


                            ImGui::Text("有些有用的文字");
                            ImGui::SameLine();
                            ImGui::Text("有些有用的文字");


                            ImGui::Button("StopAll");
                            ImGui::SameLine();
                            ImGui::Button("LandAll");
                            ImGui::SameLine();
                            ImGui::Button("ClearAll");

                            const float footer_height_to_reserve =
                                    ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
                            {
                                ImGui::BeginChild("AddrList", ImVec2(0, -footer_height_to_reserve),
                                                  true, ImGuiWindowFlags_HorizontalScrollbar);

                                for (int i = 0; i < 100; ++i) {
                                    ImGui::Text(boost::lexical_cast<std::string>(i).c_str());
                                    ImGui::SameLine();
                                    ImGui::Text("127.0.0.1");
                                    ImGui::SameLine();
                                    ImGui::Button("Land");
                                    ImGui::SameLine();
                                    ImGui::Button("Stop");
                                    ImGui::SameLine();
                                    ImGui::Button("Delete");
                                }

                                ImGui::EndChild();
                            }

                            ImGui::Text(u8"应用程序平均帧率 %.3f ms/frame (%.1f FPS)"_C, 1000.0f / io.Framerate,
                                        io.Framerate);

                            ImGui::End();
                        }
                    }


                    if (show_another_window) {
                        ImGui::Begin("Another Window",
                                     &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
                        ImGui::Text("Hello from another window!");
                        if (ImGui::Button("Close Me"))
                            show_another_window = false;
                        ImGui::End();
                    }


                    // Rendering
                    ImGui::Render();
                    ImGui::EndFrame();
                    const float clear_color_with_alpha[4] = {clear_color.x * clear_color.w,
                                                             clear_color.y * clear_color.w,
                                                             clear_color.z * clear_color.w, clear_color.w};
                    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
                    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
                    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

                    g_pSwapChain->Present(1, 0); // Present with vsync
                    //g_pSwapChain->Present(0, 0); // Present without vsync



                    // ================================ ............ ================================
                    // make a break for boost asio thread can handle another event
                    co_await boost::asio::post(executor, use_awaitable);
                    // ================================ ............ ================================

                }

            } catch (const std::exception &e) {
                BOOST_LOG_OWL(error) << "co_process_tag_info catch (const std::exception &e)" << e.what();
                throw;
//                co_return false;
            }

            boost::ignore_unused(self);
            co_return true;

        }


    };

    ImGuiService::ImGuiService(boost::asio::io_context &ioc) : ioc_(ioc) {
        impl = boost::make_shared<ImGuiServiceImpl>(ioc_);
    }

    void ImGuiService::start() {
        impl->start();
    }

    void ImGuiService::clear() {
        impl->clear();
    }
} // OwlImGuiService