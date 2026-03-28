
FetchContent_Declare(
	freetype
	GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype.git
	GIT_TAG master
)
FetchContent_MakeAvailable(freetype)

#https://github.com/bratsche/pango

set(HB_HAVE_FREETYPE ON CACHE BOOL "" FORCE)
set(HB_BUILD_UTILS OFF CACHE BOOL "" FORCE)
set(HB_BUILD_SUBSET OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  harfbuzz
  GIT_REPOSITORY https://github.com/harfbuzz/harfbuzz.git
  GIT_TAG main
)
FetchContent_MakeAvailable(harfbuzz)