#version 330
precision highp float;

in float vHeight;

uniform vec3 color;
uniform int isCar;
out vec4 fragmentColor;

const vec3 waterDeep=vec3(.04,.16,.38);
const vec3 waterShallow=vec3(.10,.35,.58);
const vec3 sand=vec3(.80,.74,.52);
const vec3 grass=vec3(.22,.52,.18);
const vec3 stone=vec3(.48,.45,.42);
const vec3 snow=vec3(.92,.93,.97);

const float seaLevel=.13;

vec3 terrainColor(float h){
    vec3 water=mix(waterDeep,waterShallow,smoothstep(seaLevel-.25,seaLevel,h));
    
    vec3 land=sand;
    land=mix(land,grass,smoothstep(seaLevel+.04,seaLevel+.22,h));
    land=mix(land,stone,smoothstep(.65,.80,h));
    land=mix(land,snow,smoothstep(.82,.92,h));
    
    return mix(water,land,smoothstep(seaLevel-.02,seaLevel+.05,h));
}

void main(){
    if(isCar==1)
    fragmentColor=vec4(color,1.);
    else
    fragmentColor=vec4(terrainColor(vHeight)*color,1.);
}
