#version 440
layout(location=0) in vec2 qt_TexCoord0;
layout(location=0) out vec4 fragColor;
layout(std140,binding=0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float lineLength;
    float spread;
    float margin;
    vec2 resolution;
    vec4 tint;
    float mode;
    float strength;
};
void main() {
    vec2 p=qt_TexCoord0*resolution;
    float d=length(vec2(p.x-clamp(p.x,margin,margin+lineLength),p.y-margin));
    float core=exp(-pow(d/(mode>0.5?1.4:0.85),2.0));
    float hot=exp(-pow(d/(mode>0.5?5.0:2.5),2.0));
    float wave=0.91+0.09*sin(p.x*0.071+sin(p.x*0.029)*1.7);
    float broad=exp(-pow(d/max(1.0,spread*wave),2.0));
    float ring=mode>0.5?exp(-pow((d-spread*0.72)/max(1.0,spread*0.17),2.0))*0.13:0.0;
    if(mode>1.5) { core=0.0;hot=0.0;ring=0.0; }
    float alpha=clamp(core+hot*0.45+broad*(mode>0.5?0.32:0.2)+ring,0.0,1.0)*strength;
    alpha=clamp(alpha,0.0,1.0);
    vec3 warm=mode>0.5 && mode<1.5?mix(tint.rgb*0.65,tint.rgb,hot):tint.rgb;
    vec3 color=mix(warm,vec3(1.0,0.99,0.91),core*(mode>0.5?1.0:0.12));
    fragColor=vec4(color*alpha,alpha)*qt_Opacity;
}
