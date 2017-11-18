cmake_minimum_required(VERSION 3.8)
project(OS_course)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        hw1/hw1_subs.c
        hw2/myshell.c)

add_executable(OS_course ${SOURCE_FILES})