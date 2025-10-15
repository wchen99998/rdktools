cmake_minimum_required(VERSION 3.18...3.28)
project(${SKBUILD_PROJECT_NAME} VERSION ${SKBUILD_PROJECT_VERSION} LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Python 3.12 COMPONENTS Interpreter Development.Module REQUIRED)
find_package(nanobind CONFIG REQUIRED)

set(RDKTOOLS_RDKIT_GIT_TAG "Release_2025_09_1" CACHE STRING "Git tag or commit to use when vendoring RDKit")

# -------------------------
# Boost vendoring (required)
# -------------------------
if(WIN32)
    message(FATAL_ERROR "Vendored Boost build is not currently supported on Windows. Please provide a prebuilt Boost 1.88 installation and set BOOST_ROOT accordingly.")
endif()

include(FetchContent)
include(ProcessorCount)

set(RDKTOOLS_BOOST_VERSION "1.88.0" CACHE STRING "Boost release to vendor for RDKit build")
set(_boost_version_filename "${RDKTOOLS_BOOST_VERSION}")
string(REPLACE "." "_" _boost_version_filename "${_boost_version_filename}")
set(_boost_install_dir "${CMAKE_BINARY_DIR}/vendor/boost")
set(_boost_config_file "${_boost_install_dir}/lib/cmake/Boost-${RDKTOOLS_BOOST_VERSION}/BoostConfig.cmake")
set(_boost_build_dir "${CMAKE_BINARY_DIR}/boost-build")

set(_boost_archive_url "https://archives.boost.io/release/${RDKTOOLS_BOOST_VERSION}/source/boost_${_boost_version_filename}.tar.gz")

FetchContent_Declare(boost_external
    URL "${_boost_archive_url}"
    URL_HASH SHA256=3621533e820dcab1e8012afd583c0c73cf0f77694952b81352bf38c1488f9cb4
)

FetchContent_GetProperties(boost_external)

if(NOT EXISTS "${_boost_config_file}")
    if(NOT boost_external_POPULATED)
        message(STATUS "Fetching Boost ${RDKTOOLS_BOOST_VERSION} source archive")
        FetchContent_Populate(boost_external)
    endif()

    set(_boost_source_dir "${boost_external_SOURCE_DIR}")
    if(NOT EXISTS "${_boost_source_dir}/b2")
        message(STATUS "Bootstrapping Boost ${RDKTOOLS_BOOST_VERSION}")
        execute_process(
            COMMAND ./bootstrap.sh --prefix=${_boost_install_dir}
            WORKING_DIRECTORY "${_boost_source_dir}"
            RESULT_VARIABLE _boost_bootstrap_result
        )
        if(NOT _boost_bootstrap_result EQUAL 0)
            message(FATAL_ERROR "Boost bootstrap.sh failed with exit code ${_boost_bootstrap_result}")
        endif()
    endif()

    # Determine parallelism for b2
    set(_boost_parallelism 2)
    if(DEFINED ENV{RDKTOOLS_BOOST_JOBS} AND NOT "$ENV{RDKTOOLS_BOOST_JOBS}" STREQUAL "")
        set(_boost_parallelism "$ENV{RDKTOOLS_BOOST_JOBS}")
    else()
        ProcessorCount(_boost_detected_parallelism)
        if(_boost_detected_parallelism GREATER 0)
            set(_boost_parallelism "${_boost_detected_parallelism}")
        endif()
    endif()

    set(_boost_b2_command
        ./b2
        install
        --prefix=${_boost_install_dir}
        --build-dir=${_boost_build_dir}
        --layout=system
        threading=multi
        link=static
        variant=release
        cxxstd=17
        cflags=-fPIC
        cxxflags=-fPIC
        -j
        ${_boost_parallelism}
    )

    set(RDKTOOLS_BOOST_COMPONENTS
        atomic
        chrono
        date_time
        filesystem
        graph
        iostreams
        program_options
        random
        regex
        serialization
        system
        thread
    )
    foreach(_component IN LISTS RDKTOOLS_BOOST_COMPONENTS)
        list(APPEND _boost_b2_command "--with-${_component}")
    endforeach()

    message(STATUS "Building Boost ${RDKTOOLS_BOOST_VERSION} with b2 (parallel jobs: ${_boost_parallelism})")
    execute_process(
        COMMAND ${_boost_b2_command}
        WORKING_DIRECTORY "${_boost_source_dir}"
        RESULT_VARIABLE _boost_b2_result
    )
    if(NOT _boost_b2_result EQUAL 0)
        message(FATAL_ERROR "Boost b2 install failed with exit code ${_boost_b2_result}")
    endif()
endif()

set(BOOST_ROOT "${_boost_install_dir}" CACHE PATH "Boost install prefix used for vendored RDKit build" FORCE)
set(BOOST_INCLUDEDIR "${BOOST_ROOT}/include" CACHE PATH "Boost include path" FORCE)
set(BOOST_LIBRARYDIR "${BOOST_ROOT}/lib" CACHE PATH "Boost library path" FORCE)
set(Boost_NO_SYSTEM_PATHS ON CACHE BOOL "Disallow searching system paths for Boost" FORCE)
set(Boost_USE_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(Boost_USE_STATIC_RUNTIME OFF CACHE BOOL "" FORCE)
set(Boost_USE_MULTITHREADED ON CACHE BOOL "" FORCE)

list(FIND CMAKE_PREFIX_PATH "${BOOST_ROOT}" _boost_prefix_index)
if(_boost_prefix_index EQUAL -1)
    list(APPEND CMAKE_PREFIX_PATH "${BOOST_ROOT}")
endif()

set(ENV{BOOST_ROOT} "${BOOST_ROOT}")
set(ENV{BOOST_INCLUDEDIR} "${BOOST_INCLUDEDIR}")
set(ENV{BOOST_LIBRARYDIR} "${BOOST_LIBRARYDIR}")

message(STATUS "Using vendored Boost ${RDKTOOLS_BOOST_VERSION} from ${BOOST_ROOT}")

# Install Boost runtime libraries alongside the extension for wheel packaging
if(NOT Boost_USE_STATIC_LIBS)
    install(
        DIRECTORY "${BOOST_LIBRARYDIR}/"
        DESTINATION rdktools/vendor/boost/lib
        FILES_MATCHING
            PATTERN "*.so"
            PATTERN "*.so.*"
            PATTERN "*.dylib"
            PATTERN "*.dll"
    )
endif()

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

    string(STRIP "${TF_COMPILE_FLAGS}" TF_COMPILE_FLAGS)
    string(STRIP "${TF_LINK_FLAGS}" TF_LINK_FLAGS)

    if(TF_COMPILE_FLAGS)
        separate_arguments(TF_COMPILE_FLAGS_LIST NATIVE_COMMAND "${TF_COMPILE_FLAGS}")
    else()
        set(TF_COMPILE_FLAGS_LIST "")
    endif()

    if(TF_LINK_FLAGS)
        separate_arguments(TF_LINK_FLAGS_LIST NATIVE_COMMAND "${TF_LINK_FLAGS}")
    else()
        set(TF_LINK_FLAGS_LIST "")
    endif()
else()
    message(FATAL_ERROR "TensorFlow not found. A working 'tensorflow' Python package is required to build the custom op.")
endif()

# -------------------------
# RDKit vendoring (required)
# -------------------------
message(STATUS "Fetching RDKit (${RDKTOOLS_RDKIT_GIT_TAG}) as part of the build")

FetchContent_Declare(rdkit_external
    GIT_REPOSITORY https://github.com/rdkit/rdkit.git
    GIT_TAG        ${RDKTOOLS_RDKIT_GIT_TAG}
    GIT_SHALLOW    TRUE
)

FetchContent_GetProperties(rdkit_external)
if(NOT rdkit_external_POPULATED)
    FetchContent_Populate(rdkit_external)

    # Patch RDKit top-level CMake to respect externally-provided static preference
    set(_rdkit_cmake_top "${rdkit_external_SOURCE_DIR}/CMakeLists.txt")
    if(EXISTS "${_rdkit_cmake_top}")
        file(READ "${_rdkit_cmake_top}" _rdkit_cmake_contents)
        string(REGEX REPLACE
            "if \\(\\(MSVC AND \\(NOT RDK_INSTALL_DLLS_MSVC\\)\\) OR \\(WIN32 AND RDK_INSTALL_STATIC_LIBS\\) OR RDK_BUILD_MINIMAL_LIB\\)[\r\n]+  set\\(RDK_BUILD_STATIC_LIBS_ONLY ON\\)[\r\n]+else\\(\\)[\r\n]+  set\\(RDK_BUILD_STATIC_LIBS_ONLY OFF\\)[\r\n]+endif\\(\\)[\r\n]+if\\(NOT RDK_BUILD_STATIC_LIBS_ONLY\\)[\r\n]+  add_definitions\\(-DRDKIT_DYN_LINK\\)[\r\n]+endif\\(\\)"
            "if(NOT DEFINED RDK_BUILD_STATIC_LIBS_ONLY)\n  if ((MSVC AND (NOT RDK_INSTALL_DLLS_MSVC)) OR (WIN32 AND RDK_INSTALL_STATIC_LIBS) OR RDK_BUILD_MINIMAL_LIB)\n    set(RDK_BUILD_STATIC_LIBS_ONLY ON)\n  else()\n    set(RDK_BUILD_STATIC_LIBS_ONLY OFF)\n  endif()\nendif()\nif(NOT RDK_BUILD_STATIC_LIBS_ONLY)\n  add_definitions(-DRDKIT_DYN_LINK)\nendif()"
            _rdkit_cmake_contents "${_rdkit_cmake_contents}")
        file(WRITE "${_rdkit_cmake_top}" "${_rdkit_cmake_contents}")
    endif()
endif()

# Trim unneeded RDKit components to keep wheel size down
set(RDK_BUILD_PYTHON_WRAPPERS OFF CACHE BOOL "" FORCE)
set(RDK_BUILD_SWIG_WRAPPERS OFF CACHE BOOL "" FORCE)
set(RDK_BUILD_CPP_TESTS OFF CACHE BOOL "" FORCE)
set(RDK_BUILD_INCHI_SUPPORT OFF CACHE BOOL "" FORCE)
set(RDK_BUILD_AVALON_SUPPORT OFF CACHE BOOL "" FORCE)
set(RDK_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(RDK_BUILD_PGSQL OFF CACHE BOOL "" FORCE)
set(RDK_INSTALL_INTREE OFF CACHE BOOL "" FORCE)
set(RDK_INSTALL_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(RDK_INSTALL_DEV_COMPONENT ON CACHE BOOL "" FORCE)
set(RDK_BUILD_FREETYPE_SUPPORT OFF CACHE BOOL "" FORCE)
set(RDK_INSTALL_COMIC_FONTS OFF CACHE BOOL "" FORCE)

# Build only static RDKit libraries so they can be folded into the extension module
set(RDK_BUILD_STATIC_LIBS_ONLY ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(${rdkit_external_SOURCE_DIR} ${rdkit_external_BINARY_DIR})

# Create the nanobind module (will be installed into rdktools package)
nanobind_add_module(_rdktools_core
    src/cpp/pybind_module.cpp
    src/cpp/molecular_ops.cpp
    src/cpp/ecfp_trace.cpp
)

# Include directories (RDKit headers provided via imported targets' usage requirements)
target_include_directories(_rdktools_core PRIVATE src/cpp)

# Link libraries (shared RDKit only; fail fast otherwise)
set(_rdkit_components
    GraphMol
    SmilesParse
    Descriptors
    FileParsers
    Fingerprints
    RDGeneral
    DataStructs
)
set(_rdkit_targets "")
foreach(component IN LISTS _rdkit_components)
    list(APPEND _rdkit_targets "${component}")
endforeach()

foreach(component_target IN LISTS _rdkit_targets)
    if(NOT TARGET ${component_target})
        message(FATAL_ERROR "Vendored RDKit target ${component_target} missing. Verify FetchContent build.")
    endif()
endforeach()
target_link_libraries(_rdktools_core PRIVATE ${_rdkit_targets})

# Compiler-specific options
target_compile_definitions(_rdktools_core PRIVATE VERSION_INFO=${SKBUILD_PROJECT_VERSION})

# Set properties and RPATH for vendored RDKit
set_target_properties(_rdktools_core PROPERTIES
    CXX_VISIBILITY_PRESET "hidden"
    INTERPROCEDURAL_OPTIMIZATION TRUE
)

if(NOT Boost_USE_STATIC_LIBS)
    set(_rdktools_boost_build_rpath "${BOOST_LIBRARYDIR}")
    if(APPLE)
        set(_rdktools_boost_install_rpath "@loader_path/vendor/boost/lib")
    elseif(UNIX)
        set(_rdktools_boost_install_rpath "$ORIGIN/vendor/boost/lib")
    else()
        set(_rdktools_boost_install_rpath "")
    endif()

    if(_rdktools_boost_install_rpath)
        set_target_properties(_rdktools_core PROPERTIES
            BUILD_RPATH "${_rdktools_boost_build_rpath}"
            INSTALL_RPATH "${_rdktools_boost_install_rpath}"
        )
    endif()
endif()

# Platform-specific settings
target_compile_options(_rdktools_core PRIVATE -Wall -Wextra)

# Install the extension module into the rdktools package directory so that
# rdktools/__init__.py can import ._rdktools_core
install(TARGETS _rdktools_core DESTINATION rdktools)

# TensorFlow custom op (required)
message(STATUS "Found TensorFlow include dir: ${TensorFlow_INCLUDE_DIRS}")
message(STATUS "Found TensorFlow lib dir: ${TensorFlow_LIBRARY_DIRS}")

# Compile/link flags already computed above; reuse TF_COMPILE_FLAGS_LIST and TF_LINK_FLAGS_LIST

# Build TensorFlow custom op (standalone module, no linkage to _rdktools_core)
add_library(rdktools_tf_ops MODULE
    src/cpp/tf_string_op.cpp
    src/cpp/ecfp_trace.cpp
)

target_include_directories(rdktools_tf_ops PRIVATE
    src/cpp
    ${TensorFlow_INCLUDE_DIRS}
)

target_compile_options(rdktools_tf_ops PRIVATE
    -O2
    -fPIC
    ${TF_COMPILE_FLAGS_LIST}
)

# Force the C++ ABI for the TensorFlow ops module to use the C++11 ABI where
# required by some TF builds and prebuilt libraries. This ensures symbols
# like those from libstdc++ match TensorFlow's expectations. Set to 1 to
# enable the new C++11 ABI.
target_compile_definitions(rdktools_tf_ops PRIVATE _GLIBCXX_USE_CXX11_ABI=1)

target_link_options(rdktools_tf_ops PRIVATE
    ${TF_LINK_FLAGS_LIST}
)

target_link_libraries(rdktools_tf_ops PRIVATE
    ${_rdkit_targets}
)

set_target_properties(rdktools_tf_ops PROPERTIES
    PREFIX ""
    OUTPUT_NAME "rdktools_tf_ops"
    POSITION_INDEPENDENT_CODE ON
)

install(TARGETS rdktools_tf_ops
        DESTINATION rdktools)
