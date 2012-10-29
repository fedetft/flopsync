
cmake_minimum_required(VERSION 2.6)
project(EVENTGEN)

## Target
set(EVENTGEN_SRCS main.cpp ../lib/serialstream.cpp)
add_executable(eventgen ${EVENTGEN_SRCS})
include_directories(../lib)

## Link libraries
set(BOOST_LIBS thread date_time system program_options random)
find_package(Boost COMPONENTS ${BOOST_LIBS} REQUIRED)
target_link_libraries(eventgen ${Boost_LIBRARIES})
find_package(Threads REQUIRED)
target_link_libraries(eventgen ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS eventgen DESTINATION bin)
