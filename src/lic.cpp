#include "lic.h"
#include <vector>
#include <random>

GLuint makeNoiseTex(int size = 512){
    std::vector<GLubyte> data(size * size);
    std::mt19937 rng(12345);
    std::uniform_int_distribution<> dist(0, 255);
    for(int i = 0; i < size * size; i++) data[i] = dist(rng);
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, size, size, 0, GL_RED, GL_UNSIGNED_BYTE, data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return tex;
}

GLuint makeVectorTex(const vector_field &vf){
    int W = vf.getWidth(), H = vf.getHeight();
    std::vector<glm::vec2> buf(W * H);
    auto vect = vf.get_vector();
    for(int j = 0; j < H; j++)
        for(int i = 0; i < W; i++)
            buf[j * W + i] = vect[j][i];  // 你的存储是 [j][i]
    GLuint tex; glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, W, H, 0, GL_RG, GL_FLOAT, buf.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return tex;
}

std::pair<GLuint, GLuint> initLICQuad( vector_field &vf){
    // 每顶点： x,y,u,v
    float quadVerts[] = {
      0, 0,               0, 0,
      vf.getWidth(), 0,               1, 0,
      vf.getWidth(), vf.getHeight(),               1, 1,

      0, 0,               0, 0,
      vf.getWidth(), vf.getHeight(),               1, 1,
      0, vf.getHeight(),               0, 1,
    };
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
    // 位置 aPos @location 0
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) 0);
    // UV aUV @location 1
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) (2 * sizeof(float)));
    glBindVertexArray(0);
    return { vao,vbo };
}
  