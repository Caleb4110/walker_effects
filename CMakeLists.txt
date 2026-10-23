cmake_minimum_required(VERSION 3.22)
project(WalkerEffects VERSION 0.1.0)

cmake_minimum_required(VERSION 3.22)
project(WalkerEffects VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(external/JUCE)

include(cmake/WalkerPlugin.cmake)
include(cmake/Tools.cmake)

add_subdirectory(components)
add_subdirectory(plugins)
