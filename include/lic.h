#include"GLinclude.h"
#include "vector_field.h"

GLuint build_noise_tex(int);
GLuint build_vector_tex(const vector_field &);
std::pair<GLuint, GLuint> init_lic_quad(vector_field &);