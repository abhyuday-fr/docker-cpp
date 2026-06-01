include(CMakeFindDependencyMacro)
find_dependency(Threads REQUIRED)
include("${CMAKE_CURRENT_LIST_DIR}/docker-cpp-targets.cmake")
