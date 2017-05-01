#version 420 core
out vec4 FragColour;
in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D blur;

const float gamma = 2.2;

void main()
{

	  vec3 sceneColour = texture(scene, TexCoords).rgb;
		vec3 blurColour = texture(blur, TexCoords).rgb;
		vec3 result = sceneColour * blurColour;

		// also gamma correct while we're at it
		//result = pow(result, vec3(1.0 / gamma));
		FragColour = vec4(result, 1.0f);
                FragColour = texture(scene, TexCoords);
}
