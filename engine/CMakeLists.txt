cmake_minimum_required(VERSION 3.21)
project(cloud_match_engine CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Optional gRPC ──────────────────────────────────────────────────────────
find_package(Protobuf QUIET)
find_package(gRPC CONFIG QUIET)

if(gRPC_FOUND AND Protobuf_FOUND)
  message(STATUS "gRPC found — building full gRPC server")
  set(HAS_GRPC TRUE)
else()
  message(STATUS "gRPC not found — match_core only (no server binary)")
  set(HAS_GRPC FALSE)
endif()

# ── GoogleTest ─────────────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ── Core library (no gRPC dependency — always builds) ─────────────────────
add_library(match_core STATIC
  src/MatchEngine.cpp
)
target_include_directories(match_core PUBLIC src)
target_link_libraries(match_core PUBLIC Threads::Threads)
find_package(Threads REQUIRED)
target_link_libraries(match_core PUBLIC Threads::Threads)
if(MSVC)
  target_compile_options(match_core PRIVATE /W3)
else()
  target_compile_options(match_core PRIVATE -Wall -Wextra)
endif()

# ── gRPC server (conditional) ──────────────────────────────────────────────
if(HAS_GRPC)
  get_target_property(GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin LOCATION)
  set(PROTO_DIR "${CMAKE_SOURCE_DIR}/../proto")

  # Generate C++ from .proto
  protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_DIR}/match.proto)
  add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/match.grpc.pb.cc
            ${CMAKE_CURRENT_BINARY_DIR}/match.grpc.pb.h
    COMMAND protobuf::protoc
    ARGS    --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
            --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
            -I${PROTO_DIR}
            ${PROTO_DIR}/match.proto
    DEPENDS ${PROTO_DIR}/match.proto
  )

  add_library(match_grpc STATIC
    src/MatchService.cpp
    ${PROTO_SRCS}
    ${CMAKE_CURRENT_BINARY_DIR}/match.grpc.pb.cc
  )
  target_compile_definitions(match_grpc PUBLIC HAS_GRPC)
  target_include_directories(match_grpc PUBLIC src ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(match_grpc PUBLIC match_core gRPC::grpc++ protobuf::libprotobuf)

  add_executable(match_engine src/main.cpp)
  target_compile_definitions(match_engine PRIVATE HAS_GRPC)
  target_link_libraries(match_engine PRIVATE match_grpc)
endif()

# ── Tests (no gRPC needed — always build) ─────────────────────────────────
enable_testing()
add_executable(test_match_engine tests/test_match_engine.cpp)
target_link_libraries(test_match_engine PRIVATE match_core GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(test_match_engine)
