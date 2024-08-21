cmake_minimum_required(VERSION 3.10)
project(WebServer)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -g")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(${PROJECT_SOURCE_DIR}/code)
add_subdirectory(code/buffer)
add_subdirectory(code/http)
add_subdirectory(code/log)
add_subdirectory(code/pool)
add_subdirectory(code/server)
add_subdirectory(code/timer)
add_subdirectory(test)

set(MAIN_SOURCE ${PROJECT_SOURCE_DIR}/code/main.cpp)

find_library(MYSQLCLIENT_LIBRARY mysqlclient)
set(ALL_LIBS ${ALL_LIBS} pthread ${MYSQLCLIENT_LIBRARY})

add_executable(server ${MAIN_SOURCE})
target_link_libraries(server ${ALL_LIBS})

# add_executable(test ${TEST_SOURCES})
# target_link_libraries(test ${ALL_LIBS})

add_custom_command(TARGET server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${PROJECT_SOURCE_DIR}/resources $<TARGET_FILE_DIR:server>/../resources)

add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/resources
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/CMakeFiles
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/CMakeCache.txt
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/cmake_install.cmake
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/Makefile
)