cmake_minimum_required(VERSION 3.16)
project(webrtc_framecapture_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(ENABLE_TESTS "Build tests" ON)

find_package(Threads REQUIRED)
find_package(OpenCV REQUIRED)

set(WEBRTC_INCLUDE_DIR "" CACHE PATH "Path to WebRTC include directory")
set(WEBRTC_LIBRARY "" CACHE FILEPATH "Path to WebRTC library")
set(THIRD_PARTY_INCLUDE_DIR "" CACHE PATH "Path to header-only deps (httplib/json)")

if(NOT WEBRTC_INCLUDE_DIR OR NOT EXISTS "${WEBRTC_INCLUDE_DIR}")
  message(FATAL_ERROR "WEBRTC_INCLUDE_DIR is required and must exist")
endif()
if(NOT WEBRTC_LIBRARY OR NOT EXISTS "${WEBRTC_LIBRARY}")
  message(FATAL_ERROR "WEBRTC_LIBRARY is required and must exist")
endif()

add_library(webrtc_receiver
  src/app/App.cpp
  src/signaling/HttpSignalingServer.cpp
  src/webrtc/WebrtcFactory.cpp
  src/webrtc/Peer.cpp
  src/webrtc/SdpUtils.cpp
  src/webrtc/Observers.cpp
  src/webrtc/VideoFrameSink.cpp
  src/media/MatUtils.cpp
  src/media/FrameWriter.cpp
  src/util/Args.cpp
  src/util/Log.cpp
)

target_include_directories(webrtc_receiver
  PUBLIC
    ${OpenCV_INCLUDE_DIRS}
    ${WEBRTC_INCLUDE_DIR}
    ${THIRD_PARTY_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(webrtc_receiver
  PUBLIC
    ${OpenCV_LIBS}
    ${WEBRTC_LIBRARY}
    Threads::Threads
)

target_compile_definitions(webrtc_receiver PUBLIC _GLIBCXX_USE_CXX11_ABI=1)

target_compile_options(webrtc_receiver PRIVATE -Wall -Wextra -Wpedantic)

add_executable(webrtc_framecapture_server src/main.cpp)

target_link_libraries(webrtc_framecapture_server PRIVATE webrtc_receiver)

if(ENABLE_TESTS)
  enable_testing()

  add_executable(test_matutils tests/test_matutils.cpp)
  target_link_libraries(test_matutils PRIVATE webrtc_receiver)
  add_test(NAME test_matutils COMMAND test_matutils)

  add_executable(test_framewriter tests/test_framewriter.cpp)
  target_link_libraries(test_framewriter PRIVATE webrtc_receiver)
  add_test(NAME test_framewriter COMMAND test_framewriter)
endif()
