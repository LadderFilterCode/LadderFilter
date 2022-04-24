cmake_minimum_required (VERSION 2.6)
project(LadderFilter)

if(CMAKE_COMPILER_IS_GNUCXX)
    message(STATUS "GCC detected, adding compile flags")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -w")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2 -mssse3 -msse4.1 -msse4.2 -mavx -mbmi -march=native")
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -O3")
    ADD_DEFINITIONS(-DUNIX)
endif(CMAKE_COMPILER_IS_GNUCXX)

if (UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
endif(UNIX)

ADD_DEFINITIONS(-DPROJECT_ROOT="${CMAKE_SOURCE_DIR}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(SOURCE_FILES src/main.cpp)

add_executable(demo src/main.cpp)
