# Asset Conversion Tools CMakeLists

cmake_minimum_required(VERSION 3.16)

# PNG to SPR converter
add_executable(png_to_spr
    png_to_spr.cpp
    ../common/image_loader.cpp
    ../common/palette_optimizer.cpp
)

target_include_directories(png_to_spr PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ../common
)

target_link_libraries(png_to_spr PRIVATE
    ${PNG_LIBRARIES}
    ${ZLIB_LIBRARIES}
)

# WAV to Wisp audio converter
add_executable(wav_to_wisp
    wav_to_wisp.cpp
    ../common/audio_encoder.cpp
    ../common/compression.cpp
)

target_include_directories(wav_to_wisp PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ../common
)

# PNG to LUT converter
add_executable(png_to_lut
    png_to_lut.cpp
    ../common/image_loader.cpp
    ../common/color_quantizer.cpp
)

target_include_directories(png_to_lut PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ../common
)

target_link_libraries(png_to_lut PRIVATE
    ${PNG_LIBRARIES}
    ${ZLIB_LIBRARIES}
)

# Install tools to tools directory
install(TARGETS png_to_spr wav_to_wisp png_to_lut
    RUNTIME DESTINATION tools
)
