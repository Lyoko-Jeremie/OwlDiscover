cmake_minimum_required(VERSION 3.26)
set(ProjectMainName OwlDiscover)
project(${ProjectMainName})

set(CMAKE_CXX_STANDARD 20)


# ============================================================================================

#-DUSE_DYNAMIC_BOOST=OFF
#-DBOOST_ROOT=h:/CodeLib/boost_1_81_0_install_x64_v11p3/
#-DFREETYPE_FIND_ARGS=h:/CodeLib/freetype-2.9_x64_v11p3-install
#-DFREETYPE_LIBRARY=h:/CodeLib/freetype-2.9_x64_v11p3-install/lib/libfreetype.a
#-DFREETYPE_INCLUDE_DIR_ft2build=h:/CodeLib/freetype-2.9_x64_v11p3-install/include/freetype2
#-DFREETYPE_INCLUDE_DIR_freetype2=h:/CodeLib/freetype-2.9_x64_v11p3-install/include/freetype2
#-DSDL2_DIR=h:/CodeLib/SDL2-2.26.4_x64_v11p3-install/lib/cmake/SDL2

# ============================================================================================

#set(DEBUG_log_multicast ON)
#set(trace_http ON)
#set(DEBUG_Mailbox ON)
#set(DEBUG_log_gui ON)

# ============================================================================================


if (DEBUG_BOOST_ASIO)
    add_definitions(-DBOOST_ASIO_ENABLE_HANDLER_TRACKING)
endif (DEBUG_BOOST_ASIO)

if (DEBUG_log_sp_tag)
    add_definitions(-DDEBUG_log_sp_tag)
endif (DEBUG_log_sp_tag)
if (DEBUG_log_dtor)
    add_definitions(-DDEBUG_log_dtor)
endif (DEBUG_log_dtor)
if (DEBUG_log_MAP)
    add_definitions(-DDEBUG_log_MAP)
endif (DEBUG_log_MAP)
if (DEBUG_log_TAG)
    add_definitions(-DDEBUG_log_TAG)
endif (DEBUG_log_TAG)
if (DEBUG_log_cmd_HTTP)
    add_definitions(-DDEBUG_log_cmd_HTTP)
endif (DEBUG_log_cmd_HTTP)
if (DEBUG_log_cmd_HTTP_error)
    add_definitions(-DDEBUG_log_cmd_HTTP_error)
endif (DEBUG_log_cmd_HTTP_error)
if (DEBUG_log_SerialPortWrite)
    add_definitions(-DDEBUG_log_SerialPortWrite)
endif (DEBUG_log_SerialPortWrite)
if (DEBUG_log_SerialPortWrite_dbg)
    add_definitions(-DDEBUG_log_SerialPortWrite_dbg)    #
endif (DEBUG_log_SerialPortWrite_dbg)
if (DEBUG_log_JSON)
    add_definitions(-DDEBUG_log_JSON)
endif (DEBUG_log_JSON)
if (DEBUG_log_http)
    add_definitions(-DDEBUG_log_http)
endif (DEBUG_log_http)
if (DEBUG_log_udp)
    add_definitions(-DDEBUG_log_udp)
endif (DEBUG_log_udp)
if (DEBUG_log_multicast)
    add_definitions(-DDEBUG_log_multicast)
endif (DEBUG_log_multicast)
if (DEBUG_log_gui)
    add_definitions(-DDEBUG_log_gui)
endif (DEBUG_log_gui)
if (DEBUG_log_SerialPortRead)
    add_definitions(-DDEBUG_log_SerialPortRead)
endif (DEBUG_log_SerialPortRead)

if (DEBUG_TAG_INFO)
    add_definitions(-DDEBUG_TAG_INFO)
endif (DEBUG_TAG_INFO)

if (DEBUG_IF_CHECK_POINT)
    add_definitions(-DDEBUG_IF_CHECK_POINT)
endif (DEBUG_IF_CHECK_POINT)

if (DEBUG_ReceiveMailRepeat)
    add_definitions(-DDEBUG_ReceiveMailRepeat)
endif (DEBUG_ReceiveMailRepeat)

if (DEBUG_Mailbox)
    add_definitions(-DDEBUG_AsyncCallbackMailbox)
endif (DEBUG_Mailbox)
if (DEBUG_NoStateReader)
    add_definitions(-DDEBUG_DisableStateReader)
endif (DEBUG_NoStateReader)


# ============================================================================================

# https://www.boost.org/doc/libs/1_81_0/libs/assert/doc/html/assert.html
add_definitions(-DBOOST_ENABLE_ASSERT_HANDLER)  #


# ============================================================================================


option(USE_DYNAMIC_BOOST "Config USE_DYNAMIC_BOOST")

set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

#if (NOT_USE_BOOST_THEAD)
#    set(BOOST_THEAD_MODULE "")
#    message(STATUS "NOT_USE_BOOST_THEAD !!!")
#else ()
#    set(BOOST_THEAD_MODULE "thread")
#    add_definitions(-DUSE_BOOST_THEAD)
#endif ()
set(BOOST_THEAD_MODULE "thread")
add_definitions(-DUSE_BOOST_THEAD)


if (USE_DYNAMIC_BOOST)
    message(STATUS "USE DYNAMIC BOOST !!!")
    add_definitions(-DBOOST_ALL_DYN_LINK=1)
else (USE_DYNAMIC_BOOST)
    message(STATUS "USE STATIC BOOST !!!")
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_STATIC_RUNTIME OFF)
endif (USE_DYNAMIC_BOOST)

set(Boost_USE_MULTITHREADED ON)
#find_package(Boost REQUIRED COMPONENTS  REQUIRED)
find_package(Boost 1.81.0 REQUIRED COMPONENTS system filesystem program_options json log log_setup url ${BOOST_THEAD_MODULE} REQUIRED)


message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")


message(STATUS "Boost_FOUND: ${Boost_FOUND}")
message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
message(STATUS "Boost_VERSION: ${Boost_VERSION}")


message(STATUS "CMAKE_THREAD_LIBS_INIT: ${CMAKE_THREAD_LIBS_INIT}")


# ============================================================================================

set(CMAKE_PREFIX_PATH "h:/CodeLib/freetype-2.9_x64_v11p3-install/lib/cmake/freetype/" ${CMAKE_PREFIX_PATH})

find_package(Freetype REQUIRED)

message(STATUS "FREETYPE_FOUND: ${FREETYPE_FOUND}")
message(STATUS "FREETYPE_INCLUDE_DIRS: ${FREETYPE_INCLUDE_DIRS}")
message(STATUS "FREETYPE_LIBRARIES: ${FREETYPE_LIBRARIES}")
message(STATUS "FREETYPE_VERSION_STRING: ${FREETYPE_VERSION_STRING}")
message(STATUS "FREETYPE_INCLUDE_DIR_ft2build: ${FREETYPE_INCLUDE_DIR_ft2build}")
message(STATUS "FREETYPE_INCLUDE_DIR_freetype2: ${FREETYPE_INCLUDE_DIR_freetype2}")

# ============================================================================================

set(CMAKE_PREFIX_PATH "h:/CodeLib/harfbuzz-7.1.0_x64_v11p3-install/lib/cmake/harfbuzz/" ${CMAKE_PREFIX_PATH})

find_package(harfbuzz REQUIRED)

message(STATUS "FREETYPE_FOUND: ${FREETYPE_FOUND}")

add_definitions(-DIMGUI_ENABLE_FREETYPE)

# ============================================================================================


find_package(SDL2 REQUIRED)

message(STATUS "SDL2_FOUND: ${SDL2_FOUND}")
message(STATUS "SDL2_INCLUDE_DIRS: ${SDL2_INCLUDE_DIRS}")
message(STATUS "SDL2_LIBDIR: ${SDL2_LIBDIR}")
message(STATUS "SDL2_LIBRARIES: ${SDL2_LIBRARIES}")
message(STATUS "SDL2_STATIC_LIBRARIES: ${SDL2_STATIC_LIBRARIES}")


# ============================================================================================


# ============================================================================================

add_definitions(-DSDL_HINT_IME_SHOW_UI)

include_directories(ExternLib/imgui ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(ExternLib/imgui/backends ${CMAKE_CURRENT_SOURCE_DIR})

add_definitions(-DIMGUI_USER_CONFIG="src/im_custom_config.h")

set(DearImGuiFile
        ExternLib/imgui/imgui.h
        ExternLib/imgui/imgui.cpp
        ExternLib/imgui/imgui_demo.cpp
        ExternLib/imgui/imgui_draw.cpp
        ExternLib/imgui/imgui_internal.h
        ExternLib/imgui/imgui_tables.cpp
        ExternLib/imgui/imgui_widgets.cpp
        ExternLib/imgui/imstb_truetype.h
        ExternLib/imgui/imstb_rectpack.h
        ExternLib/imgui/imstb_textedit.h
        ExternLib/imgui/misc/cpp/imgui_stdlib.h
        ExternLib/imgui/misc/cpp/imgui_stdlib.cpp
        ExternLib/imgui/misc/freetype/imgui_freetype.h
        ExternLib/imgui/misc/freetype/imgui_freetype.cpp
        ExternLib/imgui/misc/freetype/imgui_freetype.cpp
        ExternLib/imgui/backends/imgui_impl_sdl2.h
        ExternLib/imgui/backends/imgui_impl_sdl2.cpp
        ExternLib/imgui/backends/imgui_impl_dx11.h
        ExternLib/imgui/backends/imgui_impl_dx11.cpp)

# ============================================================================================

# ============================================================================================


# https://www.mattkeeter.com/blog/2018-01-06-versioning/
execute_process(COMMAND git log --pretty=format:'%H' -n 1
        OUTPUT_VARIABLE GIT_REV
        ERROR_QUIET)

# Check whether we got any revision (which isn't
# always the case, e.g. when someone downloaded a zip
# file from Github instead of a checkout)
if ("${GIT_REV}" STREQUAL "")
    set(GIT_REV "N/A")
    set(GIT_DIFF "")
    set(GIT_TAG "N/A")
    set(GIT_BRANCH "N/A")
else ()
    execute_process(
            COMMAND bash -c "git diff --quiet --exit-code || echo +"
            OUTPUT_VARIABLE GIT_DIFF)
    execute_process(
            COMMAND git describe --exact-match --tags
            OUTPUT_VARIABLE GIT_TAG ERROR_QUIET)
    execute_process(
            COMMAND git rev-parse --abbrev-ref HEAD
            OUTPUT_VARIABLE GIT_BRANCH)

    string(STRIP "${GIT_REV}" GIT_REV)
    #    string(SUBSTRING "${GIT_REV}" 1 7 GIT_REV)
    string(STRIP "${GIT_DIFF}" GIT_DIFF)
    string(STRIP "${GIT_TAG}" GIT_TAG)
    string(STRIP "${GIT_BRANCH}" GIT_BRANCH)
endif ()

# https://gitlab.kitware.com/cmake/cmake/-/issues/18431
string(TIMESTAMP currentSeconds "%s")
message(STATUS "currentSeconds = ${currentSeconds}")

string(TIMESTAMP BUILD_DATETIME "%Y-%m-%d %H:%M:%S")
message(STATUS "BUILD_DATETIME = ${BUILD_DATETIME}")

set(VERSION "const char* GIT_REV=\"git-[${GIT_REV}-${GIT_DIFF}]\";
const char* GIT_TAG=\"${GIT_TAG}\";
const char* GIT_BRANCH=\"${GIT_BRANCH}\";
const char* BUILD_DATETIME=\"${BUILD_DATETIME}\";
")

message(STATUS "VERSION: ${VERSION}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/VERSION/CodeVersion.h.in
        ${CMAKE_CURRENT_SOURCE_DIR}/src/VERSION/CodeVersion.h)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})


# ============================================================================================


message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")


add_executable(${ProjectMainName}
        ${DearImGuiFile}
        resource.rc
        src/main.cpp
        src/OwlLog/BoostAssertionFailed.cpp
        src/VERSION/ProgramVersion.h
        src/VERSION/CodeVersion.h
        src/OwlLog/OwlLog.h
        src/OwlLog/OwlLog.cpp
        src/ConfigLoader/ConfigLoader.h
        src/ConfigLoader/ConfigLoader.cpp
        src/AsyncCallbackMailbox/AsyncCallbackMailbox.h
        src/ImGuiService/ImGuiService.cpp
        src/ImGuiService/ImGuiService.h
        src/ImGuiService/ControlImGuiMailBox.h
        src/MultiCast/MultiCast.h
        src/MultiCast/MultiCast.cpp
        src/MultiCast/ControlMulticastMail.h
        src/DiscoverState/DiscoverState.h
        src/ControlService/UdpControl.cpp
        src/ControlService/UdpControl.h
        src/ControlService/ControlServiceUdpMailBox.h
        src/ControlService/HttpControl.cpp
        src/ControlService/HttpControl.h
        src/ControlService/ControlServiceHttpMailBox.h
        src/ControlService/OwlCmd.h)

# ============================================================================================


if (CMAKE_BUILD_TYPE STREQUAL "Release")
    # disable console window
    target_link_options(${ProjectMainName} PRIVATE -mwindows)
    add_compile_options(-Wall -Wextra -O3)
    add_definitions(-DIMGUI_DISABLE_DEMO_WINDOWS -DIMGUI_DISABLE_DEBUG_TOOLS)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "enable -O1 when debug to resolve 'string table overflow at offset' 'too big' error on mingw")
    # -O1 to resolve "string table overflow at offset" error on mingw
    # add_compile_options(-Wall -Wextra -O1 -g)
    add_compile_options(-Wall -Wextra -g)
endif ()

target_link_libraries(${ProjectMainName} ${CMAKE_THREAD_LIBS_INIT})

include_directories(${CMAKE_SOURCE_DIR} ${Boost_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS})
target_link_libraries(${ProjectMainName} ${Boost_LIBRARIES} ${FREETYPE_LIBRARIES})
link_directories(${Boost_LIBRARY_DIRS} ${SDL2_LIBDIR})
target_link_libraries(${ProjectMainName} ${SDL2_STATIC_LIBRARIES} harfbuzz::harfbuzz)

target_link_libraries(${ProjectMainName} d3d11 d3dcompiler dxgi)

if (WIN32)
    target_link_libraries(${ProjectMainName} wsock32 ws2_32 crypt32)
else (WIN32)
endif (WIN32)
