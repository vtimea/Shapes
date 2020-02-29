#include <jni.h>
#include <android/log.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include "glm/glm/glm.hpp"

#define  LOG_TAG    "libgl2jni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

using namespace glm;
std::vector<float> projMatrix;

auto gVertexShader =
        "uniform mat4 projMatrix;\n"
        "attribute vec4 vPosition;\n"
        "void main() {\n"
        "  gl_Position = projMatrix * vPosition;\n"
        "}\n";

auto gFragmentShader =
        "precision mediump float;\n"
        "uniform vec3 mColor;\n"
        "void main() {\n"
        "gl_FragColor = vec4(mColor, 1.0);"
        "}\n";

GLuint loadShader(GLenum shaderType, const char *pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char *buf = (char *) malloc(static_cast<size_t>(infoLen));
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    LOGE("Could not compile shader %d:\n%s\n",
                         shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint createProgram(const char *pVertexSource, const char *pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        glAttachShader(program, pixelShader);
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char *buf = (char *) malloc(static_cast<size_t>(bufLength));
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    LOGE("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

GLuint gProgram;
GLuint gvPositionHandle;
const float SCALE = 100;

bool setupGraphics(int w, int h) {
    LOGI("setupGraphics(%d, %d)", w, h);
    gProgram = createProgram(gVertexShader, gFragmentShader);
    if (!gProgram) {
        LOGE("Could not create program.");
        return false;
    }
    gvPositionHandle = static_cast<GLuint>(glGetAttribLocation(gProgram, "vPosition"));
    LOGI("glGetAttribLocation(\"vPosition\") = %d\n",
         gvPositionHandle);

    glViewport(0, 0, w, h);

    glUseProgram(gProgram);
    float ratio = h / (float) w;
    projMatrix = {ratio / SCALE, 0, 0, 0,
                  0, 1 / SCALE, 0, 0,
                  0, 0, 1, 0,
                  0, 0, 0, 1};

    GLint loc = glGetUniformLocation(gProgram, "projMatrix");
    if (loc != -1) {
        glUniformMatrix4fv(loc, 1, false, &projMatrix[0]);
    }
    return true;
}

void setColor(glm::vec3 color) {
    GLint loc = glGetUniformLocation(gProgram, "mColor");
    if (loc != -1) {
        glUniform3fv(loc, 1, &color[0]);
    }
}

class Polygon {
    std::vector<vec2> mPoints;
    vec3 mColor;

public:
    Polygon(std::vector<glm::vec2> points, vec3 color) : mPoints{points}, mColor(color) {}

    void draw() const {
        setColor(mColor);
        glVertexAttribPointer(gvPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, &mPoints[0]);
        glEnableVertexAttribArray(gvPositionHandle);
        glDrawArrays(GL_TRIANGLES, 0, mPoints.size());
    }
};

const Polygon triangle = Polygon({{-20.f, -20.f},
                                  {0.f,   17.32f},
                                  {20.f,  -20.f}},
                                 {0.f, 1.f, 0.f});

void renderFrame() {
    static float black = 0.f;
    glClearColor(black, black, black, 1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glUseProgram(gProgram);
    triangle.draw();
}

extern "C" {
JNIEXPORT void JNICALL
Java_com_android_gl2jni_GL2JNILib_init(JNIEnv *env, jclass obj, jint width, jint height);
JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_step(JNIEnv *env, jclass obj);
};

extern "C" JNIEXPORT void JNICALL
Java_com_android_gl2jni_GL2JNILib_init(JNIEnv *env, jclass obj, jint width, jint height) {
    setupGraphics(width, height);
}

extern "C" JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_step(JNIEnv *env, jclass obj) {
    renderFrame();
}
