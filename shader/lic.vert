#version 330 core
layout(location=0) in vec2 aPos;    // 位置，单位：矢量场像素
layout(location=1) in vec2 aUV;     // 采样 UV [0,1]
uniform mat4 uMVP;
out vec2 vUV;
void main(){
  vUV = aUV;
  gl_Position = uMVP * vec4(aPos, 0.0, 1.0);
}
