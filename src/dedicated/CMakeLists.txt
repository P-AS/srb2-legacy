# Declare dedicated interface sources

target_sourcefile(c)

target_sources(SRB2SDL2 PRIVATE)

if(${CMAKE_SYSTEM} MATCHES Windows)
	target_sources(SRB2SDL2 PRIVATE
		../win32/win_dbg.c
		../win32/Srb2win.rc)
endif()

if(${CMAKE_SYSTEM} MATCHES Darwin)
	set(MACOSX_BUNDLE_ICON_FILE Srb2mac.icns)
	set_source_files_properties(../sdl/macosx/Srb2mac.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
	target_sources(SRB2SDL2 PRIVATE
		../sdl/macosx/mac_resources.c
		../sdl/macosx/mac_resources.h
		../sdl/macosx/Srb2mac.icns
	)
endif()

if(${CMAKE_SYSTEM} MATCHES Windows)
	set_target_properties(SRB2SDL2 PROPERTIES OUTPUT_NAME srb2legacy)
elseif(${CMAKE_SYSTEM} MATCHES Linux)
	set_target_properties(SRB2SDL2 PROPERTIES OUTPUT_NAME lsrb2dlegacy)
else()
	set_target_properties(SRB2SDL2 PROPERTIES OUTPUT_NAME srb2legacy)
endif()

if(${CMAKE_SYSTEM} MATCHES Darwin)
	find_library(CORE_LIB CoreFoundation)
	target_link_libraries(SRB2SDL2 PRIVATE
		${CORE_LIB}
		${PNG_LIBRARIES}
		${ZLIB_LIBRARIES}
	)
	set_target_properties(SRB2SDL2 PROPERTIES OUTPUT_NAME "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}")
else()
	target_link_libraries(SRB2SDL2 PRIVATE
		${PNG_LIBRARIES}
		${ZLIB_LIBRARIES}
	)

	if(${CMAKE_SYSTEM} MATCHES Linux)
		target_link_libraries(SRB2SDL2 PRIVATE
			m
			rt
		)
	endif()
endif()

#target_link_libraries(SRB2SDL2 PRIVATE SRB2Core)

set_target_properties(SRB2SDL2 PROPERTIES VERSION ${SRB2_VERSION})

if(${CMAKE_SYSTEM} MATCHES Windows)
	target_link_libraries(SRB2SDL2 PRIVATE
		ws2_32
	)
	target_compile_options(SRB2SDL2 PRIVATE
		-U_WINDOWS
	)
endif()

target_include_directories(SRB2SDL2 PRIVATE
	${PNG_INCLUDE_DIRS}
	${ZLIB_INCLUDE_DIRS}
)

target_compile_definitions(SRB2SDL2 PRIVATE
	-DDEDICATED
)

#### Installation ####
if(${CMAKE_SYSTEM} MATCHES Darwin)
	install(TARGETS SRB2SDL2
		BUNDLE DESTINATION .
	)
else()
	install(TARGETS SRB2SDL2 SRB2SDL2
		RUNTIME DESTINATION .
	)
	if ((${CMAKE_BUILD_TYPE} MATCHES Debug) OR (${CMAKE_BUILD_TYPE} MATCHES RelWithDebInfo))
		set(SRB2_DEBUG_INSTALL OFF CACHE BOOL "Insert *.debug file into the install directory or package.")
		if (${SRB2_DEBUG_INSTALL})
			install(FILES $<TARGET_FILE:SRB2SDL2>.debug
				DESTINATION .
				OPTIONAL
			)
		endif()
	endif()
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
