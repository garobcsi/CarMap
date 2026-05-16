# generate_skeleton.cmake
# Usage: cmake -DTEMPLATE_FILE=Skeleton.cpp.in
#              -DVERT_GLSL=vert.glsl
#              -DFRAG_GLSL=frag.glsl
#              -DOUTPUT_FILE=Skeleton.cpp
#              -P generate_skeleton.cmake
#
# Reads the template and .glsl files, replaces @VERT_SHADER@ and @FRAG_SHADER@
# placeholders with the shader contents, producing a self-contained Skeleton.cpp
# in the exact format needed for submission.

file(READ "${VERT_GLSL}" VERT_SHADER)
file(READ "${FRAG_GLSL}" FRAG_SHADER)
file(READ "${TITLE_TXT}" TITLE_CONTENT)
file(READ "${TEMPLATE_FILE}" TEMPLATE_CONTENT)

string(REPLACE "@VERT_SHADER@" "${VERT_SHADER}" TEMPLATE_CONTENT "${TEMPLATE_CONTENT}")
string(REPLACE "@FRAG_SHADER@" "${FRAG_SHADER}" TEMPLATE_CONTENT "${TEMPLATE_CONTENT}")
string(REGEX REPLACE "//[^/]*TITLE[^/]*//" "${TITLE_CONTENT}" TEMPLATE_CONTENT "${TEMPLATE_CONTENT}")

file(WRITE "${OUTPUT_FILE}" "${TEMPLATE_CONTENT}")
