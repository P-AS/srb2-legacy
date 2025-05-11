# SRB2 Core

add_library(SRB2SDL2 SHARED)

# Core sources
target_sourcefile(c)
target_sources(SRB2SDL2 PRIVATE comptime.c md5.c config.h.in)

### Configuration
set(SRB2_CONFIG_EXTERNAL_ASSETS OFF CACHE BOOL
	"Bundle and hash external assets during configuration. Recommended when packaging.")
set(SRB2_CONFIG_HAVE_PNG ON CACHE BOOL
	"Enable PNG support. Depends on zlib, so will be disabled if you don't enable that too.")
set(SRB2_CONFIG_HAVE_ZLIB ON CACHE BOOL
	"Enable zlib support.")
set(SRB2_CONFIG_HAVE_GME OFF CACHE BOOL
	"Enable GME support.")
set(SRB2_CONFIG_HAVE_OPENMPT OFF CACHE BOOL
	"Enable OpenMPT support.")
set(SRB2_CONFIG_HAVE_THREADS OFF CACHE BOOL
	"Enable multithreading support.")
set(SRB2_CONFIG_HWRENDER OFF CACHE BOOL
	"Enable hardware rendering through OpenGL.")
set(SRB2_CONFIG_STATIC_OPENGL OFF CACHE BOOL
	"Use statically linked OpenGL. NOT RECOMMENDED.")
set(SRB2_CONFIG_DEDICATED OFF CACHE BOOL
	"Build a dedicated server.")
set(SRB2_CONFIG_DEBUGMODE OFF CACHE BOOL
	"Build a version with additional debug information.")

if(${SRB2_CONFIG_DEDICATED})
	set(SRB2_CONFIG_HAVE_GME NO)
	set(SRB2_CONFIG_HAVE_OPENMPT NO)
	set(SRB2_CONFIG_HWRENDER NO)
endif()

### use internal libraries?
if(${CMAKE_SYSTEM} MATCHES "Windows") ###set on Windows only
	set(SRB2_CONFIG_USE_INTERNAL_LIBRARIES OFF CACHE BOOL
	"Use SRB2's internal copies of required dependencies (SDL2, PNG, zlib, GME, OpenMPT).")
endif()

add_subdirectory(blua)

if(${SRB2_CONFIG_HAVE_GME})
	if(${SRB2_CONFIG_USE_INTERNAL_LIBRARIES})
		set(GME_FOUND ON)
		set(GME_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/libs/gme/include)
        if(${SRB2_SYSTEM_BITS} EQUAL 64)
			set(GME_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/gme/win64 -lgme")
		else() # 32-bit
			set(GME_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/gme/win32 -lgme")
		endif()
	else()
		find_package(GME)
	endif()
	if(${GME_FOUND})
		set(SRB2_HAVE_GME ON)
		target_compile_definitions(SRB2SDL2 PRIVATE -DHAVE_GME)
	else()
		message(WARNING "You have specified that GME is available but it was not found.")
	endif()
endif()

if(${SRB2_CONFIG_HAVE_OPENMPT})
	if(${SRB2_CONFIG_USE_INTERNAL_LIBRARIES})
		set(OPENMPT_FOUND ON)
		set(OPENMPT_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/libs/libopenmpt/inc)
        if(${SRB2_SYSTEM_BITS} EQUAL 64)
			set(OPENMPT_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/libopenmpt/lib/x86_64/mingw -lopenmpt")
		else() # 32-bit
			set(OPENMPT_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/libopenmpt/lib/x86/mingw -lopenmpt")
		endif()
	else()
		find_package(OPENMPT)
	endif()
	if(${OPENMPT_FOUND})
		set(SRB2_HAVE_OPENMPT ON)
		target_compile_definitions(SRB2SDL2 PRIVATE -DHAVE_OPENMPT)
	else()
		message(WARNING "You have specified that OpenMPT is available but it was not found.")
	endif()
endif()

if(${SRB2_CONFIG_HAVE_THREADS})
	set(SRB2_HAVE_THREADS ON)
	target_sources(SRB2SDL2 PRIVATE i_threads.c)
	target_compile_definitions(SRB2SDL2 PRIVATE -DHAVE_THREADS)
endif()

if(${SRB2_CONFIG_HAVE_ZLIB})
	set(ZLIB_FOUND ON)
	add_definitions(-DHAVE_ZLIB)
endif()

if(${SRB2_CONFIG_HAVE_PNG} AND ${SRB2_CONFIG_HAVE_ZLIB})
	if (${ZLIB_FOUND})
		set(PNG_FOUND ON)
		if(${PNG_FOUND})
			set(SRB2_HAVE_PNG ON)
			add_definitions(-DHAVE_PNG)
			add_definitions(-D_LARGEFILE64_SOURCE)
			set(PNG_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/thirdparty/libpng")

		else()
			message(WARNING "You have specified that PNG is available but it was not found. SRB2 may not compile correctly.")
		endif()
	endif()
endif()

if(${SRB2_CONFIG_HAVE_CURL})
	set(CURL_FOUND ON)
	set(CURL_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/thirdparty/curl/include)
	set(CURL_LIBRARIES "-L${CMAKE_SOURCE_DIR}/thirdparty/curl/libs/arm64-v8a -lcurl")
	#target_link_libraries(SRB2 PRIVATE CURL::curl)
	set(SRB2_HAVE_CURL ON)
	add_definitions(-DHAVE_CURL)
endif()

if(${SRB2_CONFIG_HWRENDER})
	target_compile_definitions(SRB2SDL2 PRIVATE -DHWRENDER)
	add_subdirectory(hardware)
endif()

if(${SRB2_CONFIG_HWRENDER} AND ${SRB2_CONFIG_STATIC_OPENGL})
	find_package(OpenGL)
	if(${OPENGL_FOUND})
		target_compile_definitions(SRB2SDL2 PRIVATE -DHWRENDER)
		target_compile_definitions(SRB2SDL2 PRIVATE -DSTATIC_OPENGL)
	else()
		message(WARNING "You have specified static opengl but opengl was not found. Not setting HWRENDER.")
	endif()
endif()

# Targets

# Compatibility flag with later versions of GCC
# We should really fix our code to not need this
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -mno-ms-bitfields)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -Wno-absolute-value)
endif()

target_compile_definitions(SRB2SDL2 PRIVATE -DCMAKECONFIG)

if(${SRB2_CONFIG_DEBUGMODE})
	target_compile_definitions(SRB2SDL2 PRIVATE -D_DEBUG -DPARANOIA -DRANGECHECK)
else()
	target_compile_definitions(SRB2SDL2 PRIVATE -DNDEBUG)
endif()

target_compile_definitions(SRB2SDL2 PRIVATE -DCMAKECONFIG -DNOMUMBLE -DSTDC_HEADERS)

#add_library(SRB2Core STATIC
#	${SRB2_CORE_SOURCES}
#	${SRB2_CORE_HEADERS}
#	${SRB2_CORE_RENDER_SOURCES}
#	${SRB2_CORE_GAME_SOURCES}
#	${SRB2_LUA_SOURCES}
#	${SRB2_LUA_HEADERS}
#	${SRB2_BLUA_SOURCES}
#	${SRB2_BLUA_HEADERS}
#)

if(${SRB2_CONFIG_DEDICATED})
	add_subdirectory(dedicated)
else()
	add_subdirectory(sdl)
endif()

if(${CMAKE_SYSTEM} MATCHES Windows)
	add_subdirectory(win32)
endif()

add_subdirectory(android-jni)

if(NOT ${SRB2_SDL2_AVAILABLE} AND NOT ${SRB2_WIN32_AVAILABLE})
	message(FATAL_ERROR "There are no targets available to build an SRB2 executable. :(")
endif()

# strip debug symbols into separate file when using gcc.
# to be consistent with Makefile, don't generate for OS X.
if((CMAKE_COMPILER_IS_GNUCC) AND NOT ("${CMAKE_SYSTEM_NAME}" MATCHES Darwin))
	if((${CMAKE_BUILD_TYPE} MATCHES Debug) OR (${CMAKE_BUILD_TYPE} MATCHES RelWithDebInfo))
		if(${CMAKE_BUILD_TYPE} MATCHES Debug)
			set(OBJCOPY_ONLY_KEEP_DEBUG "--only-keep-debug")
		endif()
		message(STATUS "Will make separate debug symbols in *.debug")
		add_custom_command(TARGET SRB2SDL2 POST_BUILD
			COMMAND ${OBJCOPY} ${OBJCOPY_ONLY_KEEP_DEBUG} $<TARGET_FILE:SRB2SDL2> $<TARGET_FILE:SRB2SDL2>.debug
			COMMAND ${OBJCOPY} --strip-debug $<TARGET_FILE:SRB2SDL2>
			COMMAND ${OBJCOPY} --add-gnu-debuglink=$<TARGET_FILE:SRB2SDL2>.debug $<TARGET_FILE:SRB2SDL2>
		)
	endif()
endif()
