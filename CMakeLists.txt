cmake_minimum_required(VERSION 3.15)

project(docker-cpp
  VERSION 0.1.0
  DESCRIPTION "Header-only C++ SDK for the Docker Engine API"
  LANGUAGES CXX)

# Dependencies

include(FetchContent)
FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

find_package(Threads REQUIRED)

# Library target
# INTERFACE = header-only. No compiled objects

add_library(docker-cpp INTERFACE)
add_library(docker-cpp::docker-cpp ALIAS docker-cpp)

target_include_directories(docker-cpp INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${nlohmann_json_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_compile_features(docker-cpp INTERFACE cxx_std_17)

target_link_libraries(docker-cpp INTERFACE
  Threads::Threads
)

# Tests
# only built when this is the top-level project, not when used via add_subdirectory or FetchContent by another project

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  add_executable(phase6_test tests/phase6_test.cpp)
  target_link_libraries(phase6_test PRIVATE docker-cpp::docker-cpp)

  add_executable(phase7_test tests/phase7_test.cpp)
  target_link_libraries(phase7_test PRIVATE docker-cpp::docker-cpp)

  add_executable(phase8_test tests/phase8_test.cpp)
  target_link_libraries(phase8_test PRIVATE docker-cpp::docker-cpp)

  add_executable(unit_test tests/unit_test.cpp)
  target_link_libraries(unit_test PRIVATE docker-cpp::docker-cpp)
endif()

# Install rules

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS docker-cpp
  EXPORT docker-cpp-targets
)

install (DIRECTORY include/docker
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install (EXPORT docker-cpp-targets
  FILE docker-cpp-targets.cmake
  NAMESPACE docker-cpp::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/docker-cpp
)

write_basic_package_version_file(
  docker-cpp-config-version.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  cmake/docker-cpp-config.cmake
  ${CMAKE_BINARY_DIR}/docker-cpp-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/docker-cpp
)
