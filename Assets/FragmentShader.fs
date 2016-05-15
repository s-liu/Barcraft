#version 330

in vec3 vsColour;

out vec4 fragColor;

void main() {
	fragColor = vec4( vsColour, 1 );
}
