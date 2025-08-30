# Git utilities

if(__GitUtilities)
	return()
endif()

set(__GitUtilities ON)

macro(_git_command)
	execute_process(
		COMMAND "${GIT_EXECUTABLE}" ${ARGN}
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		OUTPUT_VARIABLE output
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
endmacro()

macro(_git_easy_command)
	_git_command(${ARGN})
	set(${variable} "${output}" PARENT_SCOPE)
endmacro()

function(git_describe variable path)
	execute_process(COMMAND "${GIT_EXECUTABLE}" "describe"
		WORKING_DIRECTORY "${path}"
		RESULT_VARIABLE result
		OUTPUT_VARIABLE output
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	set(${variable} "${output}" PARENT_SCOPE)
endfunction()

function(git_current_branch variable path)
	execute_process(COMMAND ${GIT_EXECUTABLE} "symbolic-ref" "--short" "HEAD"
		WORKING_DIRECTORY "${path}"
		RESULT_VARIABLE result
		OUTPUT_VARIABLE output
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	set(${variable} "${output}" PARENT_SCOPE)
endfunction()

function(git_latest_commit variable path)
	execute_process(COMMAND ${GIT_EXECUTABLE} "rev-parse" "--short" "HEAD"
		WORKING_DIRECTORY "${path}"
		RESULT_VARIABLE result
		OUTPUT_VARIABLE output
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	set(${variable} "${output}" PARENT_SCOPE)
endfunction()


function(git_subject variable)
	_git_easy_command(log -1 --format=%s)
endfunction()
