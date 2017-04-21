#version 420 core

// Attributes passed on from the vertex shader
smooth in vec3 FragmentPosition;
smooth in vec3 FragmentNormal;
smooth in vec2 FragmentTexCoord;
smooth in vec4 Colour;
smooth in vec4 ShadowCoord;

uniform sampler2D ShadowMap;


/// @brief our output fragment colour
layout (location=0) out vec4 FragColour;

uniform vec3 LightPosition;

// Structure for holding light parameters
struct LightInfo {
    vec4 Position; // Light position in eye coords.
    vec3 La;
    vec3 Ld;
    vec3 Ls;
    vec3 Intensity;
};

// We'll have a single light in the scene with some default values
uniform LightInfo Light = LightInfo(
            vec4(0.0, 0.0, 3.0, 1.0),   // position
            vec3(0.5, 0.5, 0.5),        // La
            vec3(1.0, 1.0, 1.0),        // Ld
            vec3(1.0, 1.0, 1.0),        // Ls
            vec3(1.0, 1.0, 1.0)         //Intensity

            );

//struct LightInfo Light = LightInfo(
 //                                   vec4 Position;
 //                                   vec3 Intensity;
  //                                  );

//uniform LightInfo lights[3];

// The material properties of our object
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

// Set our gloss map texture
uniform sampler2D glossMap;

//Set label map texture
uniform sampler2D labelMap;

//Set bump map texture
uniform sampler2D bumpMap;

// The inverse View matrix
uniform mat4 invV;

// A toggle allowing you to set it to reflect or refract the light
uniform bool isReflect = true;

// Specify the refractive index for refractions
uniform float refractiveIndex = 1.0;



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

/*vec3 ADS(int lightIndex, vec3 pos, vec3 norm )
{
    // Calculate the normal (this is the expensive bit in Phong)
    //vec3 n = normalize( FragmentNormal );

    // Calculate the eye vector
    vec3 v = normalize(vec3(-pos));

    // Extract the normal from the normal map (rescale to [-1,1]
 //   vec3 tgt = normalize(texture(bumpMap, FragmentTexCoord).rgb * 2.0 - 1.0);

    // The source is just up in the Z-direction
 //   vec3 src = vec3(0.0, 0.0, 1.0);

    // Perturb the normal according to the target
   // vec3 np = rotateVector(src, tgt, n);

    vec3 s = normalize( vec3(lights[lightIndex].Position - (pos,1.0) ));

    // Reflect the light about the surface normal
    //vec3 r = reflect( -s, np );
    vec3 halfVec = normalize(v + s);

    vec3 I = lights[lightIndex].Intensity;

    return I * ( Material.Ka +
                 Material.Kd * max( dot(s, norm), 0.0) +
                 Material.Ks * pow(max( dot(halfVec, norm), 0.0), Material.Shininess));
}*/

//________________________________________________________________________________________________________________________________________//


vec3 diffAndSpecShading()
{

    // Calculate the normal (this is the expensive bit in Phong)
    vec3 n = FragmentNormal;

    // Calculate the eye vector
    vec3 v = normalize(vec3(-FragmentPosition));

    vec3 s;
    if(Light.Position.w == 0.0)
        s = normalize (vec3(Light.Position));
    else
        s = normalize( vec3(Light.Position) - FragmentPosition );


    vec3 halfVec = normalize(v + s);

    // Compute the light from the diffuse and specular components
    vec3 diffSpec = (
            Light.Intensity * (
            Light.Ld * Material.Kd * max( dot(s, n), 0.0 ) +
            Light.Ls * Material.Ks * pow( max( dot(halfVec, n), 0.0 ), Material.Shininess )));

    return diffSpec;
}


//________________________________________________________________________________________________________________________________________//

subroutine void RenderPassType();
subroutine uniform RenderPassType RenderPass;

subroutine (RenderPassType)
void shadeWithShadow()
{
    vec3 ambient = Light.La * Material.Ka;
    vec3 diffAndSpec = diffAndSpecShading();

    //Do shadow map lookup
    float shadow = float(textureProj(ShadowMap, ShadowCoord));

    //if the fragment is in shadow, use ambient only
    FragColour = vec4(diffAndSpec * shadow + ambient, 1.0);
}



//________________________________________________________________________________________________________________________________________//


subroutine (RenderPassType)
void recordDepth()
{

}


//________________________________________________________________________________________________________________________________________//





//________________________________________________________________________________________________________________________________________//


void main () {

  //  RenderPass();

    // Calculate the normal (this is the expensive bit in Phong)
    vec3 n = FragmentNormal;

    // Calculate the eye vector
    vec3 v = normalize(vec3(-FragmentPosition));

    vec3 VP = LightPosition - FragmentPosition;
    VP = normalize(VP);

    vec3 lookup;

    if (isReflect) {
        lookup = reflect(v,n);
    } else {
        lookup = refract(v,n,refractiveIndex);
    }


    // Extract the normal from the normal map (rescale to [-1,1]
    vec3 tgt = normalize(texture(bumpMap, FragmentTexCoord).rgb * 2.0 - 1.0);

    // The source is just up in the Z-direction
    vec3 src = vec3(0.0, 0.0, 1.0);

    // Perturb the normal according to the target
    vec3 np = rotateVector(src, tgt, n);

    vec3 s;
    if(Light.Position.w == 0.0)
      //  s = normalize (vec3(Light.Position));
        s = normalize (LightPosition);
    else
      //  s = normalize( vec3(Light.Position) - FragmentPosition );
        s = normalize( LightPosition - FragmentPosition );

  //  diffAndSpecShading();

    // Reflect the light about the surface normal
    //vec3 r = reflect( -s, np );
    vec3 h = normalize(v + s);

    // Compute the light from the ambient, diffuse and specular components
    vec3 lightColour = (
            Light.Intensity * (
            Light.La * Material.Ka +
            Light.Ld * Material.Kd * max( dot(s, n), 0.0 ) +
            Light.Ls * Material.Ks * pow( max( dot(h, n), 0.0 ), Material.Shininess )));

    // The mipmap level is determined by log_2(resolution), so if the texture was 4x4,
    // there would be 8 mipmap levels (128x128,64x64,32x32,16x16,8x8,4x4,2x2,1x1).
    // The LOD parameter can be anything inbetween 0.0 and 8.0 for the purposes of
    // trilinear interpolation.

    // This call actually finds out the current LOD
    float lod = textureQueryLod(envMap, lookup).x;

    // Determine the gloss value from our input texture, and scale it by our LOD resolution
    float gloss = (1 - texture(glossMap, FragmentTexCoord*2).r) * float(envMaxLOD);

    // This call determines the current LOD value for the texture map
    vec4 colour = textureLod(envMap, lookup, gloss);

    // This call just retrieves whatever you want from the environment map
 //   vec4 colour = texture(envMap, lookup);
    //float shadeFactor = textureProj(ShadowMap, ShadowCoord).x;


  //  vec3 colour = vec3(0.0);
//   for(int i = 0; i < 5; ++i)
//        colour += ADS(i, FragmentPosition, FragmentNormal);

  //  FragColour = vec4(colour, 1.0) * texture(labelMap, FragmentTexCoord);

    float gamma = 2.2;

    float NdotH = dot(n,h);

    float r1 = 1.0/(4.0 * Material.Roughness * pow(NdotH, 4.0));
    float r2 = (NdotH * NdotH - 1.0)/ (Material.Roughness * NdotH * NdotH);
    float roughness = r1 * exp(r2);
    FragColour =  texture(labelMap, FragmentTexCoord) * vec4(lightColour,1.0) * colour * roughness;// * shadeFactor;*/
    FragColour.rgb = pow(FragColour.rgb, vec3(1.0/gamma));
  //  FragColour *= roughness;
   // FragColour = vec4(lightColour,1.0) * colour;
   // FragColour = colour;
}

