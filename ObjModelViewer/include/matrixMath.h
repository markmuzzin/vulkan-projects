#ifndef __MATRIX_MATH_H__
#define __MATRIX_MATH_H__

#include <inttypes.h>
#include <math.h>
#include <stdio.h>

#define PI                          3.141592654f

typedef struct _vec3_t
{
    float x;
    float y;
    float z;
} vec3_t;


typedef struct _vec4_t
{
    float x;
    float y;
    float z;
    float w;
} vec4_t;


vec3_t negate(vec3_t v);
vec3_t normalize(vec3_t v);
vec3_t crossProd(vec3_t v, vec3_t u);
vec3_t scalarProd(vec3_t v, float u);
float dotProd(vec3_t v, vec3_t u);
vec3_t subProd(vec3_t v, vec3_t u);
vec3_t addProd(vec3_t v, vec3_t u);
void matrix4x4By4x4(float *src1, float *src2, float *dest);
void matrix4x4By4x1(float *src1, float *src2, float *dest);
void generateRotationMatrix(float angle, vec3_t axis, float *mat);
void generateScaleTranslationMatrix(vec3_t scale, vec3_t translate, float *mat);
void setIdentityMatrix(float* mat);
void generateLookAtMatrix(vec3_t eye, vec3_t target, vec3_t upDir, float *mat);
void generatePerspectiveProjectionMatrix(float fov, float aspect, float zNear, float zFar, float *mat);
void generateProjectionMatrix(float b, float l, float t, float r, float n, float f, float *mat);

#endif

