# CMakeLists.txt for Main Example
idf_component_register(
    SRCS 
        "../wisp_main_demo.cpp"
    INCLUDE_DIRS 
        "../../src"
    REQUIRES 
        "system"
)
