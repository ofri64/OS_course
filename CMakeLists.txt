cmake_minimum_required(VERSION 3.8)
project(OS_course)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")

set(SOURCE_FILES
#        hw5/pcc_server.c hw5/pcc_server.h
        hw5/pcc_client.c hw5/pcc_client.h
#        hw5/function_checks.c
        )
add_executable(OS_course ${SOURCE_FILES})