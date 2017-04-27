#version 420 core

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
            100.0,                  // Shininess
            0.5);                   //roughness


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


    return Light[lightIndex].Intensity * (ambient + diffuse + specular) * attenuation;

}

//________________________________________________________________________________________________________________________________________//

//________________________________________________________________________________________________________________________________________//

/** From http://www.neilmendoza.com/glsl-rotation-about-an-arbitrary-axis/
  */
mat4 rotationMatrix(vec3 axis, float angle)
{
    //axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

/**
  * Rotate a vector vec by using the rotation that transforms from src to tgt.
  */
vec3 rotateVector(vec3 src, vec3 tgt, vec3 vec) {
    float angle = acos(dot(src,tgt));

    // Check for the case when src and tgt are the same vector, in which case
    // the cross product will be ill defined.
    if (angle == 0.0) {
        return vec;
    }
    vec3 axis = normalize(cross(src,tgt));
    mat4 R = rotationMatrix(axis,angle);

    // Rotate the vec by this rotation matrix
    vec4 _norm = R*vec4(vec,1.0);
    return _norm.xyz / _norm.w;
}

//________________________________________________________________________________________________________________________________________//


void main ()
{

  // Calculate the normal (this is the expensive bit in Phong)
  vec3 n = FragmentNormal;

  // Calculate the eye vector
  vec3 v = normalize(vec3(-FragmentPosition));

  vec3 lookup;

  if (isReflect) {
      lookup = reflect(v,n);
  } else {
      lookup = refract(v,n,refractiveIndex);
  }

  // This call actually finds out the current LOD
  float lod = textureQueryLod(envMap, lookup).x;

  // Determine the gloss value from our input texture, and scale it by our LOD resolution
  float gloss = (1 - texture(specMap, FragmentTexCoord*2).r) * float(envMaxLOD);

  // This call determines the current LOD value for the texture map
  vec4 colour = textureLod(envMap, lookup, gloss*0.01);

  vec4 woodDiffuse = texture(difMap, FragmentTexCoord*6);


  float shadeFactor=textureProj(ShadowMap,ShadowCoord).x;
   //     shadeFactor=shadeFactor;// * 0.25 + 0.75;
//  if(shadeFactor<= 0.3)
//     outColour=vec4(0.1,0.1,0.1,1.0);
//  else
//     outColour=vec4(0.7,0.7,0.7,1.0);
  vec3 lightIntensity = vec3(0.0);
  for(int i = 0; i<3; ++i)
  {
   //   lightIntensity += Microfacet(i, n, v, texturedCan, colour);
      lightIntensity += BlinnPhong(i, n, v);
  }

  outColour= shadeFactor * vec4(lightIntensity, 1.0) * 2 * woodDiffuse;// * texture(textureMap, FragmentTexCoord);
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
