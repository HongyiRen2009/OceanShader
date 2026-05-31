#version 460 core
in vec3 position;
out vec3 localPos;
uniform mat4 matProj;
uniform mat4 matView;

void main() {
	localPos = position;
	gl_Position = matProj * matView * vec4(position, 1.0);
}
