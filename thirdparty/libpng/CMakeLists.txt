add_library(libpng STATIC
	png.c
	pngerror.c
	pngget.c
	pngmem.c
	pngpread.c
	pngread.c
	pngrio.c
	pngrtran.c
	pngrutil.c
	pngset.c
	pngtrans.c
	pngwio.c
	pngwrite.c
	pngwtran.c
	pngwutil.c
)
target_link_libraries(libpng "-lz")
add_library(libpng::libpng ALIAS libpng)
target_include_directories(libpng PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
set(PNG_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/libpng-src/projects -lpng64")
