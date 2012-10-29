
cmake_minimum_required(VERSION 2.6)
project(FWUPGRADE)

## Target
set(FWUPGRADE_SRCS main.cpp ../lib/serialstream.cpp)
add_executable(fwupgrade ${FWUPGRADE_SRCS})
include_directories(../lib)

## Link libraries
set(BOOST_LIBS thread date_time system program_options)
find_package(Boost COMPONENTS ${BOOST_LIBS} REQUIRED)
target_link_libraries(fwupgrade ${Boost_LIBRARIES})
find_package(Threads REQUIRED)
target_link_libraries(fwupgrade ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS fwupgrade DESTINATION bin)
