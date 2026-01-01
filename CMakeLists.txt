cmake_minimum_required(VERSION 3.16)
project(webrtc_framecapture_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(ENABLE_TESTS "Build tests" ON)

find_package(Threads REQUIRED)
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil libswscale)

add_library(capture_app
  src/app/App.cpp
  src/ingest/RtpReceiver.cpp
  src/media/FrameWriter.cpp
  src/util/Args.cpp
  src/util/Log.cpp
)

target_include_directories(capture_app
  PUBLIC
    ${OpenCV_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(capture_app PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(capture_app
  PUBLIC
    ${OpenCV_LIBS}
    ${FFMPEG_LIBRARIES}
    Threads::Threads
)

add_executable(webrtc_capture src/main.cpp)
target_link_libraries(webrtc_capture PRIVATE capture_app)

if(ENABLE_TESTS)
  enable_testing()

  add_executable(test_framewriter tests/test_framewriter.cpp)
  target_link_libraries(test_framewriter PRIVATE capture_app)
  add_test(NAME test_framewriter COMMAND test_framewriter)
endif()
