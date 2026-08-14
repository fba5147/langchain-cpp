set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Darwin)
set(VCPKG_OSX_ARCHITECTURES x86_64)

# See triplets/arm64-osx.cmake for why this is needed: AppleClang doesn't
# support OpenMP out of the box, and faiss's vcpkg port requires it.
# (FAISS_ENABLE_METAL, the other fix needed on arm64-osx, defaults OFF
# here since it's Darwin+arm64-only, so it isn't set.)
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
    )
endif()
