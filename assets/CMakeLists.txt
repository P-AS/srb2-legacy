## Assets Target Configuration ##

# For prepending the current source path, later
FUNCTION(PREPEND var prefix)
   SET(listVar "")
   FOREACH(f ${ARGN})
      LIST(APPEND listVar "${prefix}/${f}")
   ENDFOREACH(f)
   SET(${var} "${listVar}" PARENT_SCOPE)
ENDFUNCTION(PREPEND)

set(SRB2_ASSET_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/installer"
	CACHE STRING "Path to directory that contains all asset files for the installer.")

if (${SRB2_CONFIG_EXTERNAL_ASSETS})
	target_compile_definitions(SRB2SDL2 PRIVATE -DCMAKEEXTERNALASSETS)

	set(SRB2_ASSET_HASHED
"srb2.srb;\
player.dta;\
rings.dta;\
zones.dta;\
patch.dta"
		CACHE STRING "Asset filenames to apply MD5 checks. No spaces between entries!"
	)
endif()

set(SRB2_ASSET_DOCS
"README.txt;\
LICENSE.txt;\
LICENSE-3RD-PARTY.txt"
	CACHE STRING "Documentation filenames. In macOS, these are packaged separately from other assets. No spaces between entries!"
)

PREPEND(SRB2_ASSET_DOCS ${SRB2_ASSET_DIRECTORY} ${SRB2_ASSET_DOCS})

if (${SRB2_CONFIG_EXTERNAL_ASSETS})
	foreach(SRB2_ASSET ${SRB2_ASSET_HASHED})
		file(MD5 ${SRB2_ASSET_DIRECTORY}/${SRB2_ASSET} "SRB2_ASSET_${SRB2_ASSET}_HASH")
		set(SRB2_ASSET_${SRB2_ASSET}_HASH ${SRB2_ASSET_${SRB2_ASSET}_HASH} PARENT_SCOPE)
	endforeach()
endif()

# Installation

if(${CMAKE_SYSTEM} MATCHES Darwin)
	get_target_property(outname SRB2SDL2 OUTPUT_NAME)

	if (${SRB2_CONFIG_EXTERNAL_ASSETS})
		install(DIRECTORY "${SRB2_ASSET_DIRECTORY}/"
			DESTINATION "${outname}.app/Contents/Resources"
		)
	endif()

	install(FILES ${SRB2_ASSET_DOCS}
		DESTINATION .
		OPTIONAL
	)
else()
	if (${SRB2_CONFIG_EXTERNAL_ASSETS})
		install(DIRECTORY "${SRB2_ASSET_DIRECTORY}/"
			DESTINATION .
		)
	else()
		# Docs are assumed to be located in SRB2_ASSET_DIRECTORY, so only install if external assets aren't included
		install(FILES ${SRB2_ASSET_DOCS}
			DESTINATION .
			OPTIONAL
		)
	endif()
endif()
