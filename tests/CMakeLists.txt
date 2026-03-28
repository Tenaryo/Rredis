cmake_minimum_required(VERSION 3.13)

file(GLOB TEST_SOURCES *.cpp)
file(GLOB STORE_SOURCES ${CMAKE_SOURCE_DIR}/src/store/*.cpp)
file(GLOB BLOCK_SOURCES ${CMAKE_SOURCE_DIR}/src/block_manager/*.cpp)
file(GLOB HANDLER_SOURCES ${CMAKE_SOURCE_DIR}/src/handler/*.cpp)
file(GLOB PROTOCOL_SOURCES ${CMAKE_SOURCE_DIR}/src/protocol/*.cpp)

foreach(test_source ${TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source} ${STORE_SOURCES} ${BLOCK_SOURCES} ${HANDLER_SOURCES} ${PROTOCOL_SOURCES})
    
    target_link_libraries(${test_name} PRIVATE Threads::Threads)
    target_include_directories(${test_name} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_include_directories(${test_name} PRIVATE ${ASIO_INCLUDE_DIR})
    
    set(CMAKE_CXX_STANDARD 23)
    
    target_compile_options(${test_name} PRIVATE -UNDEBUG -g -O0)
endforeach()
