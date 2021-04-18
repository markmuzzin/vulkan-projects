#version 450

layout (location=0) in vec4 iPosition;
layout (location=1) in vec4 iNormal;
layout (location=2) in vec2 iTexCoord;

layout (binding=0) uniform sampler samp;
layout (binding=1) uniform texture2D textures[16];

layout (location=0) out vec4 oColor;

layout(push_constant) uniform constants
{
    uint imageIndex;
    float Ns;
    float sKa[3];
    float sKd[3];
    float sKs[3];
    float Ni;
    float d;
    float illum;
    int shininess;
    float slightPosition[3];
    float sCameraTarget[3];
    float sAmbientLight[3];
    float sLightSourceIntensity[3];
} pc;


void main(void)
{
    /* Can't seem to pass in an array of floats[3] as a vec3 in the shader */
    vec3 lightPosition = vec3(pc.slightPosition[0], pc.slightPosition[1], pc.slightPosition[2]);
    vec3 cameraTarget = vec3(pc.sCameraTarget[0], pc.sCameraTarget[1], pc.sCameraTarget[2]);
    vec3 Ka = clamp(vec3(pc.sKa[0], pc.sKa[1], pc.sKa[2]), -1.0, 1.0);
    vec3 Ks = clamp(vec3(pc.sKs[0], pc.sKs[1], pc.sKs[2]), -1.0, 1.0);
    vec3 Kd = clamp(vec3(pc.sKd[0], pc.sKd[1], pc.sKd[2]), -1.0, 1.0);

    vec3 ambientLight = clamp(vec3(pc.sAmbientLight[0], pc.sAmbientLight[1], pc.sAmbientLight[2]), -1.0, 1.0);
    vec3 lightSourceIntensity = clamp(vec3(pc.sLightSourceIntensity[0], pc.sLightSourceIntensity[1], pc.sLightSourceIntensity[2]), -1.0, 1.0);

    float d = clamp(pc.d, -1.0, 1.0);

    vec3 normal = vec3(normalize(iNormal.xyz));

    vec3 lightDirection = normalize(lightPosition - iPosition.xyz);
    vec3 viewDirection = normalize(cameraTarget - iPosition.xyz);
    vec3 reflectDirection = reflect((-1.0 *lightDirection), normal);

    vec3 diffuse = (Ka + lightSourceIntensity) * Kd * max(dot(normal, lightDirection), 0.0);
    vec3 specular = Ks * pow(max(dot(viewDirection, reflectDirection), 0.0), pc.shininess);


    oColor = texture(sampler2D(textures[pc.imageIndex],samp), iTexCoord) * vec4(specular + diffuse + ambientLight, d);
}
