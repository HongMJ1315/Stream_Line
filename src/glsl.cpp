#include "glsl.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

int Shader::numShaders = 0;

Shader::Shader(const char *vertexPath, const char *fragmentPath){
    ID = glCreateProgram();

    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);

    char *vSrc = read_source_codes(const_cast<char *>(vertexPath));
    char *fSrc = read_source_codes(const_cast<char *>(fragmentPath));

    std::cerr << "Vertex Shader Source:\n" << vSrc << "\n";
    glShaderSource(vert, 1, &vSrc, NULL);
    glCompileShader(vert);
    print_shader_info_log(vert);

    std::cerr << "Fragment Shader Source:\n" << fSrc << "\n";
    glShaderSource(frag, 1, &fSrc, NULL);
    glCompileShader(frag);
    print_shader_info_log(frag);

    free(vSrc);
    free(fSrc);

    glAttachShader(ID, vert);
    glAttachShader(ID, frag);
    glLinkProgram(ID);
    print_prog_info_log(ID);

    glDeleteShader(vert);
    glDeleteShader(frag);

    numShaders++;
    std::cout << "Created Shader program ID = " << ID
        << "  (total shaders: " << numShaders << ")\n";
}

Shader::~Shader(){
    glDeleteProgram(ID);
    numShaders--;
}

void Shader::setBool(const std::string &name, bool value) const{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int) value);
}
void Shader::setInt(const std::string &name, int value) const{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}
void Shader::setFloat(const std::string &name, float value) const{
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}
void Shader::setVec2(const std::string &name, const glm::vec2 &v) const{
    glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(v));
}
void Shader::setVec3(const std::string &name, const glm::vec3 &v) const{
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(v));
}
void Shader::setMat4(const std::string &name, const glm::mat4 &m) const{
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()),
        1, GL_FALSE, glm::value_ptr(m));
}
