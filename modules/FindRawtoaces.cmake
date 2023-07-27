# Find Rawtoaces headers and libraries.
#
# This module can take the following variables to define
# custom search locations:
#
#   RAWTOACES_ROOT
#
# This module defines the following variables:
#
#   RAWTOACES_FOUND         True if Rawtoaces was found
#   RAWTOACES_INCLUDE_DIRS  Where to find Rawtoaces header files
#   RAWTOACES_LIBRARIES     List of Rawtoaces libraries to link against

include (FindPackageHandleStandardArgs)

find_path (RAWTOACES_INCLUDE_DIR NAMES rawtoaces/acesrender.h
           HINTS ${RAWTOACES_ROOT}
)

find_library (RAWTOACES_IDT_LIBRARY NAMES rawtoaces_idt rawtoaces_idt.0.1.0
              PATH_SUFFIXES lib64 lib
              HINTS ${RAWTOACES_ROOT}
)

find_library (RAWTOACES_UTIL_LIBRARY NAMES rawtoaces_util rawtoaces_util.0.1.0
              PATH_SUFFIXES lib64 lib
              HINTS ${RAWTOACES_ROOT}
)

find_package_handle_standard_args (Rawtoaces 
    DEFAULT_MSG
    RAWTOACES_INCLUDE_DIR
    RAWTOACES_IDT_LIBRARY
    RAWTOACES_UTIL_LIBRARY
)

if (RAWTOACES_FOUND)
    set (RAWTOACES_INCLUDE_DIRS
        ${RAWTOACES_INCLUDE_DIR}
    )
    set (RAWTOACES_LIBRARIES
        ${RAWTOACES_IDT_LIBRARY}
        ${RAWTOACES_UTIL_LIBRARY}
    )
    message("Rawtoaces package")
    message("Include dirs: ${RAWTOACES_INCLUDE_DIRS}")
    message("Libraries: ${RAWTOACES_LIBRARIES}")

else ()
    set (RAWTOACES_INCLUDE_DIRS)
    set (RAWTOACES_LIBRARIES)
endif ()

mark_as_advanced (
    RAWTOACES_INCLUDE_DIR
    RAWTOACES_IDT_LIBRARY
    RAWTOACES_UTIL_LIBRARY
)
