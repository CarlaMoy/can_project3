#version 330 core

/// modified from the OpenGL Shading Language Example "Orange Book"
/// Roost 2002

uniform mat4 MV;
uniform mat4 MVP;
uniform mat3 normalMatrix;
uniform mat4 textureMatrix;
uniform vec3 LightPosition;
uniform  vec4  inColour;


layout (location=0) in  vec4  inVert;
layout (location=1) in vec2 inUV;
layout (location=2) in  vec3  inNormal;

out vec4  ShadowCoord;
out vec4  Colour;
out vec2 FragmentTexCoord;
//out vec2 FragmentTexCoord;
void main()
{
	vec4 ecPosition = MV * inVert;
	vec3 ecPosition3 = (vec3(ecPosition)) / ecPosition.w;
	vec3 VP = LightPosition - ecPosition3;
	VP = normalize(VP);
	vec3 normal = normalize(normalMatrix * inNormal);
	float diffuse = max(0.0, dot(normal, VP));
	vec4 texCoord = textureMatrix * inVert;
	      ShadowCoord   = texCoord;
				FragmentTexCoord = inUV;

	Colour  = vec4(diffuse * inColour.rgb, inColour.a);
	gl_Position    = MVP * inVert;
}

/*#version 330 core
layout (location = 0) in vec3 inVert;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 FragPosLightSpace;
//uniform mat4 projection;
//uniform mat4 view;
//uniform mat4 model;
uniform mat4 lightSpaceMatrix;
uniform mat4 M;
uniform mat4 MV;
uniform mat4 MVP;
uniform mat3 normalMatrix;
//uniform mat4 textureMatrix;
//uniform vec3 LightPosition;
//uniform  vec4  inColour;
void main()
{
		gl_Position = MVP * vec4(inVert, 1.0f);
		FragPos = vec3(M * vec4(inVert, 1.0));
		Normal = normalMatrix * inNormal;
		TexCoords = inUV;
		FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
}*/
