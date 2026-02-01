cmake_minimum_required(VERSION 3.10)

project(tiny_ref_checker C)

add_executable(${PROJECT_NAME} main.c)
set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 99)

if(CMAKE_BUILD_TYPE MATCHES "^[Rr]elease")
    set(IS_RELEASE_BUILD ON CACHE BOOL "" FORCE)
else()
    set(IS_RELEASE_BUILD OFF CACHE BOOL "" FORCE)
endif()

# More warnings.
if(MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC /W3 /WX /wd4996) # disable "unsafe function" warning
else()
    target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Wextra -Werror -Wconversion -Wno-unused-but-set-parameter)
endif()

# Enable ASan.
if(NOT IS_RELEASE_BUILD AND NOT MSVC)
    message(STATUS "${PROJECT_NAME}: Address sanitizer is enabled.")
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENGINE_ASAN_ENABLED)
    target_compile_options(${PROJECT_NAME} PRIVATE -fno-omit-frame-pointer)
    target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
    target_link_libraries(${PROJECT_NAME} PRIVATE -fno-omit-frame-pointer)
    target_link_libraries(${PROJECT_NAME} PRIVATE -fsanitize=address)
    if (CMAKE_COMPILER_IS_GNUCC)
	target_compile_options(${PROJECT_NAME} PRIVATE -static-libasan)
    endif()
endif()

