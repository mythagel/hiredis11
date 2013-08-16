CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(redis11)

IF(UNIX)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")
ENDIF()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

ADD_EXECUTABLE(redistest test.cpp)
TARGET_LINK_LIBRARIES(redistest hiredis)

ADD_EXECUTABLE(redistest_async ev_async.cpp)
TARGET_LINK_LIBRARIES(redistest_async hiredis ev)

ADD_EXECUTABLE(asio_test asio_test.cpp)
TARGET_LINK_LIBRARIES(asio_test boost_system-mt pthread)

ADD_EXECUTABLE(async_test async_test.cpp)
TARGET_LINK_LIBRARIES(async_test hiredis boost_system-mt pthread)
