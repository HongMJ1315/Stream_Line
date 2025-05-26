
#include <iostream>
#include <fstream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>


class vector_field {

private:
    std::vector<std::vector<glm::vec2>> vectors; // 2D vector field

    glm::vec2 sample_value(int x, int y) {
        // Ensure x and y are within bounds
        if (x < 0 || y < 0 || y >= vectors.size() || x >= vectors[y].size()) {
            return glm::vec2(0.0f, 0.0f); // Out of bounds, return zero vector
        }
        return vectors[y][x]; // Return the vector at (x, y)
    }

    glm::vec2 compute_point_gradient(int x, int y) {
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
        if (length > 0.0f) {
            gradient /= length; // Normalize to unit vector

        } else {
            gradient = glm::vec2(0.0f, 0.0f); // If length is zero, return zero vector
        }
        max_gradient = std::max(max_gradient, glm::length(gradient)); // Update max gradient
        if (min_gradient == 0.0f || glm::length(gradient) < min_gradient) {
            min_gradient = glm::length(gradient); // Update min gradient
        }
        return gradient; // Return the computed gradient
    }
    void compute_gradients() {
        for (int y = 0; y < vectors.size(); ++y) {
            for (int x = 0; x < vectors[y].size(); ++x) {
                glm::vec2 gradient = compute_point_gradient(x, y);
                float magnitude = glm::length(gradient);
                if (magnitude > max_gradient) {
                    max_gradient = magnitude;
                }
                if (magnitude < min_gradient || min_gradient == 0.0f) {
                    min_gradient = magnitude;
                }
            }
        }
    }
    public:
    float max_gradient = 0.0f; // Maximum gradient magnitude
    float min_gradient = 0.0f; // Minimum gradient magnitude
    vector_field(const std::string &filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open data file: " << filename << std::endl;
            return;
        }
        int w, h;
        file >> w >> h;
        vectors.resize(h, std::vector<glm::vec2>(w));
        for(int i = 0; i < h; ++i) {
            for (int j = 0; j < w; ++j) {
                glm::vec2 v;
                file >> v.x >> v.y;
                vectors[i][j] = v;
            }
        }
        file.close();
        compute_gradients(); // Compute gradients after loading data
        
    }

};