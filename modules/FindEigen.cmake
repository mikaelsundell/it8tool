# Find Eigen headers and libraries.
#
# This module can take the following variables to define
# custom search locations:
#
#   EIGEN_ROOT
#   EIGEN_LOCATION
#
# This module defines the following variables:
#
#   EIGEN_FOUND         True if Eigen was found
#   EIGEN_INCLUDE_DIRS  Where to find Eigen header files
#   EIGEN_LIBRARIES     List of Eigen libraries to link against

include (FindPackageHandleStandardArgs)

find_path (EIGEN_INCLUDE_DIR NAMES eigen3/Eigen/Dense
           HINTS ${EIGENBASE_ROOT}
                 ${EIGENBASE_LOCATION}
                 /usr/local/include
                 /usr/include
)

find_package_handle_standard_args (Eigen 
    DEFAULT_MSG
    EIGEN_INCLUDE_DIR
)

if (EIGEN_FOUND)
    set (EIGEN_INCLUDE_DIRS ${EIGEN_INCLUDE_DIR}/eigen3)
else ()
    set (EIGEN_INCLUDE_DIR)
endif ()

mark_as_advanced (
    EIGEN_INCLUDE_DIR
)
