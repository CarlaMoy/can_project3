/*#version 430

// The texture to be mapped
uniform sampler2D colourTex;
uniform sampler2D depthTex;


// The depth at which we want to focus
uniform float focalDepth = 0.5;

// A scale factor for the radius of blur
uniform float blurRadius = 0.008;

// The output colour. At location 0 it will be sent to the screen.
layout (location=0) out vec4 fragColour;

// We pass the window size to the shader.
uniform vec2 windowSize;




// Gaussian coefficients
const float G5x5[25] = {0.0035,    0.0123,    0.0210,    0.0123,    0.0035,
                        0.0123,    0.0543,    0.0911,    0.0543,    0.0123,
                        0.0210,    0.0911,    0.2224,    0.0911,    0.0210,
                        0.0123,    0.0543,    0.0911,    0.0543,    0.0123,
                        0.0035,    0.0123,    0.0210,    0.0123,    0.0035};

const float G9x9[81] = {0,         0,    0.0039,    0.0039,    0.0039,    0.0039,    0.0039,         0,         0,
                        0,    0.0039,    0.0078,    0.0117,    0.0117,    0.0117,    0.0078,    0.0039,         0,
                        0.0039,    0.0078,    0.0117,    0.0234,    0.0273,    0.0234,    0.0117,    0.0078,    0.0039,
                        0.0039,    0.0117,    0.0234,    0.0352,    0.0430,    0.0352,    0.0234,    0.0117,    0.0039,
                        0.0039,    0.0117,    0.0273,    0.0430,    0.0469,    0.0430,    0.0273,    0.0117,    0.0039,
                        0.0039,    0.0117,    0.0234,    0.0352,    0.0430,    0.0352,    0.0234,    0.0117,    0.0039,
                        0.0039,    0.0078,    0.0117,    0.0234,    0.0273,    0.0234,    0.0117,    0.0078,    0.0039,
                        0,    0.0039,    0.0078,    0.0117,    0.0117,    0.0117,    0.0078,    0.0039,         0,
                        0,         0,    0.0039,    0.0039,    0.0039,    0.0039,    0.0039,         0,         0};

// These define which Gaussian kernel and the size to use (G5x5 and 5 also possible)
#define SZ 9
#define G  G9x9

// Gaussian filter for regular smooth blur
vec4 GaussianBlur(vec2 texpos, float sigma) {
    // We need to know the size of half the window
    int HSZ = int(floor(SZ / 2));

    int i,j;
    vec4 colour = vec4(0.0);
    vec2 samplepos;

    // Note that this loops over n^2 values. Is there a more efficient way to do this?
    for (i=0; i<SZ; ++i) {
        for (j=0; j<SZ; ++j) {
            samplepos = texpos + sigma*vec2(float(i)-HSZ, float(j)-HSZ);
            colour += G[i*SZ+j] * texture(colourTex, samplepos);
        }
    }
    return colour;
}



void main() {
    // Determine the texture coordinate from the window size
    vec2 texpos = gl_FragCoord.xy / windowSize;

    // Determine sigma, the blur radius of this pixel
    float sigma = abs(focalDepth - texture(depthTex, texpos).x) * blurRadius;

    // Now execute the use specified blur function on this pixel based on the depth difference
		fragColour = GaussianBlur(texpos, sigma);// * texture(colourTex, texpos);
		fragColour = texture(colourTex,texpos);
}*/

#version 420 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D image;
uniform bool horizontal;

uniform float weight[5] = float[] (0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162);

void main()
{
     vec2 tex_offset = 1.0 / textureSize(image, 0); // gets size of single texel
     vec3 result = texture(image, TexCoords).rgb * weight[0];
     if(horizontal)
     {
         for(int i = 1; i < 5; ++i)
         {
            result += texture(image, TexCoords + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(image, TexCoords - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
         }
     }
     else
     {
         for(int i = 1; i < 5; ++i)
         {
             result += texture(image, TexCoords + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
             result += texture(image, TexCoords - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
         }
     }
     FragColor = vec4(result, 1.0);
}

