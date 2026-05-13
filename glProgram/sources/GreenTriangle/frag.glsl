#version 330
precision highp float;

in float vHeight;

uniform vec3 color;
out vec4 fragmentColor;

void main(){
	fragmentColor=vec4(color * vHeight,1.);
}

