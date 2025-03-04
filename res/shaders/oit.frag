#version 450

// Dimensionality: uimageBuffer is 1D, while uimage2D is 2D.
// Use Cases: Choose based on whether your application requires handling data with a natural 2D layout (uimage2D) or a linear 1D layout (uimageBuffer).
#define OIT_LAYERS 5

layout(location = 0) in vec4 inColor;
layout(location = 1) in float vDepth;

layout(set = 2, binding = 0, rgba32ui) uniform coherent uimageBuffer sampleDataImage; // sample data
layout(set = 2, binding = 1, r32ui) uniform coherent uimage2D sampleCountImage; // sample count
layout(set = 2, binding = 2, r32ui) uniform coherent uimage2D inUseImage; // flow control
layout(set = 2, binding = 3) uniform ViewportInformation
{
    ivec4 extent; // width, height, width * height
} viewportInfo;
layout(set = 3, binding = 4) uniform sampler2D texDepth; // TODO:

layout(location = 0) out vec4 outColor;

void main()
{
    // packSnorm4x8: round(clamp(c, -1.0, 1.0) * 127.0)
    ivec2 coord = ivec2(gl_FragCoord.xy);
    uvec4 storeValue = uvec4(packUnorm4x8(inColor), floatBitsToUint(vDepth), 0, 0);
    const int texelBufferCoord = viewportInfo.extent.x * OIT_LAYERS * coord.y + OIT_LAYERS * coord.x;
    bool done = gl_SampleMaskIn[0] == 0; // if simply set to false, won't work, haven't figure out yet
    bool passDepthTest = vDepth < texelFetch(texDepth, coord, 0 ).r;
    while(passDepthTest && !done)
    // int  spin = 10000; // spin counter as safety measure, not required
    // while(!done && spin-- > 0)
    {
        uint old = imageAtomicCompSwap(inUseImage, coord, 0u, 1u);
        if (old == 0u)
        {
            const uint oldCount = imageLoad(sampleCountImage, coord).r;
            imageStore(sampleCountImage, coord, uvec4(oldCount + 1));

            // leave bubble sort at shading pass
            if(oldCount < OIT_LAYERS)
            {
                imageStore(sampleDataImage, texelBufferCoord + int(oldCount), storeValue);
            }
            else
            {
                // Find the furthest element
                int  furthest = -1;
                uvec4 sample1 = storeValue;
                int  furthest2 = 0;
                uvec4 sample2 = uvec4(0, 0, 0, 0);
                for(int i = 0; i < OIT_LAYERS; ++i)
                {
                    uvec4 newSample = imageLoad(sampleDataImage, texelBufferCoord + i);
                    float testDepth = uintBitsToFloat(newSample.g);
                    float maxDepth  = uintBitsToFloat(sample1.g);
                    float maxDepth2 = uintBitsToFloat(sample2.g);
                    if(testDepth > maxDepth)
                    {
                        sample2 = sample1;
                        furthest2 = furthest;
                        sample1 = newSample;
                        furthest = i;
                    }
                    else if (testDepth > maxDepth2)
                    {       
                        sample2 = newSample;
                        furthest2 = i;
                    }
                }
                storeValue.g = sample2.g;
                vec4 farColor = unpackUnorm4x8(sample1.r);
                vec4 nearColor = unpackUnorm4x8(sample2.r);
                nearColor.rgb = nearColor.rgb + nearColor.a * farColor.rgb;
                nearColor.a = clamp(nearColor.a * farColor.a, 0.0f, 1.0f);
                storeValue.r = packUnorm4x8(nearColor);
                if (furthest == -1)
                {
                    imageStore(sampleDataImage, texelBufferCoord + furthest2, storeValue);
                }
                else
                {
                    imageStore(sampleDataImage, texelBufferCoord + furthest, storeValue);
                }
            }
            imageAtomicExchange(inUseImage, coord, 0u);
            done = true;
        }
    }
    outColor = inColor;
}