file(COPY
	package.json
	index.html
	style.css
	js
	elm
	elm.json
	webpack.config.js
	postcss.config.js
	tailwind.config.js
	DESTINATION .
)
set(NODE_ENV "$<$<STREQUAL:${CMAKE_BUILD_TYPE},Release>:production>")

add_custom_target(
	WEB ALL
	COMMAND npm install && OUTPUT_PATH="${CMAKE_BINARY_DIR}/bin"
		NODE_ENV="${NODE_ENV}" npx webpack
		DEPENDS webpack.config.js package.json elm.json
		BYPRODUCTS dist/*
)

target_compile_definitions(SRB2SDL2 PRIVATE -DHAVE_MIXER -DSOUND=SOUND_MIXER)
target_compile_definitions(SRB2SDL2 PRIVATE -DDIRECTFULLSCREEN -DHAVE_SDL -DHAVE_PNG)
target_link_options(SRB2SDL2 PRIVATE
	--use-port=sdl2
	--use-port=sdl2_mixer
	--use-port=zlib
	--use-port=libpng
	-lidbfs.js
	-sEXPORTED_RUNTIME_METHODS=ccall,cwrap
	-sEXPORTED_FUNCTIONS=_main,_main_program,_P_AddWadFile,_Command_ListWADS_f
	-sASYNCIFY
	-sNO_EXIT_RUNTIME
	-sALLOW_MEMORY_GROWTH=1
	-sINITIAL_MEMORY=983040000
	-sSTACK_SIZE=655360000
	--preload-file=${PROJECT_SOURCE_DIR}/assets/installer@/
	--use-preload-plugins
	--use-preload-cache
	#--shell-file ${PROJECT_SOURCE_DIR}/src/sdl/emscripten/srb2legacy.html
)
target_compile_options(SRB2SDL2 PRIVATE --use-port=sdl2 --use-port=sdl2_mixer --use-port=zlib --use-port=libpng)
