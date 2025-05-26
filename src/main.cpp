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

int width = 800;
int height = 600;

// -------------------- Camera 全域變數 --------------------
glm::vec3 cameraPos   = glm::vec3(420.0f, -100.0f, -180.0f);
glm::vec3 cameraFront = glm::vec3(-0.40f, 0.60f, 0.60f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw   = -90.0f;      // 初始指向 -Z（使用 -90 度）
float pitch = 0.0f;
float lastX = width / 2.0f;
float lastY = height / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
// --------------------------------------------------------

// 宣告一個 future 用來接收背景線程計算的結果（回傳兩個 surface）
#define MOVE_SPEED 10.f
#define ROTATE_SPEED 2.0f

#define MODEL_LEN 256.0f
#define MODEL_HEI 256.0f
#define MODEL_WID 256.0f




void reshape(GLFWwindow *window, int w, int h){
    width = w;
    height = h;
    glViewport(0, 0, width, height);
}

std::vector<unsigned char> data;

GLuint VAO1, VAO2;
GLsizei vertCount1, vertCount2;


void init_data(){
    std::ifstream file("Vector/1.vec");
    if(!file.is_open()){
        std::cerr << "Failed to open data file." << std::endl;
        return;
    }
    int w, h;
    file >> w >> h;
    std::cout << "Data file opened successfully. Width: " << w << ", Height: " << h << std::endl;
    std::vector<glm::vec2> vec;
    for(int i = 0; i < w * h; ++i){
        glm::vec2 v;
        file >> v.x >> v.y;
        vec.push_back(v);
    }
    std::cout << "Data loaded successfully. Total vectors: " << vec.size() << std::endl;
    file.close();
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

    
    
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::cout << "GLFW version: " << glfwGetVersionString() << std::endl;
    const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    std::cout << "GLSL version: " << glslVersion << std::endl;
    std::cout << "Renderer: " << renderer << std::endl;
    std::cout << "OpenGL version supported: " << version << std::endl;

    // ImGui 初始化
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 400");

    // 讀取資料並生成兩個 iso_surface
    init_data();

    int shaderProgram = 0;

    // 模型矩陣（如有需要可再調整位置）
    glm::mat4 model = glm::mat4(1.0f);

    // 光源 & 顏色
    glm::vec3 lightPos(300.0f, 300.0f, 600.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

    glm::vec3 minDrawPos(-200.0f, -200.0f, -200.0f);
    // 渲染主迴圈
    float ISO1 = 200.f, ISO2 = 10.f;
    while(!glfwWindowShouldClose(window)){
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ImGui 畫面
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            ImGui::Begin("Input Window");
            ImGui::End();
        }



        glUseProgram(shaderProgram);

        // 計算 view 與 projection 矩陣
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float) width / height, 0.1f, 2000.0f);

        // 傳送矩陣給 shader
        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
        GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        GLint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
        GLint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
        GLint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
        GLint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
        glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));
        glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

        GLint minDrawPosLoc = glGetUniformLocation(shaderProgram, "minDrawPos");
        glUniform3fv(minDrawPosLoc, 1, glm::value_ptr(minDrawPos));



        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // 清理
    glDeleteVertexArrays(1, &VAO1);
    glDeleteVertexArrays(1, &VAO2);
    // 如果你有另外存 VBO_pos, VBO_norm，要在這裡也一起刪除

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
