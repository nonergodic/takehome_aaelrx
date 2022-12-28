cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(signing_service)

# set(CMAKE_C_COMPILER gcc)
# set(CMAKE_CPP_COMPILER g++)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")

# set(Boost_USE_STATIC_LIBS        ON)  # only find static libs
# set(Boost_USE_DEBUG_LIBS        OFF)  # ignore debug libs and
# set(Boost_USE_RELEASE_LIBS       ON)  # only find release libs
# set(Boost_USE_MULTITHREADED      ON)
# set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost 1.81 REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIR})

# Add SQLite3 C++ wrapper around sqlite3 library (and sqlite3 itself provided for ease of use)
# Here you can set CMake variables to avoid building Example, as well as cpplint, cppcheck...
# or set them in the cmake command line (see for instance provided build.bat/build.sh scripts)
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "" FORCE)
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/SQLiteCpp)

add_executable(signing_service src/main.cpp src/startup.cpp)

target_link_libraries(signing_service cryptopp SQLiteCpp ${Boost_LIBRARIES})
