#version 440
layout(location=0) in vec2 qt_TexCoord0;
layout(location=0) out vec4 fragColor;
layout(std140,binding=0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float spread;
    float strength;
    float mode;
    vec4 tint;
    vec2 resolution;
};
void main() {
    vec2 p=(qt_TexCoord0-vec2(0.5))*resolution;
    float r=length(p);
    float core=exp(-r*r/(mode>0.5?34.0:2.6));
    float haze=exp(-r*r/max(1.0,spread*spread*0.55));
    float rayLength=mode>0.5?spread*1.3:spread*0.7;
    float rays=exp(-abs(p.x)*1.1-abs(p.y)/rayLength*3.0)+exp(-abs(p.y)*1.1-abs(p.x)/rayLength*3.0);
    vec2 diagonal=vec2(p.x+p.y,p.x-p.y)*0.70710678;
    rays+=0.35*(exp(-abs(diagonal.x)*1.4-abs(diagonal.y)/rayLength*5.0)+exp(-abs(diagonal.y)*1.4-abs(diagonal.x)/rayLength*5.0));
    float alpha=clamp(core+rays*(mode>1.5?0.0:0.65)+haze*(mode>1.5?0.75:mode>0.5?0.28:0.18),0.0,1.0)*strength;
    alpha=clamp(alpha,0.0,1.0);
    vec3 color=mix(tint.rgb,vec3(1.0,0.99,0.92),clamp(core+rays*0.4,0.0,1.0));
    if(mode>1.5)color=tint.rgb;
    fragColor=vec4(color*alpha,alpha)*qt_Opacity;
}
