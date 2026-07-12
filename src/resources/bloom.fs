#version 100

precision mediump float;

// Input vertex attributes (from vertex shader)
varying vec2 fragTexCoord;
varying vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// NOTE: Add your custom variables here

const vec2 size = vec2(720, 720);   // Framebuffer size
const float samples = 4.0;          // Pixels per axis; higher = bigger glow, worse performance
const float quality = 1.0;          // Defines size factor: Lower = smaller glow, better quality

void main()
{
    vec4 sum = vec4(0);
    vec2 sizeFactor = vec2(1)/size*quality;

    // Texel color fetching from texture sampler
    vec4 source = texture2D(texture0, fragTexCoord);

    const int range = 2;            // should be = (samples - 1)/2;

    for (int x = -range; x <= range; x++)
    {
        for (int y = -range; y <= range; y++)
        {
            sum += texture2D(texture0, fragTexCoord + vec2(x, y)*sizeFactor);
        }
    }
    // Calculate final fragment color
    vec4 finalColor = ((sum/(samples*samples))*0.4 + source)*colDiffuse;

    // Push colors away from gray to boost saturation
    float gray = dot(finalColor.rgb, vec3(0.299, 0.587, 0.114));
    gl_FragColor = vec4(mix(vec3(gray), finalColor.rgb, 1.4), finalColor.a);
}