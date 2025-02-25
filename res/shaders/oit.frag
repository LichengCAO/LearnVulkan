#version 450

// Dimensionality: uimageBuffer is 1D, while uimage2D is 2D.
// Use Cases: Choose based on whether your application requires handling data with a natural 2D layout (uimage2D) or a linear 1D layout (uimageBuffer).
#define OIT_LAYERS 5
layout(set = 0, binding = 1) uniform sampler2D uSampler;
layout(set = 1, binding = 0, r32ui) uniform coherent uimageBuffer imgAbuffer;
layout(set = 1, binding = 1, r32ui) uniform coherent uimage2D imgAux;
layout(set = 1, binding = 2, r32ui) uniform coherent uimage2D imgSpin;
layout(set = 1, binding = 3, r32ui) uniform coherent uimage2D imgDepth;
layout(set = 1, binding = 4) uniform ivec3 viewport; // width, height, width * height 
layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;
ivec2 coord = ivec2(gl_FragCoord.xy);
void main()
{
    vec4 storeValue = vec4(1.f, 1.f, 1.f, 1.f);
    const uint abufferCoord = viewport.x * gl_FragCoord.y + viewport.x;
    bool done = false;
    while(!done)
    {
        uint old = imageAtomicExchange(imgSpin, gl_FragCoord.xy, 1u);
        if (old == 0u)
        {
            const uint oldCount = imageLoad(imgAux, gl_FragCoord.xy).r;
            const uint oitCount = min(oldCount + 1, OIT_LAYERS);

            imageStore(imgAux, gl_FragCoord.xy, uvec4(oldCount + 1));

            // leave bubble sort at shading pass
            if(oldCounter < OIT_LAYERS)
            {
                imageStore(imgAbuffer, abufferCoord + oldCounter * viewport.z, storeValue);
            }
            else
            {
                // Find the furthest element
                int  furthest = -1;
                vec4 sample = storeValue;
                int  furthest2 = 0;
                vec4 sample2 = vec4(0.f, 0.f, 0.f, 0.f);
                for(int i = 0; i < OIT_LAYERS; i++)
                {
                    vec4 newSample = imageLoad(imgAbuffer, abufferCoord + i * viewport.z);
                    uint testDepth = newSample.g;
                    uint maxDepth = sample.g;
                    uint maxDepth2 = sample2.g;
                    if(testDepth > maxDepth)
                    {
                        sample2 = sample;
                        furthest2 = furthest;
                        sample = newSample;
                        furthest = i;
                    }
                    else if (testDepth > maxDepth2)
                    {       
                        sample2 = newSample;
                        furthest2 = i;
                    }
                }
                //TODO: mix sample and sample2 to storeValue
                if (furthest == -1)
                {
                    imageStore(imgAbuffer, listPos + furthest2 * viewport.z, storeValue);
                }
                else
                {
                    imageStore(imgAbuffer, listPos + furthest * viewport.z, storeValue);
                }

            }
            imageAtomicExchange(imgSpin, gl_FragCoord.xy, 0u);
            done = true;
        }
    }
    outColor = texture(uSampler, inUV);
}