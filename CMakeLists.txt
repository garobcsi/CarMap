cmake_minimum_required(VERSION 3.16)
project(GrafikaLabor LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- GLFW ---
if(WIN32)
    set(GLFW_ROOT ${CMAKE_SOURCE_DIR}/Libraries/glfw-3.4.bin.WIN64)
    if(MSVC)
        if(MSVC_VERSION GREATER_EQUAL 1930)
            set(_GLFW_LIB_DIR ${GLFW_ROOT}/lib-vc2022)
        elseif(MSVC_VERSION GREATER_EQUAL 1920)
            set(_GLFW_LIB_DIR ${GLFW_ROOT}/lib-vc2019)
        else()
            set(_GLFW_LIB_DIR ${GLFW_ROOT}/lib-vc2017)
        endif()
        set(_GLFW_LIB ${_GLFW_LIB_DIR}/glfw3.lib)
    else()
        set(_GLFW_LIB ${GLFW_ROOT}/lib-mingw-w64/libglfw3.a)
    endif()
    add_library(glfw STATIC IMPORTED GLOBAL)
    set_target_properties(glfw PROPERTIES
        IMPORTED_LOCATION ${_GLFW_LIB}
        INTERFACE_INCLUDE_DIRECTORIES ${GLFW_ROOT}/include
    )
else()
    find_package(glfw3 3.3 REQUIRED)
endif()

# --- OpenGL ---
find_package(OpenGL REQUIRED)

# --- GLAD (bundled) ---
add_library(glad STATIC
    ${CMAKE_SOURCE_DIR}/Libraries/Glad/src/glad.c
)
target_include_directories(glad PUBLIC
    ${CMAKE_SOURCE_DIR}/Libraries/Glad/include
)

# --- GLM (header-only, bundled) ---
set(GLM_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/Libraries/glm/include)

# --- ImGui (bundled) ---
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/Libraries/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_link_libraries(imgui PRIVATE glfw OpenGL::GL glad)

# --- Source files (framework + lodepng, always compiled) ---
set(FRAMEWORK_SOURCES
    glProgram/sources/framework.cpp
    glProgram/sources/framework.h
    glProgram/sources/glad.c
    glProgram/sources/lodepng.cpp
    glProgram/sources/lodepng.h
)

# --- Shader embedding: generate a submittable Skeleton.cpp from template + .glsl files ---
set(GENERATE_SCRIPT ${CMAKE_SOURCE_DIR}/cmake/generate_skeleton.cmake)
set(GEN_DIR ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GEN_DIR})

set(SKELETON_TEMPLATE ${CMAKE_SOURCE_DIR}/glProgram/sources/GreenTriangle/Skeleton.cpp)
set(VERT_GLSL         ${CMAKE_SOURCE_DIR}/glProgram/sources/GreenTriangle/vert.glsl)
set(FRAG_GLSL         ${CMAKE_SOURCE_DIR}/glProgram/sources/GreenTriangle/frag.glsl)
set(TITLE_TXT         ${CMAKE_SOURCE_DIR}/title.txt)
set(GENERATED_SKELETON ${GEN_DIR}/Skeleton.cpp)

add_custom_command(
    OUTPUT  ${GENERATED_SKELETON}
    COMMAND ${CMAKE_COMMAND}
        -DTEMPLATE_FILE=${SKELETON_TEMPLATE}
        -DVERT_GLSL=${VERT_GLSL}
        -DFRAG_GLSL=${FRAG_GLSL}
        -DTITLE_TXT=${TITLE_TXT}
        -DOUTPUT_FILE=${GENERATED_SKELETON}
        -P ${GENERATE_SCRIPT}
    DEPENDS ${SKELETON_TEMPLATE} ${VERT_GLSL} ${FRAG_GLSL} ${TITLE_TXT} ${GENERATE_SCRIPT}
    COMMENT "Generating submittable Skeleton.cpp (shaders inlined)"
)

# --- The generated Skeleton.cpp is the main application entry ---
set(APP_SOURCES
    ${GENERATED_SKELETON}
)

add_executable(${PROJECT_NAME}
    ${FRAMEWORK_SOURCES}
    ${APP_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/glProgram/sources
    ${CMAKE_SOURCE_DIR}/Libraries/Glad/include
    ${GLM_INCLUDE_DIR}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# _HAS_CXX17 is MSVC-specific; define it so framework.h picks std::filesystem
target_compile_definitions(${PROJECT_NAME} PRIVATE _HAS_CXX17=1)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    OpenGL::GL
    glad
    imgui
    ${CMAKE_DL_LIBS}
)
