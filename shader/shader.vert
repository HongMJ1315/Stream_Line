#version 400 core
layout(location=0) in vec2 aPos;
uniform mat4 uMVP;
void main(){
    gl_Position = uMVP * vec4(aPos, 0, 1);
}
