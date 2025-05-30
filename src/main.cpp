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
#include "lic.h"

int width = 800, height = 600;

GLuint streamlineVAO = 0, streamlineVBO = 0;
GLuint quadVAO = 0, quadVBO = 0;
size_t streamlineVertCount = 0;
GLuint noiseTex = 0, vectorTex = 0;

glm::mat4 proj;

int    seedCount = 20;
float  stepSize = 0.01f;
int    maxSteps = 100;
int seedCols = 20;
int seedRows = 20;

vector_field vf;

std::vector<GLsizei> lineVertexCounts;
std::vector<float> seedGradients;


void reshape(GLFWwindow *window, int w, int h){
    width = w;  height = h;
    float winAspect = float(width) / float(height);
    float fieldAspect = float(vf.getWidth()) / float(vf.getHeight());

    if(winAspect > fieldAspect){
        int viewH = height;
        int viewW = int(fieldAspect * viewH);
        int xOff = (width - viewW) / 2;
        glViewport(xOff, 0, viewW, viewH);
    }
    else{
        int viewW = width;
        int viewH = int(viewW / fieldAspect);
        int yOff = (height - viewH) / 2;
        glViewport(0, yOff, viewW, viewH);
    }

    proj = glm::ortho(0.0f, float(vf.getWidth()),
        0.0f, float(vf.getHeight()),
        -1.0f, 1.0f);
}


void rebuildStreamlines(const vector_field &vf){
    std::vector<glm::vec2> allVerts;
    allVerts.reserve(seedCols * seedRows * maxSteps);

    lineVertexCounts.clear();
    seedGradients.clear();                     // ← 清空旧数据
    const auto &G = vf.getGradients();         // 取引用

    for(int j = 0; j < seedRows; j++){
        float fy = (j + 0.5f) * vf.getHeight() / float(seedRows);
        for(int i = 0; i < seedCols; i++){
            float fx = (i + 0.5f) * vf.getWidth() / float(seedCols);

            // 1) RK4 积分，生成流线顶点
            auto line = integrateStreamline(vf, { fx, fy }, stepSize, maxSteps);
            lineVertexCounts.push_back((GLsizei) line.size());
            for(auto &p : line)  allVerts.push_back(p);

            // 2) 记录这个 seed 点的梯度幅值
            int gi = glm::clamp(int(fy), 0, vf.getHeight() - 1);
            int gj = glm::clamp(int(fx), 0, vf.getWidth() - 1);
            float mag = glm::length(G[gi][gj]);
            seedGradients.push_back(mag);
        }
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
    seedCols = vf.getWidth();
    seedRows = vf.getHeight();
    std::pair<GLuint, GLuint> vaovbo = initLICQuad(vf);
    quadVAO = vaovbo.first;
    quadVBO = vaovbo.second;
    noiseTex = makeNoiseTex(512);
    vectorTex = makeVectorTex(vf);

    rebuildStreamlines(vf);
    std::cout << "set done" << std::endl;
}


int main(int argc, char **argv){
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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 400");

    init_data();

    int shaderProgram = 0;

    Shader shader("shader/shader.vert", "shader/shader.frag");
    Shader licShader("shader/lic.vert", "shader/lic.frag");

    proj = glm::ortho(0.0f, float(vf.getHeight()),
        0.0f, float(vf.getWidth()),
        -1.0f, 1.0f);

    std::cout << vf.getHeight() << " " << vf.getWidth() << std::endl;
    int imgRotation = 0;
    bool  flipX = false;
    bool  flipY = false;
    bool show_lic = true;
    bool show_sl = true;

    glm::vec2 mm = vf.get_min_max();
    float gmin = mm.x, gmax = mm.y;

    while(!glfwWindowShouldClose(window)){
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        static int prevCols = seedCols;
        static int prevRows = seedRows;
        static float prevStep = stepSize;
        static int prevMax = maxSteps;

        ImGui::Begin("Streamline Parameters");
        ImGui::SliderInt("Seed Columns", &seedCols, 1, 100);
        ImGui::SliderInt("Seed Rows", &seedRows, 1, 100);
        ImGui::SliderInt("Max Steps", &maxSteps, 10, 2000);
        ImGui::SliderFloat("Step Size", &stepSize, 0.01f, 5.0f);

        if(seedCols != prevCols ||
            seedRows != prevRows ||
            stepSize != prevStep ||
            maxSteps != prevMax){
            prevCols = seedCols;
            prevRows = seedRows;
            prevStep = stepSize;
            prevMax = maxSteps;
            rebuildStreamlines(vf);
        }
        ImGui::End();

        ImGui::Begin("Transform");
        if(ImGui::Button("Rotate 90°")){
            imgRotation = (imgRotation - 90 + 360) % 360;
        }
        ImGui::Checkbox("Flip Horizontal", &flipX);
        ImGui::Checkbox("Flip Vertical", &flipY);
        ImGui::Checkbox("Show LIC", &show_lic);
        ImGui::Checkbox("Show Steam Line", &show_sl);
        ImGui::End();


        glm::mat4 model(1.0f);
        float cx = vf.getWidth() * 0.5f;
        float cy = vf.getHeight() * 0.5f;
        model = glm::translate(model, { cx, cy, 0.0f });
        model = glm::rotate(model,
            glm::radians((float) imgRotation),
            glm::vec3(0, 0, 1));
        model = glm::scale(model,
            { flipX ? -1.0f : 1.0f,
                flipY ? -1.0f : 1.0f,
                1.0f });
        model = glm::translate(model, { -cx, -cy, 0.0f });
        glm::mat4 mvp = proj * model;

        // LIC
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);

        if(show_lic){
            licShader.use();
            licShader.setMat4("uMVP", mvp);
            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, noiseTex);
            licShader.setInt("uNoise", 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, vectorTex);
            licShader.setInt("uVectorField", 1);

            licShader.setFloat("uStepSize", 1.0f / float(std::max(vf.getWidth(), vf.getHeight())));
            licShader.setInt("uNumSteps", 20);

            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
        }

        // Steam Line
        glDepthMask(GL_TRUE);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        if(show_sl){
            shader.use();
            shader.setMat4("uMVP", mvp);
            glBindVertexArray(streamlineVAO);
            int offset = 0;
            for(size_t k = 0; k < lineVertexCounts.size(); ++k){
                float t = (seedGradients[k] - gmin) / (gmax - gmin);
                t = glm::clamp(t, 0.0f, 1.0f);

                glm::vec3 col = glm::mix(
                    glm::vec3(0.0f, 0.0f, 1.0f),  // 蓝
                    glm::vec3(1.0f, 0.0f, 0.0f),  // 红
                    t
                );
                shader.setVec3("uColor", col);

                glDrawArrays(GL_LINE_STRIP, offset, lineVertexCounts[k]);
                offset += lineVertexCounts[k];
            }
        }
        glBindVertexArray(0);

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
