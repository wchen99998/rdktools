cmake_minimum_required(VERSION 3.15...3.26)
project(${SKBUILD_PROJECT_NAME} VERSION ${SKBUILD_PROJECT_VERSION} LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Python COMPONENTS Interpreter Development REQUIRED)
find_package(nanobind CONFIG REQUIRED)

# Find TensorFlow (optional - for TensorFlow custom ops)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(TensorFlow QUIET tensorflow)
endif()

# Alternative: Try to find TensorFlow via Python
if(NOT TensorFlow_FOUND)
    execute_process(
        COMMAND ${Python_EXECUTABLE} -c "import tensorflow as tf; print(tf.sysconfig.get_include(), end='')"
        OUTPUT_VARIABLE TensorFlow_INCLUDE_DIR
        ERROR_QUIET
        RESULT_VARIABLE TF_INCLUDE_RESULT
    )
    
    execute_process(
        COMMAND ${Python_EXECUTABLE} -c "import tensorflow as tf; print(tf.sysconfig.get_lib(), end='')"
        OUTPUT_VARIABLE TensorFlow_LIB_DIR
        ERROR_QUIET
        RESULT_VARIABLE TF_LIB_RESULT
    )
    
    if(TF_INCLUDE_RESULT EQUAL 0 AND TF_LIB_RESULT EQUAL 0 AND TensorFlow_INCLUDE_DIR AND TensorFlow_LIB_DIR)
        set(TensorFlow_FOUND TRUE)
        set(TensorFlow_INCLUDE_DIRS ${TensorFlow_INCLUDE_DIR})
        set(TensorFlow_LIBRARY_DIRS ${TensorFlow_LIB_DIR})
        message(STATUS "Found TensorFlow via Python: include=${TensorFlow_INCLUDE_DIR}, lib=${TensorFlow_LIB_DIR}")
        
        # Find the TensorFlow framework library
        find_library(TensorFlow_FRAMEWORK_LIBRARY
            NAMES tensorflow_framework
            PATHS ${TensorFlow_LIB_DIR}
            NO_DEFAULT_PATH
        )
        
        if(TensorFlow_FRAMEWORK_LIBRARY)
            message(STATUS "Found TensorFlow framework library: ${TensorFlow_FRAMEWORK_LIBRARY}")
        else()
            message(STATUS "TensorFlow framework library not found - building without TF framework linking")
        endif()
    else()
        message(STATUS "TensorFlow detection failed: include_result=${TF_INCLUDE_RESULT}, lib_result=${TF_LIB_RESULT}")
    endif()
endif()

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

# Create the nanobind module (will be installed into rdktools package)
nanobind_add_module(_rdktools_core
    src/cpp/pybind_module.cpp
    src/cpp/molecular_ops.cpp
)

# Include directories
target_include_directories(_rdktools_core PRIVATE 
    ${RDKit_INCLUDE_DIRS}
    src/cpp
)

# Link libraries
if(RDKit_FOUND AND TARGET RDKit::GraphMol)
    # Use modern CMake targets if available
    target_link_libraries(_rdktools_core PRIVATE 
        RDKit::GraphMol
        RDKit::SmilesParse
        RDKit::Descriptors
        RDKit::FileParsers
        RDKit::Fingerprints
        RDKit::RDGeneral
    )
else()
    # Use found libraries
    target_link_libraries(_rdktools_core PRIVATE ${RDKit_LIBRARIES})
endif()

# Compiler-specific options
target_compile_definitions(_rdktools_core PRIVATE VERSION_INFO=${SKBUILD_PROJECT_VERSION})

# Set properties
set_target_properties(_rdktools_core PROPERTIES
    CXX_VISIBILITY_PRESET "hidden"
    INTERPROCEDURAL_OPTIMIZATION TRUE
)

# Platform-specific settings
if(MSVC)
    target_compile_options(_rdktools_core PRIVATE /W3)
else()
    target_compile_options(_rdktools_core PRIVATE -Wall -Wextra)
endif()

# Install the extension module into the rdktools package directory so that
# rdktools/__init__.py can import ._rdktools_core
install(TARGETS _rdktools_core DESTINATION rdktools)

# TensorFlow custom op (optional)
if(TensorFlow_FOUND)
    message(STATUS "Found TensorFlow include dir: ${TensorFlow_INCLUDE_DIRS}")
    message(STATUS "Found TensorFlow lib dir: ${TensorFlow_LIBRARY_DIRS}")
    
    # Get TensorFlow compile and link flags using Python
    execute_process(
        COMMAND ${Python_EXECUTABLE} -c "import tensorflow as tf; print(' '.join(tf.sysconfig.get_compile_flags()), end='')"
        OUTPUT_VARIABLE TF_COMPILE_FLAGS
        ERROR_QUIET
        RESULT_VARIABLE TF_COMPILE_RESULT
    )
    
    execute_process(
        COMMAND ${Python_EXECUTABLE} -c "import tensorflow as tf; print(' '.join(tf.sysconfig.get_link_flags()), end='')"
        OUTPUT_VARIABLE TF_LINK_FLAGS
        ERROR_QUIET
        RESULT_VARIABLE TF_LINK_RESULT
    )
    
    if(TF_COMPILE_RESULT EQUAL 0 AND TF_LINK_RESULT EQUAL 0)
        message(STATUS "TensorFlow compile flags: ${TF_COMPILE_FLAGS}")
        message(STATUS "TensorFlow link flags: ${TF_LINK_FLAGS}")
        
        # Parse the TensorFlow flags into lists
        string(REPLACE " " ";" TF_COMPILE_FLAGS_LIST "${TF_COMPILE_FLAGS}")
        string(REPLACE " " ";" TF_LINK_FLAGS_LIST "${TF_LINK_FLAGS}")
        
        # Build TensorFlow custom op using system compiler (following TF guide)
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/rdktools_tf_ops.so
            COMMAND g++ -std=c++17 -shared 
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp/tf_string_op.cpp
                    -o ${CMAKE_CURRENT_BINARY_DIR}/rdktools_tf_ops.so
                    -fPIC ${TF_COMPILE_FLAGS_LIST} ${TF_LINK_FLAGS_LIST} -O2
            DEPENDS src/cpp/tf_string_op.cpp src/cpp/tf_string_op.hpp
            COMMENT "Building TensorFlow custom op using system compiler"
            VERBATIM
        )
        
        # Create a custom target to ensure the TensorFlow op is built
        add_custom_target(tf_ops ALL 
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/rdktools_tf_ops.so
        )
        
        # Install TensorFlow ops library
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/rdktools_tf_ops.so 
                DESTINATION rdktools)
        
    else()
        message(WARNING "Failed to get TensorFlow compilation flags - skipping TensorFlow custom ops")
    endif()
    
else()
    message(STATUS "TensorFlow not found - skipping TensorFlow custom ops")
endif()