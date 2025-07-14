#version 460 core
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require
// https://stackoverflow.com/questions/60549218/what-use-has-the-layout-specifier-scalar-in-ext-scalar-block-layout
#extension GL_EXT_scalar_block_layout : enable

#include "rt_common.glsl"

// Note that there is no requirement that the location of the callee's incoming payload match 
// the payload argument the caller passed to traceRayEXT! 
// This is quite unlike the in/out variables used to connect vertex shaders and fragment shaders.
layout(location = 1) rayPayloadInEXT HitPayload payload;

void main()
{
    payload.hitValue = vec3(1.0f, 0.0f, 0.0f); // If the shadow ray hit an object, set the hit value to black
}