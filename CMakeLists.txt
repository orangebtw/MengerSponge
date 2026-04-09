cmake_minimum_required(VERSION 3.12)

project(MengerSponge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(LLGL_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(RELEASE_BUILD OFF)
else()
    set(RELEASE_BUILD ON)
endif()

if (RELEASE_BUILD)
    message("-- Build Type: Release")
    if(MSVC)
        add_compile_options("/O2" "/Ob3" "/Qfast_transcendentals" "${BITCODE}")
        STRING(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        STRING(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
        STRING(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
        STRING(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    else()
        add_compile_options("-g0" "-O3")
    endif()
    set(DISABLE_FORCE_DEBUG_POSTFIX ON)
    set(LLGL_ENABLE_DEBUG_LAYER OFF CACHE BOOL "" FORCE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message("-- Build Type: Debug")
    if(MSVC)
        add_compile_options("/DEBUG" "/Od")
    else()
        add_compile_options("-ggdb" "-O0")
    endif()
    set(DISABLE_FORCE_DEBUG_POSTFIX OFF)
    set(LLGL_ENABLE_EXCEPTIONS ON CACHE BOOL "" FORCE)
endif()

add_subdirectory(./thirdparty/SGE)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${SRC})

target_link_libraries(${PROJECT_NAME} PRIVATE SGE)

file(GLOB_RECURSE ASSETS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/**/*.*)

set(ASSETS_OUTPUT)
foreach(asset ${ASSETS})
    file(RELATIVE_PATH out_file ${CMAKE_CURRENT_SOURCE_DIR}/assets/ ${asset})
    get_filename_component(out_dir ${out_file} DIRECTORY)
    get_filename_component(out_name ${out_file} NAME)

    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/assets/${out_dir})

    add_custom_command(
        OUTPUT
            ${CMAKE_CURRENT_BINARY_DIR}/assets/${out_dir}/${out_name}
        COMMAND
            ${CMAKE_COMMAND} -E copy ${asset} ${CMAKE_CURRENT_BINARY_DIR}/assets/${out_dir}
        DEPENDS
            ${asset}
    )

    list(APPEND ASSETS_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/assets/${out_dir}/${out_name})
endforeach()

add_custom_target(COPY_ASSETS ALL DEPENDS ${ASSETS_OUTPUT})
