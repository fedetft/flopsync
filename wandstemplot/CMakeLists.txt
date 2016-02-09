
project(WANDSTEMPLOT)
cmake_minimum_required(VERSION 2.6)

set(WANDSTEMPLOT_SRCS main.cpp window.cpp AsyncSerial.cpp QAsyncSerial.cpp)
set(WANDSTEMPLOT_HEADERS window.h AsyncSerial.h QAsyncSerial.h)
set (CMAKE_CXX_FLAGS "--std=gnu++11 ${CMAKE_CXX_FLAGS}")

find_package(Qt REQUIRED)
find_library(QWT qwt)
include(${QT_USE_FILE})
qt4_wrap_cpp(WANDSTEMPLOT_HEADERS_MOC ${WANDSTEMPLOT_HEADERS})
if(APPLE)
add_executable(wandstemplot MACOSX_BUNDLE ${WANDSTEMPLOT_SRCS} ${WANDSTEMPLOT_HEADERS_MOC})
else()
add_executable(wandstemplot ${WANDSTEMPLOT_SRCS} ${WANDSTEMPLOT_HEADERS_MOC})
endif()
target_link_libraries(wandstemplot ${QT_LIBRARIES} ${QWT})

set(BOOST_LIBS thread system)
find_package(Boost COMPONENTS ${BOOST_LIBS} REQUIRED)
target_link_libraries(wandstemplot ${Boost_LIBRARIES})
find_package(Threads REQUIRED)
target_link_libraries(wandstemplot ${CMAKE_THREAD_LIBS_INIT})
