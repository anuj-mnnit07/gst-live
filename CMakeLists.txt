cmake_minimum_required(VERSION 3.15.0)
project(gstlive VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(./videoplug)

find_package(PkgConfig REQUIRED)
# pkg_check_modules(gtk3 REQUIRED IMPORTED_TARGET gtk+-3.0)
pkg_search_module(gstreamer REQUIRED IMPORTED_TARGET gstreamer-1.0>=1.4)
pkg_search_module(gstreamer-sdp REQUIRED IMPORTED_TARGET gstreamer-sdp-1.0>=1.4)
pkg_search_module(gstreamer-app REQUIRED IMPORTED_TARGET gstreamer-app-1.0>=1.4)
pkg_search_module(gstreamer-video REQUIRED IMPORTED_TARGET gstreamer-video-1.0>=1.4)

add_executable(gstlive main.cpp)
target_link_libraries(gstlive
                        videoplug
                        # PkgConfig:gtk3
                        PkgConfig::gstreamer
                        PkgConfig::gstreamer-sdp
                        PkgConfig::gstreamer-app
                        PkgConfig::gstreamer-video
)


include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
