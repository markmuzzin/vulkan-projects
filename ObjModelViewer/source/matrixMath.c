#include "matrixMath.h"

vec3_t negate(vec3_t v)
{
    return (vec3_t) {-1.0f*v.x, -1.0f*v.y, -1.0f*v.z};
}


vec3_t normalize(vec3_t v)
{
   float length_of_v = sqrtf((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
   if (length_of_v != 0.0f)
       return (vec3_t){v.x / length_of_v, v.y / length_of_v, v.z / length_of_v};
   else
       return (vec3_t){0.0f, 0.0f, 0.0f};
}


vec3_t crossProd(vec3_t v, vec3_t u)
{
    return (vec3_t) { (v.y*u.z) - (v.z*u.y),  ((v.z*u.x) - (v.x*u.z)), (v.x*u.y) - (v.y*u.x) };
}


vec3_t scalarProd(vec3_t v, float u)
{
    return (vec3_t) {(v.x * u), (v.y * u), (v.z * u) };
}


float dotProd(vec3_t v, vec3_t u)
{
    return (v.x * u.x) + (v.y * u.y) + (v.z * u.z);
}


vec3_t subProd(vec3_t v, vec3_t u)
{
    return (vec3_t) { (v.x-u.x), (v.y-u.y), (v.z-u.z) };
}


vec3_t addProd(vec3_t v, vec3_t u)
{
    return (vec3_t) { (v.x+u.x), (v.y+u.y), (v.z+u.z) };
}


void matrix4x4By4x4(float *src1, float *src2, float *dest)
{
    int32_t i;
    float tmp[16];

    for (i = 0; i < 4; i++)
    {
        tmp[i*4]   = (src1[i*4] * src2[0]) + (src1[i*4+1] * src2[4]) + (src1[i*4+2] * src2[8])  + (src1[i*4+3] * src2[12]);
        tmp[i*4+1] = (src1[i*4] * src2[1]) + (src1[i*4+1] * src2[5]) + (src1[i*4+2] * src2[9])  + (src1[i*4+3] * src2[13]);
        tmp[i*4+2] = (src1[i*4] * src2[2]) + (src1[i*4+1] * src2[6]) + (src1[i*4+2] * src2[10]) + (src1[i*4+3] * src2[14]);
        tmp[i*4+3] = (src1[i*4] * src2[3]) + (src1[i*4+1] * src2[7]) + (src1[i*4+2] * src2[11]) + (src1[i*4+3] * src2[15]);
    }

    for (i = 0; i < 16; i++)
    {
        dest[i] = tmp[i];
    }
}


void matrix4x4By4x1(float *src1, float *src2, float *dest)
{
    float tmp[4];

    tmp[0] = (src2[0] * src1[0]) + (src2[1] * src1[4]) + (src2[2] * src1[8])  + (src2[3] * src1[12]);
    tmp[1] = (src2[0] * src1[1]) + (src2[1] * src1[5]) + (src2[2] * src1[9])  + (src2[3] * src1[13]);
    tmp[2] = (src2[0] * src1[2]) + (src2[1] * src1[6]) + (src2[2] * src1[10]) + (src2[3] * src1[14]);
    tmp[3] = (src2[0] * src1[3]) + (src2[1] * src1[7]) + (src2[2] * src1[11]) + (src2[3] * src1[15]);

    *(dest+0) = tmp[0];
    *(dest+1) = tmp[1];
    *(dest+2) = tmp[2];
    *(dest+3) = tmp[3];
}


void generateRotationMatrix(float angle, vec3_t axis, float *mat)
{
    float c, s, d;
    float x, y, z;

    x = axis.x;
    y = axis.y;
    z = axis.z;

    angle = angle * (float)PI / 180.0f;
    c = (float)cosf(angle);
    s = (float)sinf(angle);
    d = 1.0f - c;

    mat[0] = x*x*d + c;   mat[4] = x*y*d - z*s; mat[8] = x*z*d + y*s; mat[12] = 0.0f;
    mat[1] = y*x*d + z*s; mat[5] = y*y*d + c;   mat[9] = y*z*d - x*s; mat[13] = 0.0f;
    mat[2] = x*z*d - y*s; mat[6] = y*z*d + x*s; mat[10] = z*z*d + c;  mat[14] = 0.0f;
    mat[3] = 0.0f;        mat[7] = 0.0f;        mat[11] = 0.0f;       mat[15] = 1.0f;
}


void generateScaleTranslationMatrix(vec3_t scale, vec3_t translate, float *mat)
{
    float t_x,t_y,t_z;
    float s_x,s_y,s_z;

    t_x = translate.x;
    t_y = translate.y;
    t_z = translate.z;

    s_x = scale.x;
    s_y = scale.y;
    s_z = scale.z;

    mat[0] = s_x;   mat[4] = 0.0f;  mat[8] = 0.0f;   mat[12] = 0.0f;
    mat[1] = 0.0f;  mat[5] = s_y;   mat[9] = 0.0f;   mat[13] = 0.0f;
    mat[2] = 0.0f;  mat[6] = 0.0f;  mat[10] = s_z;   mat[14] = 0.0f;
    mat[3] = t_x;   mat[7] = t_y;   mat[11] = t_z;   mat[15] = 1.0f;
}


void setIdentityMatrix(float* mat)
{
    mat[0] = 1.0f;    mat[1] = 0.0f;    mat[2] = 0.0f;    mat[3] = 0.0f;
    mat[4] = 0.0f;    mat[5] = 1.0f;    mat[6] = 0.0f;    mat[7] = 0.0f;
    mat[8] = 0.0f;    mat[9] = 0.0f;    mat[10] = 1.0f;    mat[11] = 0.0f;
    mat[12] = 0.0f;   mat[13] = 0.0f;   mat[14] = 0.0f;    mat[15] = 1.0f;
}


void generateLookAtMatrix(vec3_t eye, vec3_t target, vec3_t upDir, float *mat)
{
    vec3_t forward = subProd(eye, target);
    forward = normalize(forward);

    vec3_t left = crossProd(upDir, forward);
    left = normalize(left);

    vec3_t up = crossProd(forward, left);

    mat[0] = left.x;    mat[1] = up.x;    mat[2] = forward.x;     mat[3] = -left.x * eye.x - left.y * eye.y - left.z * eye.z;
    mat[4] = left.y;    mat[5] = up.y;    mat[6] = forward.y;     mat[7] = -up.x * eye.x - up.y * eye.y - up.z * eye.z;
    mat[8] = left.z;    mat[9] = up.z;    mat[10] = forward.z;    mat[11] = -forward.x * eye.x - forward.y * eye.y - forward.z * eye.z;
    mat[12] = 0.0f;     mat[13] = 0.0f;   mat[14] = 0.0f;         mat[15] = 1.0f;
}


void generatePerspectiveProjectionMatrix(float fov, float aspect, float zNear, float zFar, float *mat)
{
    float f = 1.0f / tanf(fov * 0.5f * PI / 180.0f);

    float x = f / aspect;
    float y = -1.0f * f;
    float A = zNear / (zFar - zNear);
    float B = (zFar * zNear) / (zFar - zNear);

    mat[0] = x;      mat[4] = 0.0f;    mat[8] = 0.0f;    mat[12] = 0.0f;
    mat[1] = 0.0f;   mat[5] = y;       mat[9] = 0.0f;    mat[13] = 0.0f;
    mat[2] = 0.0f;   mat[6] = 0.0f;    mat[10] = A;      mat[14] = -1.0f;
    mat[3] = 0.0f;   mat[7] = 0.0f;    mat[11] = B;      mat[15] = 0.0f;
}


void generateProjectionMatrix(float b, float l, float t, float r, float n, float f, float *mat)
{
    mat[0] = (2.0f*n)/(r-l);    mat[4] = 0.0f;              mat[8]  = (r+l)/(r-l);   mat[12] = 0.0f;
    mat[1] = 0.0f;              mat[5] = (2.0f*n)/(t-b);    mat[9]  = (t+b)/(t-b);   mat[13] = 0.0f;
    mat[2] = 0.0f;              mat[6] = 0.0f;              mat[10] = -(f+n)/(f-n);  mat[14] = -(2.0f*f*n)/(f-n);
    mat[3] = 0.0f;              mat[7] = 0.0f;              mat[11] = -1.0f;         mat[15] = 0.0f;
}
