cmake_minimum_required(VERSION 3.15...3.26)
project(${SKBUILD_PROJECT_NAME} VERSION ${SKBUILD_PROJECT_VERSION} LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Python COMPONENTS Interpreter Development REQUIRED)
find_package(nanobind CONFIG REQUIRED)

# -----------------------------
# TensorFlow search (required)
# -----------------------------
# Use Python to query TensorFlow build flags via tf.sysconfig
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
    
    # Also obtain compile and link flags once and reuse later
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
    if(NOT (TF_COMPILE_RESULT EQUAL 0 AND TF_LINK_RESULT EQUAL 0))
        message(FATAL_ERROR "Failed to obtain TensorFlow compile/link flags from tf.sysconfig")
    endif()
    # Parse the TensorFlow flags into lists
    string(REPLACE " " ";" TF_COMPILE_FLAGS_LIST "${TF_COMPILE_FLAGS}")
    string(REPLACE " " ";" TF_LINK_FLAGS_LIST "${TF_LINK_FLAGS}")
else()
    message(FATAL_ERROR "TensorFlow not found. A working 'tensorflow' Python package is required to build the custom op.")
endif()

# -------------------------
# RDKit search (required)
# -------------------------
# Set RDKit directory - allow override via RDKIT_ROOT (cache or env)
if(NOT DEFINED RDKIT_ROOT AND DEFINED ENV{RDKIT_ROOT})
    set(RDKIT_ROOT $ENV{RDKIT_ROOT})
endif()
if(NOT DEFINED RDKIT_ROOT)
    set(RDKIT_ROOT "${CMAKE_SOURCE_DIR}/build/rdkit")
endif()

# Configure RDKit
set(RDKit_DIR "${RDKIT_ROOT}/lib/cmake/rdkit")
find_package(RDKit CONFIG REQUIRED)
message(STATUS "RDKit located at: ${RDKit_DIR}")

# Find other required dependencies (let CMake find them automatically)
find_package(Boost REQUIRED)
find_package(Eigen3 REQUIRED)

# Create the nanobind module (will be installed into rdktools package)
nanobind_add_module(_rdktools_core
    src/cpp/pybind_module.cpp
    src/cpp/molecular_ops.cpp
)

# Include directories (RDKit headers provided via imported targets' usage requirements)
target_include_directories(_rdktools_core PRIVATE src/cpp)

# Link libraries (shared RDKit only; fail fast otherwise)
if(TARGET RDKit::GraphMol)
    target_link_libraries(_rdktools_core PRIVATE 
        RDKit::GraphMol
        RDKit::SmilesParse
        RDKit::Descriptors
        RDKit::FileParsers
        RDKit::Fingerprints
        RDKit::RDGeneral
        RDKit::DataStructs
    )
else()
    message(FATAL_ERROR "RDKit dynamic targets not found. Ensure a shared RDKit with CMake config is available at ${RDKit_DIR}.")
endif()

# Compiler-specific options
target_compile_definitions(_rdktools_core PRIVATE VERSION_INFO=${SKBUILD_PROJECT_VERSION})

# Set properties and RPATH for vendored RDKit
set_target_properties(_rdktools_core PROPERTIES
    CXX_VISIBILITY_PRESET "hidden"
    INTERPROCEDURAL_OPTIMIZATION TRUE
)

# Platform-specific settings
target_compile_options(_rdktools_core PRIVATE -Wall -Wextra)

# Install the extension module into the rdktools package directory so that
# rdktools/__init__.py can import ._rdktools_core
install(TARGETS _rdktools_core DESTINATION rdktools)

# TensorFlow custom op (required)
message(STATUS "Found TensorFlow include dir: ${TensorFlow_INCLUDE_DIRS}")
message(STATUS "Found TensorFlow lib dir: ${TensorFlow_LIBRARY_DIRS}")

# Compile/link flags already computed above; reuse TF_COMPILE_FLAGS_LIST and TF_LINK_FLAGS_LIST

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