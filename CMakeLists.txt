# @author hoxnox <hoxnox@gmail.com>
# @date 20160531 16:13:29
# procman cmake build script

cmake_minimum_required(VERSION 2.6.4)

########################################################################
# options

option(WITH_TESTS "Build tests."  OFF)
option(WITH_DOCS  "Generate docs" OFF)

########################################################################
# general

project(procman)

set(procman_VERSION_MAJOR 0)
set(procman_VERSION_MINOR 0)
set(procman_VERSION_PATCH 2)
# configuration header
configure_file(
	"${PROJECT_SOURCE_DIR}/src/procman_config.cfg"
	"${PROJECT_SOURCE_DIR}/src/procman_config.h"
	ESCAPE_QUOTES
)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}"
                      "${PROJECT_SOURCE_DIR}/cmake/Modules")

include_directories("${PROJECT_SOURCE_DIR}/include")
add_definitions("-std=c++11")

########################################################################
# procman

add_library(procman ./src/procman.cpp)

########################################################################
# tests and docs

if(WITH_DOCS)
	add_subdirectory(doc)
endif()

if(WITH_TESTS)
	enable_testing()
	add_subdirectory(test)
endif()

########################################################################
# installation

set(CPACK_SET_DESTDIR ON)

INSTALL(TARGETS procman DESTINATION lib)
INSTALL(FILES include/procman.hpp DESTINATION include
	PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ)
SET(CPACK_PACKAGE_NAME procman)
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "")
SET(CPACK_PACKAGE_VENDOR "Kim Merder <hoxnox@gmail.com>")
SET(CPACK_PACKAGE_CONTACT ${CPACK_PACKAGE_VENDOR})
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README.md")
SET(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
SET(CPACK_PACKAGE_VERSION_MAJOR ${procman_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${procman_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${procman_VERSION_PATCH})
SET(CPACK_DEBIAN_PACKAGE_SECTION "misc")
SET(CPACK_RPM_PACKAGE_GROUP "Applications/System")
SET(CPACK_RPM_PACKAGE_LICENSE "MIT")
SET(CPACK_STRIP_FILES TRUE)
INCLUDE(CPack)

