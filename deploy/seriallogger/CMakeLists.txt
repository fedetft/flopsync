
cmake_minimum_required(VERSION 2.6)
project(SERIALLLOGGER)

## Target
set(SERIALLLOGGER_SRCS main.cpp ../lib/serialstream.cpp)
add_executable(seriallogger ${SERIALLLOGGER_SRCS})
include_directories(../lib)

## Link libraries
set(BOOST_LIBS thread date_time system program_options)
find_package(Boost COMPONENTS ${BOOST_LIBS} REQUIRED)
target_link_libraries(seriallogger ${Boost_LIBRARIES})
find_package(Threads REQUIRED)
target_link_libraries(seriallogger ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS seriallogger DESTINATION bin)
