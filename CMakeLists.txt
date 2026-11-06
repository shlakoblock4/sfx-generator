cmake_minimum_required(VERSION 3.20)
project(sfx_qt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Ищем все исходники в папке
file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")

# --- Qt ---
find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)

# --- OGG (Vorbis) ---
find_package(Ogg CONFIG QUIET)
find_package(Vorbis CONFIG QUIET)

# --- MP3 (LAME) ---
find_package(mp3lame CONFIG QUIET)
if(NOT mp3lame_FOUND)
    find_package(lame CONFIG QUIET)
endif()

add_executable(sfx ${SOURCES} ${HEADERS})

if(MSVC)
    target_compile_options(sfx PRIVATE /O2 /arch:AVX2)
else()
    target_compile_options(sfx PRIVATE -O3 -march=native -ffast-math)
endif()

target_link_libraries(sfx PRIVATE Qt6::Widgets Qt6::Multimedia)

# --- OGG ---
if(Ogg_FOUND AND Vorbis_FOUND)
    target_compile_definitions(sfx PRIVATE HAVE_VORBIS)
    target_link_libraries(sfx PRIVATE Ogg::ogg Vorbis::vorbis Vorbis::vorbisenc)
    message(STATUS "OGG export: enabled (libvorbis)")
else()
    message(STATUS "OGG export: DISABLED")
endif()

# --- MP3 ---
if(mp3lame_FOUND)
    target_compile_definitions(sfx PRIVATE HAVE_LAME)
    if(TARGET mp3lame::mp3lame)
        target_link_libraries(sfx PRIVATE mp3lame::mp3lame)
    else()
        target_link_libraries(sfx PRIVATE mp3lame)
    endif()
    message(STATUS "MP3 export: enabled (libmp3lame)")
elseif(lame_FOUND)
    target_compile_definitions(sfx PRIVATE HAVE_LAME)
    if(TARGET lame::lame)
        target_link_libraries(sfx PRIVATE lame::lame)
    else()
        target_link_libraries(sfx PRIVATE lame)
    endif()
    message(STATUS "MP3 export: enabled (liblame)")
else()
    message(STATUS "MP3 export: DISABLED (install vcpkg mp3lame)")
endif()

# === Автоматический запуск windeployqt после сборки ===
if(WIN32)
    find_program(WINDEPLOYQT_EXE windeployqt HINTS "${CMAKE_PREFIX_PATH}/bin")
    if(WINDEPLOYQT_EXE)
        add_custom_command(TARGET sfx POST_BUILD
            COMMAND ${WINDEPLOYQT_EXE} 
                --release 
                --no-translations 
                --no-compiler-runtime 
                $<TARGET_FILE:sfx>
            COMMENT "Автоматическое копирование Qt DLL..."
        )
    endif()
endif()