# CMakeLists.txt for Wisp Engine Asset Conversion Tools

# Asset conversion tools
add_subdirectory(tools/asset_converters)

# Custom target to build all conversion tools
add_custom_target(tools
    DEPENDS png_to_spr wav_to_wisp png_to_lut
    COMMENT "Building asset conversion tools"
)

# Custom target to build example assets
add_custom_target(example_assets
    COMMAND ${CMAKE_COMMAND} -E echo "Building example assets..."
    COMMAND powershell -ExecutionPolicy Bypass -File ${CMAKE_SOURCE_DIR}/build_assets.ps1 -All
    DEPENDS tools
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Converting example assets to engine formats"
)

# Add example_assets to default build
add_dependencies(${PROJECT_NAME} example_assets)
