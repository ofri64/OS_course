cmake_minimum_required(VERSION 3.8)
project(OS_course)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        hw2/myshell.c
        hw2/myshell_main.c hw3/message_slot/message_slot.h hw3/message_slot/message_slot.c)

add_executable(OS_course ${SOURCE_FILES})