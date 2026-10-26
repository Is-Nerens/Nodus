#pragma once

#include <GL/glew.h>
#include <stdio.h>

static GLuint Compile_Shader(GLenum type, const char* src) 
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[512];
        glGetShaderInfoLog(shader, 512, NULL, info);
        printf("Shader compile error: %s\n", info);
    }
    return shader;
}

static GLuint Create_Shader_Program(const char* vertex_src, const char* fragment_src) 
{
    GLuint vertexShader = Compile_Shader(GL_VERTEX_SHADER, vertex_src);
    GLuint fragmentShader = Compile_Shader(GL_FRAGMENT_SHADER, fragment_src);
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char info[512];
        glGetProgramInfoLog(program, 512, NULL, info);
        printf("Shader link error: %s\n", info);
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}