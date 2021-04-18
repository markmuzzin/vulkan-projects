#version 450
layout(location=0) in vec4 aPosition;
layout(location=1) in vec4 aNormal;
layout(location=2) in vec2 aTexCoord;

layout (binding=2) uniform MVP
{
    mat4 persepctiveProjMatrix;
    mat4 rotationMatrixUp;
    mat4 rotationMatrixRight;
    mat4 viewMatrix;
};

layout(location=0) out vec4 oPosition;
layout(location=1) out vec4 oNormal;
layout(location=2) out vec2 oTexCoord;

void main(void)
{
    gl_Position = aPosition * (rotationMatrixUp * rotationMatrixRight * viewMatrix * persepctiveProjMatrix);
    oNormal = aNormal;
    oTexCoord = aTexCoord;
    oPosition = aPosition;
}
