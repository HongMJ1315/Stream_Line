#include"GLinclude.h"
#include "vector_field.h"

GLuint makeNoiseTex(int);
GLuint makeVectorTex(const vector_field &);
std::pair<GLuint, GLuint> initLICQuad(vector_field &);