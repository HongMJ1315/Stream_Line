#include <vector>
#include <string>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

class vector_field{
private:
    int w, h;
    std::vector<std::vector<glm::vec2>> vectors; // 2D vector field
    glm::vec2 sample_value(int x, int y) const;
    glm::vec2 compute_point_gradient(int x, int y);
    void compute_gradients();
public:
    float max_gradient = 0.0f; // Maximum gradient magnitude
    float min_gradient = 0.0f; // Minimum gradient magnitude
    vector_field(const std::string &filename);
    vector_field();
    glm::vec2 sampleBilinear(float fx, float fy) const;
    int getWidth() const;
    int getHeight() const;
};

std::vector<glm::vec2> integrateStreamline(
    const vector_field &vf,
    glm::vec2 seed,
    float h = 0.5f,
    int maxSteps = 500);