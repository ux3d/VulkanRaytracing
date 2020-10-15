#version 460 core

layout(push_constant) uniform UniformPushConstant {
    mat4 projection;
    mat4 view;
    mat4 world;

    uint attributeCount;

    uint targetsCount;
} in_upc;

layout (location = POSITION_LOC) in vec3 in_position;
#ifdef NORMAL_VEC3
layout (location = NORMAL_LOC) in vec3 in_normal;
#endif
#ifdef TANGENT_VEC4
layout (location = TANGENT_LOC) in vec4 in_tangent;
#endif
#ifdef TEXCOORD_0_VEC2
layout (location = TEXCOORD_0_LOC) in vec2 in_texCoord0;
#endif
#ifdef TEXCOORD_1_VEC2
layout (location = TEXCOORD_1_LOC) in vec2 in_texCoord1;
#endif
#ifdef COLOR_0_VEC4
layout (location = COLOR_0_LOC) in vec4 in_color;
#endif
#ifdef COLOR_0_VEC3
layout (location = COLOR_0_LOC) in vec3 in_color;
#endif
#ifdef JOINTS_0_VEC4
layout (location = JOINTS_0_LOC) in vec4 in_joints0;
#endif
#ifdef JOINTS_1_VEC4
layout (location = JOINTS_1_LOC) in vec4 in_joints1;
#endif
#ifdef WEIGHTS_0_VEC4
layout (location = WEIGHTS_0_LOC) in vec4 in_weights0;
#endif
#ifdef WEIGHTS_1_VEC4
layout (location = WEIGHTS_1_LOC) in vec4 in_weights1;
#endif

layout (location = 0) out vec3 out_position;
layout (location = 1) out vec3 out_view;
#ifdef NORMAL_VEC3
layout (location = 2) out vec3 out_normal;
#endif
#ifdef TANGENT_VEC4
layout (location = 3) out vec3 out_tangent;
layout (location = 4) out vec3 out_bitangent;
#endif
#ifdef TEXCOORD_0_VEC2
layout (location = 5) out vec2 out_texCoord0;
#endif
#ifdef TEXCOORD_1_VEC2
layout (location = 6) out vec2 out_texCoord1;
#endif
#ifdef COLOR_0_VEC4
layout (location = 7) out vec4 out_color;
#endif
#ifdef COLOR_0_VEC3
layout (location = 7) out vec4 out_color;
#endif

#ifdef HAS_TARGET_POSITION
layout (binding = TARGET_POSITION_BINDING) buffer Position {
    float i[];
} u_targetPosition;
#endif
#ifdef HAS_TARGET_NORMAL
layout (binding = TARGET_NORMAL_BINDING) buffer Normal {
    float i[];
} u_targetNormal;
#endif
#ifdef HAS_TARGET_TANGENT
layout (binding = TARGET_TANGENT_BINDING) buffer Tangent {
    float i[];
} u_targetTangent;
#endif

#ifdef HAS_WEIGHTS
layout (binding = WEIGHTS_BINDING) uniform Weights { 
    vec4 i[TARGETS_COUNT];
} u_weights;
#endif

layout (location = 8) flat out float out_determinant;

void main()
{
    mat3 normalMatrix = transpose(inverse(mat3(in_upc.world)));

#ifdef NORMAL_VEC3
    vec3 normal = in_normal;
#ifdef HAS_TARGET_NORMAL
    for (uint target = 0; target < in_upc.targetsCount; target++)
    {
        uint index = 3 * gl_VertexIndex + 3 * target * in_upc.attributeCount;
        normal += u_weights.i[target / 4][target % 4] * vec3(u_targetNormal.i[index + 0], u_targetNormal.i[index + 1], u_targetNormal.i[index + 2]);
    }
#endif
    out_normal = normalMatrix * normal;
#endif

#ifdef TANGENT_VEC4
    vec3 tangent = in_tangent.xyz;
#ifdef HAS_TARGET_TANGENT
    for (uint target = 0; target < in_upc.targetsCount; target++)
    {
        uint index = 3 * gl_VertexIndex + 3 * target * in_upc.attributeCount;
        tangent += u_weights.i[target / 4][target % 4] * vec3(u_targetTangent.i[index + 0], u_targetTangent.i[index + 1], u_targetTangent.i[index + 2]);
    }
#endif
    vec3 bitangent = cross(normal, tangent) * in_tangent.w;
    out_tangent = normalMatrix * tangent;
    out_bitangent = normalMatrix * bitangent;
#endif

#ifdef TEXCOORD_0_VEC2
    out_texCoord0 = in_texCoord0;
#endif
#ifdef TEXCOORD_1_VEC2
    out_texCoord1 = in_texCoord1;
#endif

#ifdef COLOR_0_VEC4
    out_color = in_color;
#endif
#ifdef COLOR_0_VEC3
    out_color = vec4(in_color, 1.0);
#endif

    vec3 tempPosition = in_position;
#ifdef HAS_TARGET_POSITION
    for (uint target = 0; target < in_upc.targetsCount; target++)
    {
        uint index = 3 * gl_VertexIndex + 3 * target * in_upc.attributeCount;

        tempPosition += u_weights.i[target / 4][target % 4] * vec3(u_targetPosition.i[index + 0], u_targetPosition.i[index + 1], u_targetPosition.i[index + 2]);
    }
#endif
    vec4 position = vec4(tempPosition, 1.0);
    position = in_upc.world * position;
    out_position = position.xyz / position.w;

    out_determinant = determinant(in_upc.world);
    
    out_view = inverse(mat3(in_upc.view)) * vec3(0.0, 0.0, 1.0);

    gl_Position = in_upc.projection * in_upc.view * position;
}
