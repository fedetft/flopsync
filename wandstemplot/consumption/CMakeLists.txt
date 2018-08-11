
project(WANDSTEMPLOT)
cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_AUTOMOC ON)

set(WANDSTEMPLOT_SRCS main.cpp window.cpp AsyncSerial.cpp QAsyncSerial.cpp)
set(WANDSTEMPLOT_HEADERS window.h AsyncSerial.h QAsyncSerial.h)
add_executable(wandstemplot ${WANDSTEMPLOT_SRCS})

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
find_library(QWT qwt-qt5)
target_link_libraries(wandstemplot Qt5::Widgets ${QWT})

set(BOOST_LIBS system)
find_package(Boost COMPONENTS ${BOOST_LIBS} REQUIRED)
target_link_libraries(wandstemplot ${Boost_LIBRARIES})
find_package(Threads REQUIRED)
target_link_libraries(wandstemplot ${CMAKE_THREAD_LIBS_INIT})
