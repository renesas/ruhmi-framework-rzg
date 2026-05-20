cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 17)
project(image_classification C CXX)

set(EXE_NAME image_classification)

file(GLOB SOURCE *.cpp *.h *.hpp)
include_directories(${CMAKE_SYSROOT}/usr/src/debug/gstreamer1.0-plugins-base/1.22.12/gst-libs/gst/gl)
list(APPEND SOURCE ${CMAKE_SYSROOT}/usr/src/debug/gstreamer1.0-plugins-base/1.22.12/gst-libs/gst/gl/xdg-shell-client-protocol.c)

add_executable (${EXE_NAME} ${SOURCE})

find_package(PkgConfig REQUIRED)
pkg_check_modules(OPENCV REQUIRED opencv4)
include_directories(${OPENCV_INCLUDE_DIRS})
link_directories(${OPENCV_LIBRARY_DIRS})

# Link against MERA2, OpenCV and wayland-client libraries
target_link_libraries(${EXE_NAME} mera2_plan_io mera2_runtime ${OPENCV_LIBRARIES} wayland-client wayland-egl pthread jpeg EGL GLESv2)
