#version 420 core

/// modified from the OpenGL Shading Language Example "Orange Book"
/// Roost 2002

in vec4 Colour;
layout (location=0) out vec4 outColour;

uniform sampler2D ShadowMap;
uniform sampler2D textureMap;


in vec4 ShadowCoord;
in vec2 FragmentTexCoord;
in vec3 FragmentNormal;
in vec3 FragmentPosition;

// Structure for holding light parameters
struct LightInfo {
    vec4 Position; // Light position in eye coords.
    vec3 La;
    vec3 Ld;
    vec3 Ls;
    vec3 Intensity;
    float Linear;
    float Quadratic;
};

uniform LightInfo Light[3];

struct MaterialInfo {
    vec3 Ka; // Ambient reflectivity
    vec3 Kd; // Diffuse reflectivity
    vec3 Ks; // Specular reflectivity
    float Shininess; // Specular shininess factor
    float Roughness;
};

// The object has a material
uniform MaterialInfo Material = MaterialInfo(
            vec3(0.2, 0.2, 0.2),    // Ka
            vec3(0.6, 0.6, 0.6),    // Kd
            vec3(0.8, 0.8, 0.8),    // Ks
            50.0,                  // Shininess
            0.01);                   //roughness


// A texture unit for storing the 3D texture
uniform samplerCube envMap;

// Set the maximum environment level of detail (cannot be queried from GLSL apparently)
uniform int envMaxLOD = 10;

// Set spec map texture
uniform sampler2D specMap;

//Set diffuse map texture
uniform sampler2D difMap;

//Set normal map texture
uniform sampler2D normMap;


// A toggle allowing you to set it to reflect or refract the light
uniform bool isReflect = true;

// Specify the refractive index for refractions
uniform float refractiveIndex = 1.0;

float shadeAmount = 4.0;

//________________________________________________________________________________________________________________________________________//


vec3 BlinnPhong(int lightIndex, vec3 _n, vec3 _v)
{



    vec3 s;
    if(Light[lightIndex].Position.w == 0.0)
        s = normalize (vec3(Light[lightIndex].Position));
    else
        s = normalize( vec3(Light[lightIndex].Position) - FragmentPosition );


    vec3 h = normalize(_v + s);

    float NdotS = dot(_n,s);

    vec3 ambient = Light[lightIndex].La * Material.Ka;

    vec3 diffuse = Light[lightIndex].Ld * Material.Kd * max( NdotS, 0.0 );

    vec3 specular = vec3(0.0);

    if(NdotS > 0.0)
    {
        specular = Light[lightIndex].Ls * Material.Ks * pow( max( dot(h, _n), 0.0 ), Material.Shininess);

    }

    //attenuation
    float dist = length(Light[lightIndex].Position.xyz - FragmentPosition);
    float attenuation = 1.0/ (1.0 + Light[lightIndex].Linear * dist + Light[lightIndex].Quadratic * (dist * dist));


    return Light[lightIndex].Intensity * (ambient + diffuse + specular*10) * attenuation;

}


//________________________________________________________________________________________________________________________________________//


void main ()
{

    // Calculate the normal (this is the expensive bit in Phong)
    vec3 n = FragmentNormal;

    // Calculate the eye vector
    vec3 v = normalize(vec3(-FragmentPosition));

    vec4 woodDiffuse = texture(difMap, FragmentTexCoord*10);


    float shadeFactor=textureProj(ShadowMap,ShadowCoord).x;
    shadeFactor *= pow(shadeFactor, shadeAmount);



    vec3 lightIntensity = vec3(0.0);
    for(int i = 0; i<3; ++i)
    {
        //   lightIntensity += Microfacet(i, n, v, texturedCan, colour);
        lightIntensity += BlinnPhong(i, n, v);
    }

    outColour= shadeFactor * vec4(lightIntensity, 1.0) * woodDiffuse;

}

