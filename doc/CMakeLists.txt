# @author $username$ <$usermail$>
# @date $date$
# procman documentation build script

find_package(Doxygen)
if(DOXYGEN_FOUND)
	set(ENV{procman_ROOT} "${PROJECT_SOURCE_DIR}")
	add_custom_target(doc COMMAND ${CMAKE_COMMAND}
		-D PDIR="${PROJECT_SOURCE_DIR}"
		-P "${PROJECT_SOURCE_DIR}/doc/doxygen.cmake")
else()
	message(WARNING
		"Doxygen not found."
		" Documentation wouldn't be ganarated")
endif()

