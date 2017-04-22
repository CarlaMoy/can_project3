#version 330 core

/// modified from the OpenGL Shading Language Example "Orange Book"
/// Roost 2002

in vec4 Colour;
layout (location=0) out vec4 outColour;
// for mac we need this
//uniform sampler2DShadow ShadowMap;
// for windows unix we need
uniform sampler2D ShadowMap;

uniform sampler2D textureMap;

in vec4 ShadowCoord;
in vec2 FragmentTexCoord;


void main ()
{
  float shadeFactor=textureProj(ShadowMap,ShadowCoord).x;
   //     shadeFactor=shadeFactor;// * 0.25 + 0.75;
//  if(shadeFactor<= 0.3)
//     outColour=vec4(0.1,0.1,0.1,1.0);
//  else
//     outColour=vec4(0.7,0.7,0.7,1.0);

  outColour=vec4(shadeFactor * Colour.rgb, Colour.a) * texture(textureMap, FragmentTexCoord);
//  outColour = vec4(0.2,0.2,0.6,1.0);
 // outColour = vec4(shadeFactor* vec4(1.0,1.0,0.0,1.0));
  //outColour=vec4(ShadowCoord);

}

/*#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;


//uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;

uniform vec3 lightPos;
uniform vec3 viewPos;

float ShadowCalculation(vec4 fragPosLightSpace, float bias)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // Get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    // Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // Check whether current frag pos is in shadow
    float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;

    return shadow;
}

void main()
{



   // vec3 color = texture(diffuseTexture, TexCoords).rgb;
    vec3 normal = normalize(Normal);
    vec3 lightColor = vec3(1.0);
    // Ambient
    vec3 ambient = vec3(0.15);// * color;
    // Diffuse
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    // Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);
    spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;
    // Calculate shadow

    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    float shadow = ShadowCalculation(FragPosLightSpace, bias);
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular));// * color;

    FragColor = vec4(lighting, 1.0f);
}*/
