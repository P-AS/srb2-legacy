add_library(sdl2 INTERFACE)
target_include_directories(sdl2 INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(sdl2 INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/libs/arm64-v8a/libSDL2.so")
add_library(SDL2::SDL2 ALIAS sdl2)
