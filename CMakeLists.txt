cmake_minimum_required(VERSION 3.20)
project(SysTrace VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0A00)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
endif()

set(SOURCES
    main.cpp
    src/collector.cpp
    src/http_server.cpp
    src/json_serializer.cpp
    src/persistence.cpp
)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party/httplib
)

add_executable(SysTrace ${SOURCES})

if(WIN32)
    target_link_libraries(SysTrace PRIVATE
        psapi
        kernel32
        ws2_32
        pdh
        iphlpapi
    )
endif()

if(MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MINGW)
        target_compile_options(SysTrace PRIVATE -O2 -s)
    endif()
endif()