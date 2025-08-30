cmake_minimum_required(VERSION 3.15...3.26)
project(${SKBUILD_PROJECT_NAME} VERSION ${SKBUILD_PROJECT_VERSION} LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 CONFIG REQUIRED)

# Find RDKit
# First try to find RDKit via CMake config
find_package(RDKit CONFIG QUIET)

if(NOT RDKit_FOUND)
    # Fallback: find RDKit via pkg-config or manual paths
    find_path(RDKit_INCLUDE_DIR
        NAMES GraphMol/GraphMol.h
        PATHS
            /usr/include/rdkit
            /usr/local/include/rdkit
            /opt/conda/include/rdkit
            $ENV{CONDA_PREFIX}/include/rdkit
            $ENV{RDBASE}/Code
        DOC "RDKit include directory"
    )

    find_library(RDKit_GraphMol_LIBRARY
        NAMES GraphMol
        PATHS
            /usr/lib
            /usr/local/lib
            /opt/conda/lib
            $ENV{CONDA_PREFIX}/lib
            $ENV{RDBASE}/lib
        DOC "RDKit GraphMol library"
    )

    find_library(RDKit_SmilesParse_LIBRARY
        NAMES SmilesParse
        PATHS
            /usr/lib
            /usr/local/lib
            /opt/conda/lib
            $ENV{CONDA_PREFIX}/lib
            $ENV{RDBASE}/lib
        DOC "RDKit SmilesParse library"
    )

    find_library(RDKit_Descriptors_LIBRARY
        NAMES Descriptors
        PATHS
            /usr/lib
            /usr/local/lib
            /opt/conda/lib
            $ENV{CONDA_PREFIX}/lib
            $ENV{RDBASE}/lib
        DOC "RDKit Descriptors library"
    )

    find_library(RDKit_FileParsers_LIBRARY
        NAMES FileParsers
        PATHS
            /usr/lib
            /usr/local/lib
            /opt/conda/lib
            $ENV{CONDA_PREFIX}/lib
            $ENV{RDBASE}/lib
        DOC "RDKit FileParsers library"
    )

    find_library(RDKit_Fingerprints_LIBRARY
        NAMES Fingerprints
        PATHS
            /usr/lib
            /usr/local/lib
            /opt/conda/lib
            $ENV{CONDA_PREFIX}/lib
            $ENV{RDBASE}/lib
        DOC "RDKit Fingerprints library"
    )

    find_library(RDKit_RDGeneral_LIBRARY
        NAMES RDGeneral
        PATHS
            /usr/lib
            /usr/local/lib
            /opt/conda/lib
            $ENV{CONDA_PREFIX}/lib
            $ENV{RDBASE}/lib
        DOC "RDKit RDGeneral library"
    )

    # Check if all required components were found
    if(RDKit_INCLUDE_DIR AND 
       RDKit_GraphMol_LIBRARY AND 
       RDKit_SmilesParse_LIBRARY AND 
       RDKit_Descriptors_LIBRARY AND
       RDKit_FileParsers_LIBRARY AND
       RDKit_Fingerprints_LIBRARY AND
       RDKit_RDGeneral_LIBRARY)
        set(RDKit_FOUND TRUE)
        set(RDKit_LIBRARIES 
            ${RDKit_GraphMol_LIBRARY}
            ${RDKit_SmilesParse_LIBRARY}
            ${RDKit_Descriptors_LIBRARY}
            ${RDKit_FileParsers_LIBRARY}
            ${RDKit_Fingerprints_LIBRARY}
            ${RDKit_RDGeneral_LIBRARY}
        )
        set(RDKit_INCLUDE_DIRS ${RDKit_INCLUDE_DIR})
    endif()
endif()

if(NOT RDKit_FOUND)
    message(FATAL_ERROR "RDKit not found. Please install RDKit or set RDBASE environment variable.")
endif()

message(STATUS "Found RDKit include dir: ${RDKit_INCLUDE_DIRS}")
message(STATUS "Found RDKit libraries: ${RDKit_LIBRARIES}")

# Create the pybind11 module (will be installed into rdtools package)
pybind11_add_module(_rdtools_core
    src/cpp/pybind_module.cpp
    src/cpp/molecular_ops.cpp
)

# Include directories
target_include_directories(_rdtools_core PRIVATE 
    ${RDKit_INCLUDE_DIRS}
    src/cpp
)

# Link libraries
if(RDKit_FOUND AND TARGET RDKit::GraphMol)
    # Use modern CMake targets if available
    target_link_libraries(_rdtools_core PRIVATE 
        RDKit::GraphMol
        RDKit::SmilesParse
        RDKit::Descriptors
        RDKit::FileParsers
        RDKit::Fingerprints
        RDKit::RDGeneral
    )
else()
    # Use found libraries
    target_link_libraries(_rdtools_core PRIVATE ${RDKit_LIBRARIES})
endif()

# Compiler-specific options
target_compile_definitions(_rdtools_core PRIVATE VERSION_INFO=${SKBUILD_PROJECT_VERSION})

# Set properties
set_target_properties(_rdtools_core PROPERTIES
    CXX_VISIBILITY_PRESET "hidden"
    INTERPROCEDURAL_OPTIMIZATION TRUE
)

# Platform-specific settings
if(MSVC)
    target_compile_options(_rdtools_core PRIVATE /W3)
else()
    target_compile_options(_rdtools_core PRIVATE -Wall -Wextra)
endif()

# Install the extension module into the rdtools package directory so that
# rdtools/__init__.py can import ._rdtools_core
install(TARGETS _rdtools_core DESTINATION rdtools)