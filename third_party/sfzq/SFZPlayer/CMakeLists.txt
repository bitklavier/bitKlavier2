cmake_minimum_required(VERSION 3.10)
project(sfzero LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/test")
# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option for building tests
option(SFZERO_ENABLE_TESTING "Enable testing for SFZero library" OFF)

# Collect source and header files automatically
file(GLOB SFZERO_SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
)
file(GLOB SFZERO_HEADERS
        "${CMAKE_CURRENT_LIST_DIR}/*.h"
        "${CMAKE_CURRENT_LIST_DIR}/vendor/**/*.h"
)

# Create a static library target
add_library(sfzero STATIC
        ${SFZERO_SOURCES}
        ${SFZERO_HEADERS}
)

# Make headers visible to dependents
target_include_directories(sfzero
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/vendor/surge

)

# Optionally enable warnings
#if (MSVC)
#    target_compile_options(sfzero PRIVATE /W4)
#else()
#    target_compile_options(sfzero PRIVATE -Wall -Wextra -Wpedantic)
#endif()

# Optional: enable AddressSanitizer for debug
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")

# Optional: testing setup
#if (SFZERO_ENABLE_TESTING)
#    enable_testing()
#    add_subdirectory(tests)
#endif()