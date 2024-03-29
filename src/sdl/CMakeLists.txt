# Declare SDL2 interface sources

set(SRB2_CONFIG_SDL2_USEMIXER ON CACHE BOOL "Use SDL2_mixer or regular sdl sound")

if(${SRB2_CONFIG_SDL2_USEMIXER})
	if(${SRB2_CONFIG_USE_INTERNAL_LIBRARIES})
		set(SDL2_MIXER_FOUND ON)
		if(${SRB2_SYSTEM_BITS} EQUAL 64)
			set(SDL2_MIXER_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/libs/SDL2_mixer/x86_64-w64-mingw32/include/SDL2)
			set(SDL2_MIXER_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/SDL2_mixer/x86_64-w64-mingw32/lib -lSDL2_mixer")
		else() # 32-bit
			set(SDL2_MIXER_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/libs/SDL2_mixer/i686-w64-mingw32/include/SDL2)
			set(SDL2_MIXER_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/SDL2_mixer/i686-w64-mingw32/lib -lSDL2_mixer")
		endif()
	else()
		find_package(SDL2_mixer)
	endif()
	if(${SDL2_MIXER_FOUND})
		set(SRB2_HAVE_MIXER ON)
		set(SRB2_SDL2_SOUNDIMPL mixer_sound.c)
	else()
		message(WARNING "You specified that SDL2_mixer is available, but it was not found. Falling back to sdl sound.")
		set(SRB2_SDL2_SOUNDIMPL sdl_sound.c)
	endif()
else()
	set(SRB2_SDL2_SOUNDIMPL sdl_sound.c)
endif()

set(SRB2_SDL2_SOURCES
	dosstr.c
	endtxt.c
	hwsym_sdl.c
	i_cdmus.c
	i_main.c
	i_net.c
	i_system.c
	i_ttf.c
	i_video.c
	#IMG_xpm.c
	ogl_sdl.c

	${SRB2_SDL2_SOUNDIMPL}
)

set(SRB2_SDL2_HEADERS
	endtxt.h
	hwsym_sdl.h
	i_ttf.h
	ogl_sdl.h
	sdlmain.h
)

source_group("Interface Code" FILES ${SRB2_SDL2_SOURCES} ${SRB2_SDL2_HEADERS})

# Dependency
if(${SRB2_CONFIG_USE_INTERNAL_LIBRARIES})
	set(SDL2_FOUND ON)
	if(${SRB2_SYSTEM_BITS} EQUAL 64)
		set(SDL2_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/libs/SDL2/x86_64-w64-mingw32/include/SDL2)
		set(SDL2_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/SDL2/x86_64-w64-mingw32/lib -lSDL2")
	else() # 32-bit
		set(SDL2_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/libs/SDL2/i686-w64-mingw32/include/SDL2)
		set(SDL2_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/SDL2/i686-w64-mingw32/lib -lSDL2")
	endif()
else()
	find_package(SDL2)
endif()

if(${SDL2_FOUND})
	set(SRB2_SDL2_TOTAL_SOURCES
		${SRB2_CORE_SOURCES}
		${SRB2_CORE_HEADERS}
		${SRB2_PNG_SOURCES}
		${SRB2_PNG_HEADERS}
		${SRB2_CORE_RENDER_SOURCES}
		${SRB2_CORE_GAME_SOURCES}
		${SRB2_LUA_SOURCES}
		${SRB2_LUA_HEADERS}
		${SRB2_BLUA_SOURCES}
		${SRB2_BLUA_HEADERS}
		${SRB2_SDL2_SOURCES}
		${SRB2_SDL2_HEADERS}
	)

	source_group("Main" FILES ${SRB2_CORE_SOURCES} ${SRB2_CORE_HEADERS}
		${SRB2_PNG_SOURCES} ${SRB2_PNG_HEADERS})
	source_group("Renderer" FILES ${SRB2_CORE_RENDER_SOURCES})
	source_group("Game" FILES ${SRB2_CORE_GAME_SOURCES})
	source_group("Assembly" FILES ${SRB2_ASM_SOURCES} ${SRB2_NASM_SOURCES})
	source_group("LUA" FILES ${SRB2_LUA_SOURCES} ${SRB2_LUA_HEADERS})
	source_group("LUA\\Interpreter" FILES ${SRB2_BLUA_SOURCES} ${SRB2_BLUA_HEADERS})

	if(${SRB2_CONFIG_HWRENDER})
		set(SRB2_SDL2_TOTAL_SOURCES ${SRB2_SDL2_TOTAL_SOURCES}
			${SRB2_HWRENDER_SOURCES}
			${SRB2_HWRENDER_HEADERS}
			${SRB2_R_OPENGL_SOURCES}
			${SRB2_R_OPENGL_HEADERS}
		)

		source_group("Hardware" FILES ${SRB2_HWRENDER_SOURCES} ${SRB2_HWRENDER_HEADERS})
		source_group("Hardware\\OpenGL Renderer" FILES ${SRB2_R_OPENGL_SOURCES} ${SRB2_R_OPENGL_HEADERS})
	endif()

	if(${SRB2_USEASM})
		set(SRB2_SDL2_TOTAL_SOURCES ${SRB2_SDL2_TOTAL_SOURCES}
			${SRB2_NASM_SOURCES}
		)
		if(MSVC)
			set(SRB2_SDL2_TOTAL_SOURCES ${SRB2_SDL2_TOTAL_SOURCES}
				${SRB2_NASM_OBJECTS}
			)
			set_source_files_properties(${SRB2_NASM_OBJECTS} PROPERTIES GENERATED ON)
		else()
			list(APPEND SRB2_SDL2_TOTAL_SOURCES ${SRB2_ASM_SOURCES})
			set_source_files_properties(${SRB2_ASM_SOURCES} PROPERTIES LANGUAGE C)
			set_source_files_properties(${SRB2_ASM_SOURCES} PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")
		endif()
	endif()

	if(${CMAKE_SYSTEM} MATCHES Windows)
		set(SRB2_SDL2_TOTAL_SOURCES ${SRB2_SDL2_TOTAL_SOURCES}
			${CMAKE_SOURCE_DIR}/src/win32/win_dbg.c
			${CMAKE_SOURCE_DIR}/src/win32/Srb2win.rc
		)
	endif()

	if(${CMAKE_SYSTEM} MATCHES Darwin)
		set(MACOSX_BUNDLE_ICON_FILE Srb2mac.icns)
		set_source_files_properties(macosx/Srb2mac.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
		set(SRB2_SDL2_MAC_SOURCES
			macosx/mac_alert.c
			macosx/mac_alert.h
			macosx/mac_resources.c
			macosx/mac_resources.h
			macosx/Srb2mac.icns
		)
		source_group("Interface Code\\OSX Compatibility" FILES ${SRB2_SDL2_MAC_SOURCES})
		set(SRB2_SDL2_TOTAL_SOURCES ${SRB2_SDL2_TOTAL_SOURCES} ${SRB2_SDL2_MAC_SOURCES})
	endif()

	add_executable(SRB2SDL2 MACOSX_BUNDLE WIN32 ${SRB2_SDL2_TOTAL_SOURCES})
	set_target_properties(SRB2SDL2 PROPERTIES OUTPUT_NAME ${SRB2_SDL2_EXE_NAME})

	if(${CMAKE_SYSTEM} MATCHES Darwin)
		find_library(CORE_LIB CoreFoundation)
		target_link_libraries(SRB2SDL2 PRIVATE
			${CORE_LIB}
			SDL2
			SDL2_mixer
			${GME_LIBRARIES}
			${OPENMPT_LIBRARIES}
			${PNG_LIBRARIES}
			${ZLIB_LIBRARIES}
			${OPENGL_LIBRARIES}
		)
		set_target_properties(SRB2SDL2 PROPERTIES OUTPUT_NAME "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}")
	else()
		target_link_libraries(SRB2SDL2 PRIVATE
			${SDL2_LIBRARIES}
			${SDL2_MIXER_LIBRARIES}
			${GME_LIBRARIES}
			${OPENMPT_LIBRARIES}
			${PNG_LIBRARIES}
			${ZLIB_LIBRARIES}
			${OPENGL_LIBRARIES}
		)

		if(${CMAKE_SYSTEM} MATCHES Linux)
			target_link_libraries(SRB2SDL2 PRIVATE
				m
				rt
			)
		endif()
	endif()

	#target_link_libraries(SRB2SDL2 PRIVATE SRB2Core)

	if(${SRB2_USEASM})
		if(${SRB2_CONFIG_YASM})
			set(ASM_ASSEMBLER_TEMP ${CMAKE_ASM_YASM_COMPILER})
			set(ASM_ASSEMBLER_OBJFORMAT ${CMAKE_ASM_YASM_OBJECT_FORMAT})
			set_source_files_properties(${SRB2_NASM_SOURCES} LANGUAGE ASM_YASM)
		else()
			set(ASM_ASSEMBLER_TEMP ${CMAKE_ASM_NASM_COMPILER})
			set(ASM_ASSEMBLER_OBJFORMAT ${CMAKE_ASM_NASM_OBJECT_FORMAT})
			set_source_files_properties(${SRB2_NASM_SOURCES} LANGUAGE ASM_NASM)
		endif()

		if(MSVC)
			# using assembler with msvc doesn't work, must do it manually
			foreach(ASMFILE ${SRB2_NASM_SOURCES})
				get_filename_component(ASMFILE_NAME ${ASMFILE} NAME_WE)
				set(ASMFILE_NAME ${ASMFILE_NAME}.obj)
				add_custom_command(TARGET SRB2SDL2 PRE_LINK
					COMMAND ${ASM_ASSEMBLER_TEMP} ARGS -f ${ASM_ASSEMBLER_OBJFORMAT} -o ${CMAKE_CURRENT_BINARY_DIR}/${ASMFILE_NAME} ${ASMFILE}
					COMMENT "assemble ${ASMFILE_NAME}."
				)
			endforeach()
		endif()
	endif()

	set_target_properties(SRB2SDL2 PROPERTIES VERSION ${SRB2_VERSION})

	if(${CMAKE_SYSTEM} MATCHES Windows)
		target_link_libraries(SRB2SDL2 PRIVATE
			ws2_32
		)
		target_compile_options(SRB2SDL2 PRIVATE
			-U_WINDOWS
		)
	endif()

	if(MSVC)
		if(${SRB2_CONFIG_USE_INTERNAL_LIBRARIES})
			set(SDL2_MAIN_FOUND ON)
			if(${SRB2_SYSTEM_BITS} EQUAL 64)
				set(SDL2_MAIN_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/libs/SDL2/x86_64-w64-mingw32/include/SDL2)
				set(SDL2_MAIN_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/SDL2/x86_64-w64-mingw32/lib -lSDL2main")
			else() # 32-bit
				set(SDL2_MAIN_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/libs/SDL2/i686-w64-mingw32/include/SDL2)
				set(SDL2_MAIN_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/SDL2/i686-w64-mingw32/lib -lSDL2main")
			endif()
		else()
			find_package(SDL2_MAIN REQUIRED)
		endif()
		target_link_libraries(SRB2SDL2 PRIVATE
			${SDL2_MAIN_LIBRARIES}
		)
		target_compile_options(SRB2SDL2 PRIVATE
			/Umain
			/D_CRT_SECURE_NO_WARNINGS # something about string functions.
			/D_CRT_NONSTDC_NO_DEPRECATE
			/DSDLMAIN
			/D_WINSOCK_DEPRECATED_NO_WARNINGS # Don't care
		)
	endif()

	target_include_directories(SRB2SDL2 PRIVATE
		${SDL2_INCLUDE_DIRS}
		${SDL2_MIXER_INCLUDE_DIRS}
		${GME_INCLUDE_DIRS}
		${OPENMPT_INCLUDE_DIRS}
		${PNG_INCLUDE_DIRS}
		${ZLIB_INCLUDE_DIRS}
		${OPENGL_INCLUDE_DIRS}
	)

	if(${SRB2_HAVE_MIXER})
		target_compile_definitions(SRB2SDL2 PRIVATE -DHAVE_MIXER -DSOUND=SOUND_MIXER)
	endif()

	target_compile_definitions(SRB2SDL2 PRIVATE
		-DHAVE_SDL
	)

	## strip debug symbols into separate file when using gcc
	if(CMAKE_COMPILER_IS_GNUCC)
		if(${CMAKE_BUILD_TYPE} MATCHES Debug)
			message(STATUS "Will make separate debug symbols in *.debug")
			add_custom_command(TARGET SRB2SDL2 POST_BUILD
				COMMAND ${OBJCOPY} --only-keep-debug $<TARGET_FILE:SRB2SDL2> $<TARGET_FILE:SRB2SDL2>.debug
				COMMAND ${OBJCOPY} --strip-debug $<TARGET_FILE:SRB2SDL2>
				COMMAND ${OBJCOPY} --add-gnu-debuglink=$<TARGET_FILE_NAME:SRB2SDL2>.debug $<TARGET_FILE:SRB2SDL2>
			)
		endif()
	endif()

	#### Installation ####
	if(${CMAKE_SYSTEM} MATCHES Darwin)
		install(TARGETS SRB2SDL2
			BUNDLE DESTINATION .
		)
	else()
		install(TARGETS SRB2SDL2 SRB2SDL2
			RUNTIME DESTINATION .
		)
	endif()

	if(${CMAKE_SYSTEM} MATCHES Windows)
		set(win_extra_dll_list "")
		macro(getwinlib dllname defaultname)
			if(${SRB2_CONFIG_USE_INTERNAL_LIBRARIES})
				if (${CMAKE_GENERATOR} STREQUAL "MinGW Makefiles")
					if(${SRB2_SYSTEM_BITS} EQUAL 64)
						find_library(SRB2_SDL2_DLL_${dllname} "${defaultname}"
							HINTS ${CMAKE_SOURCE_DIR}/libs/dll-binaries/x86_64
							HINTS ${CMAKE_SOURCE_DIR}/libs/SDL2/x86_64-w64-mingw32/bin
							HINTS ${CMAKE_SOURCE_DIR}/libs/SDL2_mixer/x86_64-w64-mingw32/bin
							HINTS ${CMAKE_SOURCE_DIR}/libs/libopenmpt/bin/x86_64/mingw
						)
					else()
						find_library(SRB2_SDL2_DLL_${dllname} "${defaultname}"
							HINTS ${CMAKE_SOURCE_DIR}/libs/dll-binaries/i686
							HINTS ${CMAKE_SOURCE_DIR}/libs/SDL2/i686-w64-mingw32/bin
							HINTS ${CMAKE_SOURCE_DIR}/libs/SDL2_mixer/i686-w64-mingw32/bin
							HINTS ${CMAKE_SOURCE_DIR}/libs/libopenmpt/bin/x86/mingw
						)
					endif()
				else()
					if(${SRB2_SYSTEM_BITS} EQUAL 64)
						find_library(SRB2_SDL2_DLL_${dllname} "${defaultname}"
							HINTS ${CMAKE_SOURCE_DIR}/libs/dll-binaries/x86_64
							HINTS ${CMAKE_SOURCE_DIR}/libs/SDL2/lib/x64
							HINTS ${CMAKE_SOURCE_DIR}/libs/SDL2_mixer/lib/x64
							HINTS ${CMAKE_SOURCE_DIR}/libs/libopenmpt/bin/x86_64/mingw
						)
					else()
						find_library(SRB2_SDL2_DLL_${dllname} "${defaultname}"
							HINTS ${CMAKE_SOURCE_DIR}/libs/dll-binaries/i686
							HINTS ${CMAKE_SOURCE_DIR}/libs/SDL2/lib/x86
							HINTS ${CMAKE_SOURCE_DIR}/libs/SDL2_mixer/lib/x86
							HINTS ${CMAKE_SOURCE_DIR}/libs/libopenmpt/bin/x86/mingw
						)
					endif()
				endif()

				list(APPEND win_extra_dll_list ${SRB2_SDL2_DLL_${dllname}})
			else()
				find_library(SRB2_SDL2_DLL_${dllname} "${defaultname}")
				list(APPEND win_extra_dll_list ${SRB2_SDL2_DLL_${dllname}})
	endif()
		endmacro()
		getwinlib(SDL2 "SDL2.dll")
		if(${SRB2_CONFIG_SDL2_USEMIXER})
			getwinlib(SDL2_mixer "SDL2_mixer.dll")
			getwinlib(libogg_0 "libogg-0.dll")
			getwinlib(libvorbis_0 "libvorbis-0.dll")
			getwinlib(libvorbisfile_3 "libvorbisfile-3.dll")
		endif()
		if(${SRB2_CONFIG_HAVE_GME})
			getwinlib(libgme "libgme.dll")
		endif()
		if(${SRB2_CONFIG_HAVE_OPENMPT})
			getwinlib(libopenmpt "libopenmpt.dll")
		endif()

		install(PROGRAMS
			${win_extra_dll_list}
			DESTINATION .
		)

		# We also want to copy those DLLs to build directories on MSVC.
		# So we'll add a post_build step.
		copy_files_to_build_dir(SRB2SDL2 win_extra_dll_list)
	endif()


	# Mac bundle fixup
	# HACK: THIS IS IMPORTANT! See the escaped \${CMAKE_INSTALL_PREFIX}? This
	# makes it so that var is evaluated LATER during cpack, not right now!
	# This fixes the quirk where the bundled libraries don't land in the final package
	# https://cmake.org/pipermail/cmake/2011-March/043532.html
	#
	# HOWEVER: ${CPACK_PACKAGE_DESCRIPTION_SUMMARY} is NOT escaped, because that var
	# is only available to us at this step. Read the link: ${CMAKE_INSTALL_PREFIX} at
	# this current step points to the CMAKE build folder, NOT the folder that CPACK uses.
	# Therefore, it makes sense to escape that var, but not the other.
	if(${CMAKE_SYSTEM} MATCHES Darwin)
		install(CODE "
			include(BundleUtilities)
			fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/${CPACK_PACKAGE_DESCRIPTION_SUMMARY}.app\"
				\"\"
				/Library/Frameworks
			)"
		)
	endif()

	set(SRB2_SDL2_AVAILABLE YES PARENT_SCOPE)
else()
	message(WARNING "SDL2 was not found, so the SDL2 target will not be available.")
	set(SRB2_SDL2_AVAILABLE NO PARENT_SCOPE)
endif()
