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
int    maxSteps = 100;
int seedCols = 20;  // 水平种子数量
int seedRows = 20;  // 垂直种子数量

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
    allVerts.reserve(seedCols * seedRows * maxSteps);

    lineVertexCounts.clear();

    for(int j = 0; j < seedRows; ++j){
        float fy = (j + 0.5f) * vf.getHeight() / float(seedRows);
        for(int i = 0; i < seedCols; ++i){
            float fx = (i + 0.5f) * vf.getWidth()  / float(seedCols);

            // RK4 积分
            auto line = integrateStreamline(vf, { fx, fy },
                                            stepSize, maxSteps);
            lineVertexCounts.push_back((GLsizei)line.size());
            for(auto &p : line)
                allVerts.push_back(p);
        }
    }

    // 上传 VBO/VAO
    streamlineVertCount = allVerts.size();
    if(streamlineVAO == 0){
        glGenVertexArrays(1, &streamlineVAO);
        glGenBuffers     (1, &streamlineVBO);
    }
    glBindVertexArray(streamlineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, streamlineVBO);
    glBufferData(GL_ARRAY_BUFFER,
        streamlineVertCount * sizeof(glm::vec2),
        allVerts.data(),
        GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          sizeof(glm::vec2), (void*)0);
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
    glfwSetWindowSizeCallback(window, reshape);
    reshape(window, width, height);
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


    proj = glm::ortho(0.0f, float(vf.getHeight()),
        0.0f, float(vf.getWidth()),
        -1.0f, 1.0f);

    std::cout << vf.getHeight() << " " << vf.getWidth() << std::endl;
    int imgRotation = 0;     // 角度（度）
    bool  flipX       = false;    // 水平翻转
    bool  flipY       = false;    // 垂直翻转


    while(!glfwWindowShouldClose(window)){
        // 1) 新 frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 2) 参数面板
        // 我们把“生成流线”的参数放在一个单独窗口里
        static int prevCols   = seedCols;
        static int prevRows   = seedRows;
        static float prevStep = stepSize;
        static int prevMax    = maxSteps;

        ImGui::Begin("Streamline Parameters");
        // 水平、垂直种子数
        ImGui::SliderInt("Seed Columns", &seedCols, 1, 100);
        ImGui::SliderInt("Seed Rows",    &seedRows, 1, 100);
        // 最大积分步数
        ImGui::SliderInt("Max Steps",    &maxSteps, 10, 2000);
        // RK4 步长
        ImGui::SliderFloat("Step Size",  &stepSize, 0.01f, 5.0f);

        // 如果任一参数变化，就重新 build
        if (seedCols  != prevCols   ||
            seedRows  != prevRows   ||
            stepSize  != prevStep   ||
            maxSteps  != prevMax)
        {
            prevCols = seedCols;
            prevRows = seedRows;
            prevStep = stepSize;
            prevMax  = maxSteps;
            rebuildStreamlines(vf);
        }
        ImGui::End();

        // 3) Transform 面板（如果你还需要旋转/翻转的话）
        ImGui::Begin("Transform");
        if(ImGui::Button("Rotate 90°")) {
            imgRotation = (imgRotation - 90 + 360) % 360;
        }
        ImGui::Checkbox("Flip Horizontal", &flipX);
        ImGui::Checkbox("Flip Vertical",   &flipY);
        ImGui::End();

        // 4) 构造 Model 和 MVP（同你原本那段）
        glm::mat4 model(1.0f);
        float cx = vf.getWidth() * 0.5f;
        float cy = vf.getHeight()* 0.5f;
        model = glm::translate(model, {cx, cy, 0.0f});
        model = glm::rotate(model,
                            glm::radians((float)imgRotation),
                            glm::vec3(0,0,1));
        model = glm::scale(model,
                        { flipX? -1.0f:1.0f,
                            flipY? -1.0f:1.0f,
                            1.0f });
        model = glm::translate(model, {-cx, -cy, 0.0f});
        glm::mat4 mvp = proj * model;

        // 5) 渲染流线
        glClearColor(0.1f,0.1f,0.1f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.use();
        shader.setMat4("uMVP", mvp);
        glBindVertexArray(streamlineVAO);
        int offset = 0;
        for(auto count : lineVertexCounts){
            glDrawArrays(GL_LINE_STRIP, offset, count);
            offset += count;
        }
        glBindVertexArray(0);

        // 6) 渲染 ImGui 并 swap
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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
