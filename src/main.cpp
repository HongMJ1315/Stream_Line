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

GLuint streamline_vao = 0, streamline_vbo = 0;
GLuint quad_vao = 0, quad_vbo = 0;

size_t streamline_vert_cnt = 0;
GLuint noise_tex = 0, vect_tex = 0;

glm::mat4 proj;

float  step_size = 0.01f;
int    max_steps = 100;
int seed_cols = 20;
int seed_rows = 20;

vector_field vf;

std::vector<GLsizei> line_vert_cnt;
std::vector<float> seed_grad;


void reshape(GLFWwindow *window, int w, int h){
    width = w;  height = h;
    float win_aspect = float(width) / float(height);
    float field_aspect = float(vf.get_width()) / float(vf.get_height());

    if(win_aspect > field_aspect){
        int viewH = height;
        int viewW = int(field_aspect * viewH);
        int x_off = (width - viewW) / 2;
        glViewport(x_off, 0, viewW, viewH);
    }
    else{
        int view_w = width;
        int view_h = int(view_w / field_aspect);
        int y_off = (height - view_h) / 2;
        glViewport(0, y_off, view_w, view_h);
    }

    proj = glm::ortho(0.0f, float(vf.get_width()),
        0.0f, float(vf.get_height()),
        -1.0f, 1.0f);
}


void rebuild_streamlines(const vector_field &vf){
    std::vector<glm::vec2> all_verts;
    all_verts.reserve(seed_cols * seed_rows * max_steps);

    line_vert_cnt.clear();
    seed_grad.clear();                     // ← 清空旧数据
    const auto &G = vf.get_gradients();         // 取引用

    for(int j = 0; j < seed_rows; j++){
        float fy = (j + 0.5f) * vf.get_height() / float(seed_rows);
        for(int i = 0; i < seed_cols; i++){
            float fx = (i + 0.5f) * vf.get_width() / float(seed_cols);

            auto line = integrate_streamline(vf, { fx, fy }, step_size, max_steps);
            line_vert_cnt.push_back((GLsizei) line.size());
            for(auto &p : line)  all_verts.push_back(p);

            int gi = glm::clamp(int(fy), 0, vf.get_height() - 1);
            int gj = glm::clamp(int(fx), 0, vf.get_width() - 1);
            float mag = glm::length(G[gi][gj]);
            seed_grad.push_back(mag);
        }
    }

    streamline_vert_cnt = all_verts.size();
    if(streamline_vao == 0){
        glGenVertexArrays(1, &streamline_vao);
        glGenBuffers(1, &streamline_vbo);
    }
    glBindVertexArray(streamline_vao);
    glBindBuffer(GL_ARRAY_BUFFER, streamline_vbo);
    glBufferData(GL_ARRAY_BUFFER,
        streamline_vert_cnt * sizeof(glm::vec2),
        all_verts.data(),
        GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
        sizeof(glm::vec2), (void *) 0);
    glBindVertexArray(0);
}

void init_data(){
    vf = vector_field("Vector/9.vec");
    seed_cols = vf.get_width();
    seed_rows = vf.get_height();
    std::pair<GLuint, GLuint> vaovbo = init_lic_quad(vf);
    quad_vao = vaovbo.first;
    quad_vbo = vaovbo.second;
    noise_tex = build_noise_tex(512);
    vect_tex = build_vector_tex(vf);

    rebuild_streamlines(vf);
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

    Shader streamline_shader("shader/shader.vert", "shader/shader.frag");
    Shader lic_shader("shader/lic.vert", "shader/lic.frag");

    proj = glm::ortho(0.0f, float(vf.get_height()),
        0.0f, float(vf.get_width()),
        -1.0f, 1.0f);

    std::cout << vf.get_height() << " " << vf.get_width() << std::endl;

    int img_rotate = 0;
    bool  flip_x = false;
    bool  flip_y = false;
    bool show_lic = true;
    bool show_sl = true;

    glm::vec2 mm = vf.get_min_max();
    float gmin = mm.x, gmax = mm.y;

    while(!glfwWindowShouldClose(window)){
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        static int prev_cols = seed_cols;
        static int prev_rows = seed_rows;
        static float prev_step = step_size;
        static int prev_max = max_steps;

        ImGui::Begin("Streamline Parameters");
        ImGui::SliderInt("Seed Columns", &seed_cols, 1, 100);
        ImGui::SliderInt("Seed Rows", &seed_rows, 1, 100);
        ImGui::SliderInt("Max Steps", &max_steps, 10, 2000);
        ImGui::SliderFloat("Step Size", &step_size, 0.01f, 5.0f);

        if(seed_cols != prev_cols ||
            seed_rows != prev_rows ||
            step_size != prev_step ||
            max_steps != prev_max){
            prev_cols = seed_cols;
            prev_rows = seed_rows;
            prev_step = step_size;
            prev_max = max_steps;
            rebuild_streamlines(vf);
        }
        ImGui::End();

        ImGui::Begin("Transform");
        if(ImGui::Button("Rotate 90°")){
            img_rotate = (img_rotate - 90 + 360) % 360;
        }
        ImGui::Checkbox("Flip Horizontal", &flip_x);
        ImGui::Checkbox("Flip Vertical", &flip_y);
        ImGui::Checkbox("Show LIC", &show_lic);
        ImGui::Checkbox("Show Steam Line", &show_sl);
        ImGui::End();


        glm::mat4 model(1.0f);
        float cx = vf.get_width() * 0.5f;
        float cy = vf.get_height() * 0.5f;
        model = glm::translate(model, { cx, cy, 0.0f });
        model = glm::rotate(model,
            glm::radians((float) img_rotate),
            glm::vec3(0, 0, 1));
        model = glm::scale(model,
            { flip_x ? -1.0f : 1.0f,
                flip_y ? -1.0f : 1.0f,
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
            lic_shader.use();
            lic_shader.set_mat4("uMVP", mvp);
            glBindVertexArray(quad_vao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, noise_tex);
            lic_shader.set_int("uNoise", 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, vect_tex);
            lic_shader.set_int("uVectorField", 1);

            lic_shader.set_float("uStepSize", 1.0f / float(std::max(vf.get_width(), vf.get_height())));
            lic_shader.set_int("uNumSteps", 20);

            glBindVertexArray(quad_vao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
        }

        // Steam Line
        glDepthMask(GL_TRUE);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        if(show_sl){
            streamline_shader.use();
            streamline_shader.set_mat4("uMVP", mvp);
            glBindVertexArray(streamline_vao);
            int offset = 0;
            for(size_t k = 0; k < line_vert_cnt.size(); ++k){
                float t = (seed_grad[k] - gmin) / (gmax - gmin);
                t = glm::clamp(t, 0.0f, 1.0f);

                glm::vec3 col = glm::mix(
                    glm::vec3(0.0f, 0.0f, 1.0f),  // 蓝
                    glm::vec3(1.0f, 0.0f, 0.0f),  // 红
                    t
                );
                streamline_shader.set_vec3("uColor", col);

                glDrawArrays(GL_LINE_STRIP, offset, line_vert_cnt[k]);
                offset += line_vert_cnt[k];
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
