#ifndef __OBJ_FILE_LOADER_H__
#define __OBJ_FILE_LOADER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <vulkan/vulkan.h>
#include <errno.h>
#include <matrixMath.h>

#define STRLEN                      128

#define MAX_MATERIALS               32
#define MAX_MATERIAL_CHANGES        300

#define ELEMENTS_PER_FACE           3
#define ELEMENTS_PER_VERTEX         3
#define ELEMENTS_PER_TEXCOORDS      2

typedef struct materialProperties_t
{
    uint32_t imageIndex;
    float Ns;
    vec3_t Ka;
    vec3_t Kd;
    vec3_t Ks;
    float Ni;
    float d;
    float illum;
    int shininess;
} materialProperties_t;

typedef struct sceneProperties_t
{
    vec3_t lightPosition;
    vec3_t cameraTarget;
    vec3_t ambientLight;
    vec3_t lightSourceIntensity;
} sceneProperties_t;

typedef struct pushConstants_t
{
    materialProperties_t mp;
    sceneProperties_t sp;
} pushConstants_t;

typedef struct material_t
{
    char *name;
    char *fileName;
    materialProperties_t mp;
} material_t;


typedef struct material_change_t
{
    uint32_t startFace;
    material_t *material;
} material_change_t;


typedef struct model_t
{
    float *vertArray;
    float *texArray;
    float *normArray;

    float *v;
    uint32_t numOfVertices;

    float *vt;
    uint32_t numOfTexCoords;

    float *vn;
    uint32_t numOfNormals;

    uint32_t *f;
    uint32_t numOfFaces;

    char *materialLibFilename;
    uint32_t materialCount;
    material_change_t materialChange[MAX_MATERIAL_CHANGES];
    uint32_t materialChangeCount;

    float modelRotationUp;
    float modelRotationRight;

    vec3_t cameraRight;
    vec3_t cameraDirection;
    vec3_t cameraTarget;

    vec3_t cameraPosition;
    vec3_t cameraFront;
    vec3_t cameraUp;

    sceneProperties_t sp;
} model_t;


VkBool32 loadModel(model_t *object, material_t *materials, char *objFileName);
void prepareObjectArrays(model_t *object);
char* getPath(char *string);

#endif
