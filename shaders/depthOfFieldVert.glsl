/*#version 430

// The vertex position attribute
layout (location=0) in vec3 VertexPosition;



// We need an MVP because the plane needs to be rotated
uniform mat4 MVP;

void main() {
    // Set the position of the current vertex
    gl_Position = MVP * vec4(VertexPosition, 1.0);


}*/

#version 420 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

uniform vec4 MVP;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 1.0);
}
