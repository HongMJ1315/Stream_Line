#version 400 core
out vec4 FragColor;
in vec2 vUV;            // 屏幕空间 UV

uniform sampler2D uNoise;
uniform sampler2D uVectorField;
uniform float uStepSize;    // e.g. 1.0/512
uniform int   uNumSteps;    // e.g. 20

void main(){
    float sum = 0.0;
    float wsum = 0.0;
    vec2 uv = vUV;

    // 向前采样
    vec2 pos = uv;
    for(int i=0;i<uNumSteps;i++){
        vec2 dir = texture(uVectorField, pos).xy;
        pos += dir * uStepSize;
        float w = 1.0 - float(i)/float(uNumSteps);
        sum  += w * texture(uNoise,pos).r;
        wsum += w;
    }
    // 向后采样
    pos = uv;
    for(int i=0;i<uNumSteps;i++){
        vec2 dir = texture(uVectorField, pos).xy;
        pos -= dir * uStepSize;
        float w = 1.0 - float(i)/float(uNumSteps);
        sum  += w * texture(uNoise,pos).r;
        wsum += w;
    }
    float lic = sum / wsum;
    FragColor = vec4(vec3(lic),1.0);
}
