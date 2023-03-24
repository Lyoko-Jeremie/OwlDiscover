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


# https://www.boost.org/doc/libs/1_81_0/libs/assert/doc/html/assert.html
add_definitions(-DBOOST_ENABLE_ASSERT_HANDLER)  #


# ============================================================================================


option(USE_DYNAMIC_BOOST "Config USE_DYNAMIC_BOOST")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "enable -O1 when debug to resolve 'string table overflow at offset' 'too big' error on mingw")
    # -O1 to resolve "string table overflow at offset" error on mingw
    add_definitions(-Wall -Wextra -O1 -g)
endif ()

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

set(CMAKE_PREFIX_PATH  "h:/CodeLib/freetype-2.9_x64_v11p3-install/lib/cmake/freetype/" ${CMAKE_PREFIX_PATH})

find_package(Freetype REQUIRED)

message(STATUS "FREETYPE_FOUND: ${FREETYPE_FOUND}")
message(STATUS "FREETYPE_INCLUDE_DIRS: ${FREETYPE_INCLUDE_DIRS}")
message(STATUS "FREETYPE_LIBRARIES: ${FREETYPE_LIBRARIES}")
message(STATUS "FREETYPE_VERSION_STRING: ${FREETYPE_VERSION_STRING}")
message(STATUS "FREETYPE_INCLUDE_DIR_ft2build: ${FREETYPE_INCLUDE_DIR_ft2build}")
message(STATUS "FREETYPE_INCLUDE_DIR_freetype2: ${FREETYPE_INCLUDE_DIR_freetype2}")

# ============================================================================================

set(CMAKE_PREFIX_PATH  "h:/CodeLib/harfbuzz-7.1.0_x64_v11p3-install/lib/cmake/harfbuzz/" ${CMAKE_PREFIX_PATH})

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


message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")


add_executable(${ProjectMainName}
        ${DearImGuiFile}
        src/main.cpp
        src/OwlLog/BoostAssertionFailed.cpp
        src/OwlLog/OwlLog.h
        src/OwlLog/OwlLog.cpp
        src/ImGuiService/ImGuiService.cpp
        src/ImGuiService/ImGuiService.h)

# ============================================================================================


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
