# @author hoxnox <hoxnox@gmail.com>
# @date 20160531 16:13:29
# procman cmake tests build script

find_package(Threads)
list(APPEND LIBRARIES ${CMAKE_THREAD_LIBS_INIT})

if (NOT USE_SYSTEM_GTEST)
    include(ExternalProject)
    if(NOT GTEST_SRC)
        set(GTEST_SRC $ENV{GTEST})
    endif()
    if(NOT GTEST_SRC)
        set(GTEST_SRC https://github.com/google/googletest/archive/release-1.7.0.tar.gz)
    endif()
    ExternalProject_Add(
        gtestlib
        URL ${GTEST_SRC}
        PREFIX "${CMAKE_CURRENT_BINARY_DIR}/gtest"
        CMAKE_ARGS -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
            -DBUILD_SHARED_LIBS=False
            -DCMAKE_CXX_COMPILER:STRING='${CMAKE_CXX_COMPILER}'
            -DCMAKE_CXX_FLAGS:STRING='${CMAKE_CXX_FLAGS}'
        INSTALL_COMMAND ""
        BUILD_IN_SOURCE 1
        LOG_DOWNLOAD 1
        LOG_UPDATE 1
        LOG_CONFIGURE 1
        LOG_BUILD 1
        LOG_TEST 1
        LOG_INSTALL 1
    )
    ExternalProject_Get_Property(gtestlib BINARY_DIR)
    ExternalProject_Get_Property(gtestlib SOURCE_DIR)
    set(GTEST_INCLUDE_DIRS ${SOURCE_DIR}/include)
    set(GTEST_LIBRARIES ${BINARY_DIR}/libgtest.a)
else ()
    find_package(GTest REQUIRED)
endif()
include_directories(${GTEST_INCLUDE_DIRS})
list(APPEND LIBRARIES ${GTEST_LIBRARIES})

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/test_all" "#!/bin/sh\n\n")
function(test n)
	add_executable(${n} ../src/procman.cpp test.cpp)
	set_property(TARGET ${n} PROPERTY COMPILE_DEFINITIONS ${n})
	add_dependencies(${n} gtestlib)
	target_link_libraries(${n} ${LIBRARIES})
	file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/test_all" "./test/${n}\n")
endfunction()

test(test_stop_normal)
test(test_stop_signal)
test(test_pid_file)

file(INSTALL "${CMAKE_CURRENT_BINARY_DIR}/test_all"
	DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/.."
	FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)

