cmake_minimum_required(VERSION 3.27)
project(Tetrus)

SET(CMAKE_SYSTEM_NAME Windows)

SET(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
SET(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
SET(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
SET(CMAKE_RANLIB x86_64-w64-mingw32-ranlib)

SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -s")

include_directories(../nya-common)
include_directories(../nya-common/3rdparty)
include_directories(../nya-common/3rdparty/imgui)
include_directories(../nya-common/3rdparty/imgui/backends)
include_directories(../nya-common/3rdparty/taglib)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include/taglib)
link_directories(${CMAKE_SOURCE_DIR}/../nya-common/lib64)

add_compile_definitions(IMGUI_USER_CONFIG="../../nya_imconfig.h" NYA_BACKEND_DX11 NYA_AUDIO_BASS NYA_NET_ENET NYA_APP_WINDOWNAME="nya :3" NYA_FONTNAME="FranxurterTotallyMedium-gxwjp.ttf" NYA_FONTSIZE=64.0 NYA_APP_WINDOWED=false NYANET_MAXPLAYERS=64)
add_executable(Tetrus
        ../nya-common/3rdparty/imgui/imgui.cpp
        ../nya-common/3rdparty/imgui/backends/imgui_impl_dx11.cpp
        ../nya-common/3rdparty/imgui/imgui_draw.cpp
        ../nya-common/3rdparty/imgui/imgui_tables.cpp
        ../nya-common/3rdparty/imgui/imgui_widgets.cpp
        ../nya-common/nya_commonfuncs.cpp
        ../nya-common/nya_commondraw.cpp
        ../nya-common/nya_commonaudio.cpp
        ../nya-common/nya_commontimer.cpp
        ../nya-common/nya_commonnet.cpp
        ../nya-common/nya_dx11_appbase.cpp
        tetrus_util.cpp
        tetrus_config.cpp
        tetrus_controls.cpp
        tetrus_audio.cpp
        tetrus_board.cpp
        tetrus_piece.cpp
        tetrus_player.cpp
        tetrus_saveload.cpp
        tetrus_menu.cpp
        tetrus_scoreboard.cpp
        tetrus_replay.cpp
        tetrus_net.cpp
        main.cpp)

target_link_libraries(Tetrus d3d11)
target_link_libraries(Tetrus d3dcompiler)
target_link_libraries(Tetrus bass)
target_link_libraries(Tetrus enet)
target_link_libraries(Tetrus ws2_32)
target_link_libraries(Tetrus winmm)