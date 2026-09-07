file(COPY
	${PROJECT_SOURCE_DIR}/src/web/index.html
	${PROJECT_SOURCE_DIR}/src/web/favicon.ico
	${PROJECT_SOURCE_DIR}/src/web/timidity.data
	${PROJECT_SOURCE_DIR}/src/web/js/timidity.js
	DESTINATION ${PROJECT_BINARY_DIR}/bin
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
	--pre-js ${PROJECT_SOURCE_DIR}/src/web/js/requestfullscreen.js
	-sNO_DISABLE_EXCEPTION_CATCHING
	-sFORCE_FILESYSTEM=1
	-sDEFAULT_TO_CXX
)
target_compile_options(SRB2SDL2 PRIVATE --use-port=sdl2 --use-port=sdl2_mixer --use-port=zlib --use-port=libpng)
