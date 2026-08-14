set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Darwin)
set(VCPKG_OSX_ARCHITECTURES arm64)

# Overlay of vcpkg's built-in arm64-osx triplet (see
# CONTRIBUTING.md/benchmarks/RESULTS.md for how this was diagnosed).
# faiss's vcpkg port fails to build on real arm64-osx (Apple Silicon)
# without these:
#
# 1. AppleClang doesn't support OpenMP out of the box, and CMake's
#    FindOpenMP can't auto-detect it even with libomp installed and
#    OpenMP_ROOT set as an environment passthrough -- it needs the exact
#    compiler flags and library path spelled out as cache variables.
#    Resolved dynamically since the Homebrew prefix differs between Intel
#    (`/usr/local`) and Apple Silicon (`/opt/homebrew`) installs.
# 2. faiss 1.14.3 added FAISS_ENABLE_METAL, a GPU backend for Apple
#    Silicon that defaults ON for Darwin+arm64 regardless of
#    FAISS_ENABLE_GPU (a separate option) and requires Apple's
#    separately-downloaded Metal Toolchain component, which isn't
#    installed on CI runners by default. This project only uses faiss's
#    CPU-only IndexFlatIP, so it's turned off rather than requiring that
#    extra component.
find_program(BREW_EXECUTABLE brew)
if(BREW_EXECUTABLE)
    execute_process(
        COMMAND "${BREW_EXECUTABLE}" --prefix libomp
        OUTPUT_VARIABLE LIBOMP_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()
if(LIBOMP_PREFIX)
    set(VCPKG_CMAKE_CONFIGURE_OPTIONS
        "-DOpenMP_CXX_FLAGS=-Xpreprocessor -fopenmp -I${LIBOMP_PREFIX}/include"
        "-DOpenMP_CXX_LIB_NAMES=omp"
        "-DOpenMP_omp_LIBRARY=${LIBOMP_PREFIX}/lib/libomp.dylib"
        "-DFAISS_ENABLE_METAL=OFF"
    )
endif()
