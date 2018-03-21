#version 330 core

in vec3 pos;
in vec3 normal;

out vec3 o_pos;
out vec3 o_normal;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

void main()
{	
	o_normal = normal;
	vec4 position =  M*vec4(pos,1);
	o_pos = position.xyz;

	gl_Position = P*V * position; 	
}