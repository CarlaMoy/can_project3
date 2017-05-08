#version 420 core

// Attributes passed on from the vertex shader
smooth in vec3 FragmentPosition;
smooth in vec3 FragmentNormal;
smooth in vec2 FragmentTexCoord;
smooth in vec4 Colour;
smooth in vec4 ShadowCoord;
smooth in vec3 eyeDirection;

uniform sampler2D ShadowMap;

uniform vec2 iResolution;


/// @brief our output fragment colour
layout (location=0) out vec4 FragColour;

layout (location=3) in vec4 gPosition;

layout (location=4) in vec4 gNormal;

layout (location=5) in vec4 gColour;

//uniform vec3 LightPosition;



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


// The material properties of our object
struct MaterialInfo {
    vec3 Ka; // Ambient reflectivity
    vec3 Kd; // Diffuse reflectivity
    vec3 Ks; // Specular reflectivity
    float Shininess; // Specular shininess factor
    float Roughness; // Roughness factor
};

// The object has a material
uniform MaterialInfo Material = MaterialInfo(
            vec3(0.2, 0.2, 0.2),    // Ka
            vec3(0.6, 0.6, 0.6),    // Kd
            vec3(0.8, 0.8, 0.8),    // Ks
            100.0,                  // Shininess
            0.01);                   //roughness

// A texture unit for storing the 3D texture
uniform samplerCube envMap;

// Set the maximum environment level of detail (cannot be queried from GLSL apparently)
uniform int envMaxLOD = 10;

// Set our gloss map texture
uniform sampler2D glossMap;

//Set label map texture
uniform sampler2D labelMap;

//Set bump map texture
uniform sampler2D normalMap;

// The inverse View matrix
uniform mat4 invV;

// A toggle allowing you to set it to reflect or refract the light
uniform bool isReflect = true;

// Specify the refractive index for refractions
uniform float refractiveIndex = 1.0;


vec3 GroundColour = vec3(0.3, 0.15,0.1);


float gamma = 1.5;



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


vec3 BlinnPhong(int lightIndex, vec3 _n, vec3 _v)
{
    vec3 s;
    if(Light[lightIndex].Position.w == 0.0)
        s = normalize (vec3(Light[lightIndex].Position));
    else
        s = normalize( vec3(Light[lightIndex].Position) - FragmentPosition );


    vec3 h = normalize(_v + s);

    float NdotS = dot(_n,s);

    float a = NdotS;// * 0.5 + 0.5;

    vec3 diffuse = mix(GroundColour, Light[lightIndex].Intensity, a) * Light[lightIndex].Ld * Material.Kd; //Hemisphere lighting, taken from OpenGL Programming Guide, Eight Edition

    vec3 ambient = Light[lightIndex].La * Material.Ka;

    //  vec3 diffuse = Light[lightIndex].Ld * Material.Kd * max( NdotS, 0.0 );

    vec3 specular = vec3(0.0);

    if(NdotS > 0.0)
    {
        specular = Light[lightIndex].Ls * Material.Ks * pow( max( dot(h, _n), 0.0 ), Material.Shininess);

    }

    //attenuation
    float dist = length(Light[lightIndex].Position.xyz - FragmentPosition);
    float attenuation = 1.0/ (1.0 + Light[lightIndex].Linear * dist + Light[lightIndex].Quadratic * (dist * dist));


    return Light[lightIndex].Intensity * (ambient + diffuse + specular*6) * attenuation;

}

//________________________________________________________________________________________________________________________________________//

//Geometric Attenuation function
//Code taken from myBU: Principles of Rendering/Unit Materials/Realtime Rendering Content/shading.pdf
float calculateGeometricAttenuation(in float _NdotH, in float _NdotV, in float _NdotS, in vec3 _h)
{
    float NH2 = 2.0f * _NdotH;
    float invVdotH = 1.0f / max(dot(eyeDirection, _h), 0.0f);
    float g1 = (NH2 * _NdotV) * invVdotH;
    float g2 = (NH2 * _NdotS) * invVdotH;

    return min(1.0f, min(g1, g2));
}

//________________________________________________________________________________________________________________________________________//


// Schlick approximation function based on lecture slides
// Code taken from myBU: Principles of Rendering/Unit Materials/Realtime Rendering Content/shading.pdf
float calculateFresnel(in vec3 _h)
{
    float VdotH = max(dot(eyeDirection, _h), 0.0f);
    float F0 = 0.4f;
    return pow(1.0f - VdotH, 5.0f) * (1.0f - F0) + F0;
}

//________________________________________________________________________________________________________________________________________//


//Beckmann Distribution function
// Code taken from myBU: Principles of Rendering/Unit Materials/Realtime Rendering Content/shading.pdf
float calculateBeckmannDisribution(in float _NdotH)
{
    float r1 = 1.0f / (4.0f * Material.Roughness * pow(_NdotH, 4.0f));
    float r2 = (_NdotH * _NdotH - 1.0f) / (Material.Roughness * _NdotH * _NdotH);

    return r1 * exp(r2);
}

//________________________________________________________________________________________________________________________________________//

vec3 lightContribution(int lightIndex, in vec3 perturbedNormal)
{

    // Calculate the eye vector
    vec3 v = normalize(vec3(-FragmentPosition));

    vec3 s;
    if(Light[lightIndex].Position.w == 0.0)
        s = normalize (vec3(Light[lightIndex].Position));
    else
        s = normalize( vec3(Light[lightIndex].Position) - FragmentPosition );


    vec3 h = normalize(v + s);

    float NdotS = dot(perturbedNormal,s);
    float NdotH = dot(perturbedNormal,h);
    float NdotV = dot(perturbedNormal,v);


    float DGF =  calculateBeckmannDisribution(NdotH) * calculateGeometricAttenuation(NdotH, NdotV, NdotS, h) * calculateFresnel(h);

    vec3 ambient = Light[lightIndex].La * Material.Ka;

    vec3 diffuse = Light[lightIndex].Ld * Material.Kd * max( NdotS, 0.0 );

    vec3 specular = vec3(0.0);

    if(NdotS > 0.0)
    {
        specular = ((Light[lightIndex].Ls * Material.Ks * DGF) / max(dot(perturbedNormal, h), 0));
    }

    //attenuation
    float dist = length(Light[lightIndex].Position.xyz - FragmentPosition);
    float attenuation = 1.0/ (1.0 + Light[lightIndex].Linear * dist + Light[lightIndex].Quadratic * (dist * dist));


    return Light[lightIndex].Intensity * (ambient + diffuse + specular * 5) * attenuation;

}

//________________________________________________________________________________________________________________________________________//

//Code taken from https://thebookofshaders.com/13/
// Author @patriciogv - 2015
// http://patriciogonzalezvivo.com

#ifdef GL_ES
precision mediump float;
#endif

vec2 u_resolution = vec2(0.1,0.2);
vec2 u_mouse = vec2(0.2,0.3);
//uniform float u_time;

float random (in vec2 _st) {
    return fract(sin(dot(_st.xy,
                         vec2(12.9898,78.233)))*
                 43758.5453123);
}

// Based on Morgan McGuire @morgan3d
// https://www.shadertoy.com/view/4dS3Wd
float noise (in vec2 _st) {
    vec2 i = floor(_st);
    vec2 f = fract(_st);

    // Four corners in 2D of a tile
    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(a, b, u.x) +
            (c - a)* u.y * (1.0 - u.x) +
            (d - b) * u.x * u.y;
}
float NUM_OCTAVES = 8;

float fbm ( in vec2 _st) {
    float v = 0.0;
    float a = 0.5;
    vec2 shift = vec2(100.0);
    // Rotate to reduce axial bias
    mat2 rot = mat2(cos(0.5), sin(0.5),
                    -sin(0.5), cos(0.50));
    for (int i = 0; i < NUM_OCTAVES; ++i) {
        v += a * noise(_st);
        _st = rot * _st * 2.0 + shift;
        a *= 0.5;
    }
    return v;
}

vec4 generateNoise()
{
    vec2 st = FragmentTexCoord.xy/u_resolution.xy*3.;
    // st += st * abs(sin(u_time*0.1)*3.0);
    vec3 color = vec3(0.0);

    vec2 q = vec2(0.);
    q.x = fbm( st );
    q.y = fbm( st + vec2(1.0));

    vec2 r = vec2(0.);
    r.x = fbm( st + 1.0*q + vec2(1.7,9.2)+ 0.15);
    r.y = fbm( st + 1.0*q + vec2(8.3,2.8)+ 0.126);

    float f = fbm(st+r);

    color = mix(vec3(0.101961,0.619608,0.666667),
                vec3(0.666667,0.666667,0.498039),
                clamp((f*f)*4.0,0.0,1.0));

    color = mix(color,
                vec3(0,0,0.164706),
                clamp(length(q),0.0,1.0));

    color = mix(color,
                vec3(0.666667,1,1),
                clamp(length(r.x),0.0,1.0));

    return vec4((f*f*f+.6*f*f+.5*f)*color*0.1,1.0);
}
//End of code taken from https://thebookofshaders.com/13/
//________________________________________________________________________________________________________________________________________//


void main ()
{
    // Calculate the normal (this is the expensive bit in Phong)
    vec3 n = normalize(FragmentNormal);

    // Calculate the eye vector
    vec3 v = normalize(vec3(-FragmentPosition));


    vec3 lookup;

    if (isReflect) {
        lookup = reflect(v,n);
    } else {
        lookup = refract(v,n,refractiveIndex);
    }


    // Extract the normal from the normal map (rescale to [-1,1]
       vec3 tgt = normalize(texture(normalMap, FragmentTexCoord).rgb * 2.0 - 1.0);

    // The source is just up in the Z-direction
        vec3 src = vec3(0.0, 0.0, 1.0);

    // Perturb the normal according to the target
        vec3 np = rotateVector(src, tgt, n);


    // This call actually finds out the current LOD
    float lod = textureQueryLod(envMap, lookup).x;

    // Determine the gloss value from our input texture, and scale it by our LOD resolution
    float gloss = (1 - texture(glossMap, FragmentTexCoord*2).r) * float(envMaxLOD);

    // This call determines the current LOD value for the texture map
    vec4 colour = textureLod(envMap, lookup, gloss*0.01);

    vec4 texturedCan = texture(labelMap, FragmentTexCoord);



    vec3 lightIntensity = vec3(0.0);
    for(int i = 0; i<3; ++i)
    {
        //interchange between lighting models
    //    lightIntensity += BlinnPhong(i, n, v);
        lightIntensity += lightContribution(i, np);
    }



    FragColour = texturedCan * vec4(lightIntensity,1.0) * colour + generateNoise();
    FragColour.rgb = pow(FragColour.rgb, vec3(1.0/gamma));

}
