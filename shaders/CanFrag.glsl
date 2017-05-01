#version 420 core

// Attributes passed on from the vertex shader
smooth in vec3 FragmentPosition;
smooth in vec3 FragmentNormal;
smooth in vec2 FragmentTexCoord;
smooth in vec4 Colour;
smooth in vec4 ShadowCoord;

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

// We'll have a single light in the scene with some default values
/*uniform LightInfo Light = LightInfo(
						vec4(0.0, 0.0, 3.0, 1.0),   // position
						vec3(0.5, 0.5, 0.5),        // La
						vec3(1.0, 1.0, 1.0),        // Ld
						vec3(1.0, 0.0, 0.0),        // Ls
						vec3(1.0, 1.0, 1.0)         //Intensity
						);*/
uniform LightInfo Light[3];

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

float Fbias = 0.0;
float Fscale = 0.3;
float Fpower = 0.2; //Fresnel bias, fresnel scale and fresnel power

vec3 GroundColour = vec3(0.3, 0.15,0.1);



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


float MicrofacetDistribution(float _roughness, float _NdotH)
{
	  float r1 = 1.0/(4.0 * _roughness * pow(_NdotH, 4.0));
		float r2 = (_NdotH * _NdotH - 1.0)/ (_roughness * _NdotH * _NdotH);
		return r1 * exp(r2);
}


//________________________________________________________________________________________________________________________________________//


float MicrofacetGA(float _NdotU, float _k)
{
	  return _NdotU/((_NdotU * (1-_k)) + _k);
}

//________________________________________________________________________________________________________________________________________//

float MicrofacetFresnel(float _Fo, float _VdotH)
{
	  return _Fo + ((1 - _Fo) * pow(2, (-5.55473*_VdotH - 6.98316*_VdotH)));
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

vec3 Microfacet(int lightIndex, vec3 _n, vec3 _v, vec4 _texturedCan, vec4 _colour)
{



	  vec3 s;
		if(Light[lightIndex].Position.w == 0.0)
			  s = normalize (vec3(Light[lightIndex].Position));
		else
			  s = normalize( vec3(Light[lightIndex].Position) - FragmentPosition );


		vec3 h = normalize(_v + s);

		float NdotS = dot(_n,s);
		float NdotH = dot(_n,h);
		float NdotV = dot(_n,_v);
		//  float NdotS = dot(n,s);
		// float VdotH = dot(v,h);

		float k = pow((Material.Roughness + 1),2)/8;
 //   float Fo = pow(1- VdotH, 5);

		float D = MicrofacetDistribution(Material.Roughness, NdotH);
		float nvGA = MicrofacetGA(NdotV, k);
		float nsGA = MicrofacetGA(NdotS, k);
		float G = nvGA * nsGA;

		float FresnelTerm = Fbias +Fscale * pow(1.0 + dot(_v, _n), Fpower);
		vec4 F = mix(vec4(0.5,0.5,0.5,1.0), _colour, FresnelTerm);

		float DGF = D * G * F.x * F.y * F.z;

		vec3 ambient = Light[lightIndex].La * Material.Ka;

		vec3 diffuse = Light[lightIndex].Ld * Material.Kd * max( NdotS, 0.0 );

		vec3 specular = vec3(0.0);

		if(NdotS > 0.0)
		{
			  //specular = Light[lightIndex].Ls * Material.Ks * pow( max( dot(h, _n), 0.0 ), Material.Shininess);
			  specular = ((Light[lightIndex].Ls * Material.Ks * DGF) / max(dot(h, _n), 0));
		}

		//attenuation
		float dist = length(Light[lightIndex].Position.xyz - FragmentPosition);
		float attenuation = 1.0/ (1.0 + Light[lightIndex].Linear * dist + Light[lightIndex].Quadratic * (dist * dist));


		return Light[lightIndex].Intensity * (ambient + diffuse + specular * 5) * attenuation;

}

//________________________________________________________________________________________________________________________________________//

/*subroutine void RenderPassType();
subroutine uniform RenderPassType RenderPass;
subroutine (RenderPassType)
void shadeWithShadow()
{
 //   vec3 ambient = Light.La * Material.Ka;
		vec3 diffAndSpec = diffAndSpecShading();
		//Do shadow map lookup
		float shadow = float(textureProj(ShadowMap, ShadowCoord));
		//if the fragment is in shadow, use ambient only
		FragColour = vec4(diffAndSpec * shadow + ambient, 1.0);
}*/



//________________________________________________________________________________________________________________________________________//


/*subroutine (RenderPassType)
void recordDepth()
{
}*/




//________________________________________________________________________________________________________________________________________//

//code taken from https://www.shadertoy.com/view/Msf3WH
vec2 hash( vec2 p ) // replace this by something better
{
	      p = vec2( dot(p,vec2(127.1,311.7)),
				                  dot(p,vec2(269.5,183.3)) );

				return -1.0 + 2.0*fract(sin(p)*43758.5453123);
}

float noise( in vec2 p )
{
	  const float K1 = 0.366025404; // (sqrt(3)-1)/2;
		const float K2 = 0.211324865; // (3-sqrt(3))/6;

		    vec2 i = floor( p + (p.x+p.y)*K1 );

				vec2 a = p - i + (i.x+i.y)*K2;
				vec2 o = step(a.yx,a.xy);
				vec2 b = a - o + K2;
				vec2 c = a - 1.0 + 2.0*K2;

				vec3 h = max( 0.5-vec3(dot(a,a), dot(b,b), dot(c,c) ), 0.0 );

				vec3 n = h*h*h*h*vec3( dot(a,hash(i+0.0)), dot(b,hash(i+o)), dot(c,hash(i+1.0)));

				return dot( n, vec3(70.0) );

}

//end of code taken from https://www.shadertoy.com/view/Msf3WH


//________________________________________________________________________________________________________________________________________//


//________________________________________________________________________________________________________________________________________//


void main () {

	//  RenderPass();

	  // Calculate the normal (this is the expensive bit in Phong)
	  vec3 n = FragmentNormal;

		// Calculate the eye vector
		vec3 v = normalize(vec3(-FragmentPosition));



//    vec3 VP = LightPosition - FragmentPosition;
//    VP = normalize(VP);

		vec3 lookup;

		if (isReflect) {
			  lookup = reflect(v,n);
		} else {
			  lookup = refract(v,n,refractiveIndex);
		}


		// Extract the normal from the normal map (rescale to [-1,1]
//   vec3 tgt = normalize(texture(bumpMap, FragmentTexCoord).rgb * 2.0 - 1.0);

		// The source is just up in the Z-direction
//    vec3 src = vec3(0.0, 0.0, 1.0);

		// Perturb the normal according to the target
//    vec3 np = rotateVector(src, tgt, n);

//    vec3 s;
//    if(Light.Position.w == 0.0)
		  //  s = normalize (vec3(Light.Position));
//       s = normalize (LightPosition);
//    else
		  //  s = normalize( vec3(Light.Position) - FragmentPosition );
//        s = normalize( LightPosition - FragmentPosition );

		//  diffAndSpecShading();

		// Reflect the light about the surface normal
		//vec3 r = reflect( -s, np );
//    vec3 h = normalize(v + s);


/*    float NdotH = dot(n,h);
		float NdotV = dot(n,v);
		float NdotS = dot(n,s);
	 // float VdotH = dot(v,h);
		float k = pow((Material.Roughness + 1),2)/8;
 //   float Fo = pow(1- VdotH, 5);
		float D = MicrofacetDistribution(Material.Roughness, NdotH);
		float nvGA = MicrofacetGA(NdotV, k);
		float nsGA = MicrofacetGA(NdotS, k);
		float G = nvGA * nsGA;
	 // float F = MicrofacetFresnel(Fo, VdotH);*/

		// Compute the light from the ambient, diffuse and specular components
//    vec3 lightColour = (
//            Light.Intensity * (
//            Light.La * Material.Ka +
//            Light.Ld * Material.Kd * max( dot(s, n), 0.0 ) +
//            Light.Ls * Material.Ks * pow( max( dot(h, n), 0.0 ), Material.Shininess )));
		       // (Light.Ls * Material.Ks  * F) / NdotV));

		// The mipmap level is determined by log_2(resolution), so if the texture was 4x4,
		// there would be 8 mipmap levels (128x128,64x64,32x32,16x16,8x8,4x4,2x2,1x1).
		// The LOD parameter can be anything inbetween 0.0 and 8.0 for the purposes of
		// trilinear interpolation.

		// This call actually finds out the current LOD
		float lod = textureQueryLod(envMap, lookup).x;

		// Determine the gloss value from our input texture, and scale it by our LOD resolution
		float gloss = (1 - texture(glossMap, FragmentTexCoord*2).r) * float(envMaxLOD);

		// This call determines the current LOD value for the texture map
		vec4 colour = textureLod(envMap, lookup, gloss*0.01);

		// This call just retrieves whatever you want from the environment map
 //   vec4 colour = texture(envMap, lookup);
		//float shadeFactor = textureProj(ShadowMap, ShadowCoord).x;


		//  vec3 colour = vec3(0.0);
//   for(int i = 0; i < 5; ++i)
//        colour += ADS(i, FragmentPosition, FragmentNormal);

		//  FragColour = vec4(colour, 1.0) * texture(labelMap, FragmentTexCoord);

		float gamma = 1.2;



		// lightIntensity = BlinnPhong(0, n, v);

//code taken from https://www.shadertoy.com/view/Msf3WH
		vec2 p = gl_FragCoord.xy / iResolution.xy;

		    vec2 uv = p*(iResolution.x/iResolution.y);

				float f = 0.0;

				// left: value noise
				if( p.x<0.6 )
				{
					      f = noise( 16.0*uv );
				}
				// right: fractal noise (4 octaves)
				else
				{
					      uv *= 5.0;
								mat2 m = mat2( 1.6,  1.2, -1.2,  1.6 );
								f  = 0.5000*noise( uv ); uv = m*uv;
								f += 0.2500*noise( uv ); uv = m*uv;
								f += 0.1250*noise( uv ); uv = m*uv;
								f += 0.0625*noise( uv ); uv = m*uv;
				}

				f = 0.5 + 0.5*f;

				f *= smoothstep( 0.0, 0.005, abs(p.x-0.6) );

				vec4 noise = vec4( f, f, f, 1.0 );
//end of code taken from https://www.shadertoy.com/view/Msf3WH
				vec4 texturedCan = texture(labelMap, FragmentTexCoord) ;

/*    float NdotH = dot(n,h);
		float NdotV = dot(n,v);
		float NdotS = dot(n,s);
	 // float VdotH = dot(v,h);
		float k = pow((Material.Roughness + 1),2)/8;
 //   float Fo = pow(1- VdotH, 5);
		float D = MicrofacetDistribution(Material.Roughness, NdotH);
		float nvGA = MicrofacetGA(NdotV, k);
		float nsGA = MicrofacetGA(NdotS, k);
		float G = nvGA * nsGA;
		float FresnelTerm = Fbias +Fscale * pow(1.0 + dot(v, n), Fpower);
		vec4 F = mix(texturedCan, colour, FresnelTerm);
		float DGF = D * G * F.x;*/

				vec3 lightIntensity = vec3(0.0);
				for(int i = 0; i<3; ++i)
				{
					//   lightIntensity += Microfacet(i, n, v, texturedCan, colour);
					lightIntensity += BlinnPhong(i, n, v);
				}



				FragColour = texturedCan * vec4(lightIntensity,1.0) * colour;// * roughness;// * shadeFactor;*/
				// FragColour = vec4(lightIntensity,1.0);
				FragColour.rgb = pow(FragColour.rgb, vec3(1.0/gamma));
 //   FragColour = noise;
				//  FragColour *= roughness;
				// FragColour = vec4(lightColour,1.0) * colour;
				//  FragColour = colour;
 //   FragColour = vec4(lightIntensity,1.0);
				// FragColour = vec4(0.2,0.2,0.7,1.0);
}
