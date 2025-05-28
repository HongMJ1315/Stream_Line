#include <iostream>
#include <algorithm>
#include <vector>
#include <utility>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iomanip>
#include <fstream>

#include "glsl.h"
#include "GLinclude.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <future>
#include "vector_field.h"

int width = 800, height = 600;

GLuint streamlineVAO = 0, streamlineVBO = 0;
size_t streamlineVertCount = 0;

glm::mat4 proj;

int    seedCount = 20;
float  stepSize = 0.5f;
int    maxSteps = 800;

vector_field vf;

std::vector<GLsizei> lineVertexCounts;


void reshape(GLFWwindow *window, int w, int h){
    width = w;  height = h;
    float winAspect = float(width) / float(height);
    float fieldAspect = float(vf.getWidth()) / float(vf.getHeight());

    if(winAspect > fieldAspect){
        // 窗口比 field 更宽：上下留黑
        int viewH = height;
        int viewW = int(fieldAspect * viewH);
        int xOff = (width - viewW) / 2;
        glViewport(xOff, 0, viewW, viewH);
    }
    else{
        // 窗口比 field 更高：左右留黑
        int viewW = width;
        int viewH = int(viewW / fieldAspect);
        int yOff = (height - viewH) / 2;
        glViewport(0, yOff, viewW, viewH);
    }

    // 投影就依然用原本 field 的 w,h，保证在 letter‐box 区域里不拉伸
    proj = glm::ortho(0.0f, float(vf.getWidth()),
        0.0f, float(vf.getHeight()),
        -1.0f, 1.0f);
}


void rebuildStreamlines(const vector_field &vf){
    std::vector<glm::vec2> allVerts;
    allVerts.reserve(seedCount * maxSteps);

    lineVertexCounts.clear();         // 清掉旧记录

    for(int i = 0; i < seedCount; ++i){
        float fx = (i + 0.5f) * vf.getWidth() / float(seedCount);
        float fy = 0.5f * vf.getHeight();
        auto line = integrateStreamline(vf, { fx, fy }, stepSize, maxSteps);

        // 记录这一条有多少点
        lineVertexCounts.push_back((GLsizei) line.size());

        // 推进 VBO 用的数组
        for(auto &p : line)
            allVerts.push_back(p);
    }

    streamlineVertCount = allVerts.size();
    if(streamlineVAO == 0){
        glGenVertexArrays(1, &streamlineVAO);
        glGenBuffers(1, &streamlineVBO);
    }
    glBindVertexArray(streamlineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, streamlineVBO);
    glBufferData(GL_ARRAY_BUFFER,
        streamlineVertCount * sizeof(glm::vec2),
        allVerts.data(),
        GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
        sizeof(glm::vec2), (void *) 0);
    glBindVertexArray(0);
}


void init_data(){
    vf = vector_field("Vector/1.vec");
    rebuildStreamlines(vf);
    std::cout << "set done" << std::endl;
}


int main(int argc, char **argv){
    // 初始化
    glutInit(&argc, argv);
    if(!glfwInit()){
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *window = glfwCreateWindow(width, height, "Hw1", nullptr, nullptr);
    if(!window){
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetWindowSizeCallback(window, reshape);



    if(!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)){
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::cout << "GLFW version: " << glfwGetVersionString() << std::endl;
    const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *version = glGetString(GL_VERSION);
    std::cout << "GLSL version: " << glslVersion << std::endl;
    std::cout << "Renderer: " << renderer << std::endl;
    std::cout << "OpenGL version supported: " << version << std::endl;

    // ImGui 初始化
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 400");

    init_data();

    int shaderProgram = 0;

    Shader shader("shader/shader.vert", "shader/shader.frag");

    vector_field vf("Vector/1.vec");

    proj = glm::ortho(0.0f, float(vf.getWidth()),
        0.0f, float(vf.getHeight()),
        -1.0f, 1.0f);

    std::cout << vf.getHeight() << " " << vf.getWidth() << std::endl;

    while(!glfwWindowShouldClose(window)){
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        shader.setMat4("uMVP", proj);
        int offset = 0;
        glBindVertexArray(streamlineVAO);

        for(auto count : lineVertexCounts){
            // 每条线单独一笔
            glDrawArrays(GL_LINE_STRIP, offset, count);
            offset += count;
        }
        // glDrawArrays(GL_LINE_STRIP, 0, (GLsizei) streamlineVertCount);
        glBindVertexArray(0);
        // 显示到屏幕，并处理事件
        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
