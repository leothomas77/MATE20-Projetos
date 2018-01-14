#version 430

layout(location = 0) in vec4 aPosition;

smooth out vec4 fragPos;

void main() {
	fragPos = aPosition;

	gl_Position = aPosition;
}
//