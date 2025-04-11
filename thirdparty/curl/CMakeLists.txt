add_library(curl INTERFACE)
target_include_directories(curl INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(curl INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/libs/arm64-v8a/libcurl.a")
add_library(CURL::curl ALIAS curl)
