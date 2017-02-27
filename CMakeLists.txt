cmake_minimum_required(VERSION 3.5)
cmake_policy(SET CMP0015 NEW)
cmake_policy(SET CMP0003 NEW)

ENABLE_LANGUAGE(C)
project(gbdxm CXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC")

# gbdxm version
set(gbdxm_VERSION_MAJOR 1)
set(gbdxm_VERSION_MINOR 0)
set(gbdxm_VERSION_PATCH 0)

# Set the build number
if(DEFINED ENV{BUILD_NUMBER})
    set(gbdxm_VERSION_BUILD $ENV{BUILD_NUMBER})
else()
    set(gbdxm_VERSION_BUILD "SNAPSHOT")
endif()

# Initialize gbdxversion.h
configure_file(gbdxmVersion.h.in include/gbdxmVersion.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

add_executable(gbdxm
        src/main.cpp
        src/gbdxm.h
        src/gbdxm.cpp)

find_package(DeepCore REQUIRED)
if (DeepCore_FOUND)
    include_directories(${DEEPCORE_INCLUDE_DIRS})
    target_link_libraries(gbdxm ${DEEPCORE_LIBRARIES})
    list(APPEND CMAKE_MODULE_PATH ${DEEPCORE_CMAKE_DIR})
endif()

find_package(Minizip REQUIRED)
if(MINIZIP_FOUND)
    include_directories(${MINIZIP_INCLUDE_DIRS})
    target_link_libraries(gbdxm ${MINIZIP_LIBRARIES})
endif()

find_package(ZLIB REQUIRED)
if(ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
    target_link_libraries(gbdxm ${ZLIB_LIBRARIES})
endif()

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost COMPONENTS program_options REQUIRED)
if(Boost_FOUND)
    target_link_libraries(gbdxm ${Boost_PROGRAM_OPTIONS_LIBRARY})
endif()

find_package(Jsoncpp REQUIRED)
if (JSONCPP_FOUND)
    include_directories(${JSONCPP_INCLUDE_DIR})
    target_link_libraries(gbdxm ${JSONCPP_LIBRARY})
endif()

find_package(Threads)
target_link_libraries(gbdxm ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})

INSTALL(TARGETS gbdxm
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib

        )

if (UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "TGZ")
    set(CPACK_PACKAGE_NAME "gbdxm")
    set(CPACK_PACKAGE_VENDOR "DigitalGlobe")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "DeepCore Support <deepcore-support@digitalglobe.com>")
    set(CPACK_DEBIAN_PACKAGE_VERSION "${gbdxm_VERSION_MAJOR}.${gbdxm_VERSION_MINOR}.${gbdxm_VERSION_PATCH}+${gbdxm_VERSION_BUILD}")
    set(CPACK_PACKAGE_VERSION "${gbdxm_VERSION_MAJOR}.${gbdxm_VERSION_MINOR}.${gbdxm_VERSION_PATCH}+${gbdxm_VERSION_BUILD}")
    set(CPACK_PACKAGE_VERSION_MAJOR "${gbdxm_VERSION_MAJOR}")
    set(CPACK_PACKAGE_VERSION_MINOR "${gbdxm_VERSION_MINOR}")
    set(CPACK_PACKAGE_VERSION_PATCH "${gbdxm_VERSION_PATCH}")
    set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
endif()
include(CPack)

