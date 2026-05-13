#version 330
precision highp float;

layout(location=0)in vec3 aPos;

uniform mat4 MVP;
out float vHeight;

void main(){
	vHeight=aPos.y;
	gl_Position=MVP*vec4(aPos,1.);
}
