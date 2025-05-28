
#include <iostream>
#include <fstream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include "vector_field.h"



vector_field::vector_field(const std::string &filename){
    std::ifstream file(filename);
    if(!file.is_open()){
        std::cerr << "Failed to open data file: " << filename << std::endl;
        return;
    }
    file >> w >> h;
    vectors.resize(w, std::vector<glm::vec2>(h));
    for(int i = 0; i < w; ++i){
        for(int j = 0; j < h; ++j){
            glm::vec2 v;
            file >> v.x >> v.y;
            vectors[i][j] = v;
        }
    }
    file.close();
    compute_gradients(); // Compute gradients after loading data
}

vector_field::vector_field(){};

glm::vec2 vector_field::sample_value(int x, int y) const{
    // Ensure x and y are within bounds
    if(x < 0 || y < 0 || y >= vectors.size() || x >= vectors[y].size()){
        return glm::vec2(0.0f, 0.0f); // Out of bounds, return zero vector
    }
    return vectors[y][x]; // Return the vector at (x, y)
}

glm::vec2 vector_field::compute_point_gradient(int x, int y){
    int dx = 1; // Sample offset in x direction
    int dy = 1; // Sample offset in y direction
    glm::vec2 v1 = sample_value(x + dx, y + dy);
    glm::vec2 v2 = sample_value(x - dx, y - dy);
    glm::vec2 v3 = sample_value(x + dx, y - dy);
    glm::vec2 v4 = sample_value(x - dx, y + dy);

    glm::vec2 gradient;
    gradient.x = (v1.x - v2.x + v3.x - v4.x) / (4 * dx);
    gradient.y = (v1.y - v2.y + v3.y - v4.y) / (4 * dy);
    // Normalize the gradient
    float length = glm::length(gradient);
    if(length > 0.0f){
        gradient /= length; // Normalize to unit vector

    }
    else{
        gradient = glm::vec2(0.0f, 0.0f); // If length is zero, return zero vector
    }
    max_gradient = std::max(max_gradient, glm::length(gradient)); // Update max gradient
    if(min_gradient == 0.0f || glm::length(gradient) < min_gradient){
        min_gradient = glm::length(gradient); // Update min gradient
    }
    return gradient; // Return the computed gradient
}

void vector_field::compute_gradients(){
    for(int y = 0; y < vectors.size(); ++y){
        for(int x = 0; x < vectors[y].size(); ++x){
            glm::vec2 gradient = compute_point_gradient(x, y);
            float magnitude = glm::length(gradient);
            if(magnitude > max_gradient){
                max_gradient = magnitude;
            }
            if(magnitude < min_gradient || min_gradient == 0.0f){
                min_gradient = magnitude;
            }
        }
    }
}

glm::vec2 vector_field::sampleBilinear(float fx, float fy) const{
    int x0 = int(floor(fx)), y0 = int(floor(fy));
    int x1 = x0 + 1, y1 = y0 + 1;
    float sx = fx - x0, sy = fy - y0;
    glm::vec2 v00 = sample_value(x0, y0);
    glm::vec2 v10 = sample_value(x1, y0);
    glm::vec2 v01 = sample_value(x0, y1);
    glm::vec2 v11 = sample_value(x1, y1);
    glm::vec2 v0 = glm::mix(v00, v10, sx);
    glm::vec2 v1 = glm::mix(v01, v11, sx);
    return glm::mix(v0, v1, sy);
}


int vector_field::getHeight() const{ return h; }
int vector_field::getWidth() const{ return w; }

std::vector<glm::vec2> integrateStreamline(const vector_field &vf, glm::vec2 seed,
    float h, int maxSteps){
    std::vector<glm::vec2> pts;
    pts.reserve(maxSteps);
    glm::vec2 p = seed;
    pts.push_back(p);

    for(int i = 0; i < maxSteps; ++i){
        glm::vec2 k1 = vf.sampleBilinear(p.x, p.y);
        glm::vec2 k2 = vf.sampleBilinear(p.x + 0.5f * h * k1.x,
            p.y + 0.5f * h * k1.y);
        glm::vec2 k3 = vf.sampleBilinear(p.x + 0.5f * h * k2.x,
            p.y + 0.5f * h * k2.y);
        glm::vec2 k4 = vf.sampleBilinear(p.x + h * k3.x,
            p.y + h * k3.y);
        glm::vec2 dp = (k1 + 2.0f * k2 + 2.0f * k3 + k4) * (h / 6.0f);
        p += dp;
        if(p.x < 0 || p.y < 0 ||
            p.x >= vf.getWidth() || p.y >= vf.getHeight())
            break;
        pts.push_back(p);
    }
    return pts;
}
