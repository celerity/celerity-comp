find_path(FLINT_INCLUDE_DIR flint/flint.h
    HINTS "$ENV{HOME}/usr/include")

find_library(FLINT_LIBRARIES 
    NAMES flint
    HINTS "$ENV{HOME}/usr/lib")

set(FLINT_LIBRARIES ${FLINT_LIBRARY})
set(FLINT_INCLUDE_DIRS ${FLINT_INCLUDE_DIR})
set(FLINT_TARGETS flint)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FLINT DEFAULT_MSG 
    FLINT_LIBRARIES
    FLINT_INCLUDE_DIRS)

mark_as_advanced(FLINT_INCLUDE_DIR FLINT_LIBRARY)
