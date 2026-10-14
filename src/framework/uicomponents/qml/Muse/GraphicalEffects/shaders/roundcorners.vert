#version 440

// The vertex shader of Qt Quick's ShaderEffect, compiled with the same GLSL versions as roundcorners.frag: with
// Qt's own, the two stages could get different GLSL ES versions on OpenGL ES 3 (Android), which do not link

layout(location = 0) in vec4 qt_Vertex;
layout(location = 1) in vec2 qt_MultiTexCoord0;

layout(location = 0) out vec2 qt_TexCoord0;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float radius;
    vec2 sourceSize;
};

void main() {
    qt_TexCoord0 = qt_MultiTexCoord0;
    gl_Position = qt_Matrix * qt_Vertex;
}
