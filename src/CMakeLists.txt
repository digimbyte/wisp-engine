# Component CMakeLists.txt for wisp-engine

# Main application files
set(MAIN_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bootloader.cpp
)

# Core engine files
file(GLOB_RECURSE CORE_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/system/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/*.cpp
)

# Combine all sources
set(app_sources ${MAIN_SOURCES} ${CORE_SOURCES})

# Remove test files and examples
list(FILTER app_sources EXCLUDE REGEX ".*test.*\\.cpp$")
list(FILTER app_sources EXCLUDE REGEX ".*example.*\\.cpp$")

# Remove certain memory-intensive files for ESP32-C6
if(CONFIG_IDF_TARGET_ESP32C6)
    message(STATUS "Configuring minimal engine for ESP32-C6")
    
    # Remove heavyweight engine components
    list(FILTER app_sources EXCLUDE REGEX ".*lut_system\\.cpp$")
    list(FILTER app_sources EXCLUDE REGEX ".*sprite_layers\\.cpp$") 
    list(FILTER app_sources EXCLUDE REGEX ".*database_system\\.cpp$")
    list(FILTER app_sources EXCLUDE REGEX ".*save_system\\.cpp$")
    list(FILTER app_sources EXCLUDE REGEX ".*database_system_safe\\.cpp$")
    list(FILTER app_sources EXCLUDE REGEX ".*generic_database\\.cpp$")
    list(FILTER app_sources EXCLUDE REGEX ".*curated_api\\.cpp$")
    list(FILTER app_sources EXCLUDE REGEX ".*loop\\.h$")
    list(FILTER app_sources EXCLUDE REGEX ".*optimized_engine\\.h$")
    
    # Add minimal engine instead
    list(APPEND app_sources ${CMAKE_CURRENT_SOURCE_DIR}/engine/minimal/minimal_engine.cpp)
    
    message(STATUS "Minimal engine configuration complete")
else()
    message(STATUS "Using full engine for ESP32-S3")
    # Remove minimal engine for full builds
    list(FILTER app_sources EXCLUDE REGEX ".*minimal_engine\\.cpp$")
endif()

# Debug: Print what files will be compiled
message(STATUS "Source files to be compiled:")
foreach(src_file ${app_sources})
    get_filename_component(filename ${src_file} NAME)
    message(STATUS "  ${filename}")
endforeach()

idf_component_register(SRCS ${app_sources})
