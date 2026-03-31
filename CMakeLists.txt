cmake_minimum_required(VERSION 3.16)
project(simplecomputer C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(include)

add_library(mySimpleComputer mySimpleComputer/mySimpleComputer.c)
add_library(myTerm myTerm/myTerm.c)
add_library(myReadKey myReadKey/myReadKey.c)
add_library(myBigChars myBigChars/myBigChars.c)
add_library(simpleassembler_lib simpleassembler/simpleassembler.c)
add_library(simplebasic_lib simplebasic/simplebasic.c)

add_executable(console console/main.c)
target_link_libraries(console PRIVATE mySimpleComputer myTerm myReadKey myBigChars simpleassembler_lib simplebasic_lib)

add_executable(simpleassembler simpleassembler/main.c)
target_link_libraries(simpleassembler PRIVATE simpleassembler_lib mySimpleComputer)

add_executable(simplebasic simplebasic/main.c)
target_link_libraries(simplebasic PRIVATE simplebasic_lib)
