#version 420 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gColour;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D labelMap;
//uniform sampler2D texture_specular1;

void main()
{
    // Store the fragment position vector in the first gbuffer texture
    gPosition = FragPos;
    // Also store the per-fragment normals into the gbuffer
    gNormal = normalize(Normal);
    // And the diffuse per-fragment color
    gColour = texture(labelMap, TexCoords).rgb;
    // Store specular intensity in gAlbedoSpec's alpha component
  //  gColour.a = texture(texture_specular1, TexCoords).r;
}
