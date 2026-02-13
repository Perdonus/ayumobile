# This file is part of Telegram Desktop,
# the official desktop application for the Telegram messaging service.
#
# For license and copyright information please follow this link:
# https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL

cmake_minimum_required(VERSION 3.25...3.31)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include(cmake/validate_special_target.cmake)
include(cmake/version.cmake)
desktop_app_parse_version(Telegram/build/version)

project(Telegram
    LANGUAGES C CXX
    VERSION ${desktop_app_version_cmake}
    DESCRIPTION "AyuGram Desktop"
    HOMEPAGE_URL "https://ayugram.one"
)

if (APPLE)
    enable_language(OBJC OBJCXX)
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Telegram)

get_filename_component(third_party_loc "Telegram/ThirdParty" REALPATH)
get_filename_component(submodules_loc "Telegram" REALPATH)
get_filename_component(cmake_helpers_loc "cmake" REALPATH)

if (NOT DESKTOP_APP_USE_PACKAGED AND WIN32)
    set(Python3_EXECUTABLE ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/python/Scripts/python)
endif()

include(cmake/variables.cmake)
include(cmake/nice_target_sources.cmake)
include(cmake/target_compile_options_if_exists.cmake)
include(cmake/target_link_frameworks.cmake)
include(cmake/target_link_options_if_exists.cmake)
include(cmake/init_target.cmake)
include(cmake/generate_target.cmake)
include(cmake/nuget.cmake)
include(cmake/validate_d3d_compiler.cmake)
include(cmake/target_prepare_qrc.cmake)

include(cmake/options.cmake)
include(cmake/external/qt/package.cmake)

if (DESKTOP_APP_SPECIAL_TARGET STREQUAL "android")
    set(add_cld3_library 0)
    set(add_hunspell_library 1)
endif()

set(desktop_app_skip_libs
    ada
    glibmm
    variant
)
if (DESKTOP_APP_SPECIAL_TARGET STREQUAL "android")
    list(APPEND desktop_app_skip_libs
        boost
    )
endif()

add_subdirectory(cmake)
add_subdirectory(Telegram)
