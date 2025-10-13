add_custom_command(OUTPUT dist DEPENDS SRB2SDL2)

file(COPY package.json DESTINATION .)
file(COPY index.html DESTINATION .)
file(COPY style.css DESTINATION .)
file(COPY js DESTINATION .)
file(COPY elm DESTINATION .)
file(COPY elm.json DESTINATION .)
file(COPY webpack.config.js DESTINATION .)
file(COPY postcss.config.js DESTINATION .)
file(COPY tailwind.config.js DESTINATION .)
set(NODE_ENV "$<$<STREQUAL:${CMAKE_BUILD_TYPE},Release>:production>")

add_custom_target(
  WEB ALL
  COMMAND npm install && OUTPUT_PATH="${CMAKE_BINARY_DIR}/bin"
          NODE_ENV="${NODE_ENV}" npx webpack
  DEPENDS webpack.config.js package.json elm.json
  BYPRODUCTS dist/*)

add_link_options("SHELL: -lidbfs.js")
add_link_options("SHELL: -s EXPORTED_RUNTIME_METHODS=ccall,cwrap")
add_link_options(
  "SHELL: -s EXPORTED_FUNCTIONS=_main,_main_program,_P_AddWadFile,_Command_ListWADS_f"
)
add_link_options("SHELL: -s ASYNCIFY")
add_link_options("SHELL: -sUSE_SDL=2")
add_link_options("SHELL: -sUSE_ZLIB=1")
add_link_options("SHELL: -sUSE_SDL_MIXER=2")
add_link_options("SHELL: -s ALLOW_MEMORY_GROWTH=1")
add_link_options("SHELL: -s INITIAL_MEMORY=655360000")
add_link_options("SHELL: -s STACK_SIZE=65536000")
add_link_options("--preload-file=${PROJECT_SOURCE_DIR}/assets/installer@/")
add_link_options("SHELL: --use-preload-plugins")
add_link_options("SHELL: -s NO_EXIT_RUNTIME")
#add_link_options("SHELL: --shell-file ${CMAKE_SOURCE_DIR}/sdl/emscripten/srb2legacy.html")

target_compile_definitions(SRB2SDL2 PRIVATE -DHAVE_MIXER -DSOUND=SOUND_MIXER)
target_compile_definitions(
  SRB2SDL2 PRIVATE -DDIRECTFULLSCREEN -DHAVE_SDL -DHAVE_PNG)
target_link_options(
  SRB2SDL2
  PRIVATE
  -sUSE_SDL=2
  -sUSE_SDL_MIXER=2
  -sUSE_ZLIB
  -sUSE_LIBPNG
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
target_compile_options(SRB2SDL2 PRIVATE -sUSE_SDL=2 -sUSE_SDL_MIXER=2
                                        -sUSE_ZLIB -sUSE_LIBPNG)
