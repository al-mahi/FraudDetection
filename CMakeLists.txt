cmake_minimum_required(VERSION 3.15)
project(FraudDetection)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(MySQL REQUIRED)
add_executable(fraud_detector src/main.cpp include/mysql_helper.h)
target_link_libraries(fraud_detector ${MYSQL_LIBRARY})


Include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v2.13.1)

FetchContent_MakeAvailable(Catch2)

add_executable(tests tests/test.cpp)
target_link_libraries(tests PRIVATE Catch2::Catch2)