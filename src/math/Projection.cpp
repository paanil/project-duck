
#include "Projection.h"
#include "Constants.h"
#include <cmath>

namespace rob
{

    mat4f Projection_Perspective_lh(float fov_y, float aspect_ratio,
                                    float near_z, float far_z)
    {
        const float tan_half_fov_y = std::tan(fov_y * 0.5f * DEG2RAD_f);
        const float f = 1.0f / tan_half_fov_y;
//        const float df = 1.0f / (near_z - far_z);
        const float df = 1.0f / (far_z - near_z);

        const float A = f / aspect_ratio;
        const float B = (far_z + near_z) * df;
        const float C = (-2.0f * far_z * near_z) * df;

        return mat4f(
             A,   0.0f,  0.0f,  0.0f,
            0.0f,  f,    0.0f,  0.0f,
            0.0f, 0.0f,   B,    C,
            0.0f, 0.0f, 1.0f,  0.0f
        );
    }


    mat4f Projection_Perspective_rh(float fov_y, float aspect_ratio,
                                    float near_z, float far_z)
    {
        const float tan_half_fov_y = std::tan(fov_y * 0.5f * DEG2RAD_f);
        const float f = 1.0f / tan_half_fov_y;
        const float df = 1.0f / (near_z - far_z);

        const float A = f / aspect_ratio;
        const float B = (far_z + near_z) * df;
        const float C = (2.0f * far_z * near_z) * df;

        return mat4f(
             A,   0.0f,  0.0f,  0.0f,
            0.0f,  f,    0.0f,  0.0f,
            0.0f, 0.0f,   B,     C,
            0.0f, 0.0f, -1.0f,  0.0f
        );
    }


    mat4f Projection_Orthogonal_lh(float left, float right,
                                   float bottom, float top,
                                   float near, float far)
    {
        const float w = right - left;
        const float h = top - bottom;
        const float d = far - near;

        const float tx = (right + left) / -w;
        const float ty = (top + bottom) / -h;
        const float tz = near / -d;

//        2/(r-l)      0            0           0
//        0            2/(t-b)      0           0
//        0            0            1/(zf-zn)   0
//        (l+r)/(l-r)  (t+b)/(b-t)  zn/(zn-zf)  1

        return mat4f(
            2.0f / w, 0.0f,     0.0f,     tx,
            0.0f,     2.0f / h, 0.0f,     ty,
            0.0f,     0.0f,     2.0f / d, tz,
            0.0f,     0.0f,     0.0f,     1.0f
        );
    }


    mat4f Projection_Orthogonal_rh(float left, float right,
                                   float bottom, float top,
                                   float near, float far)
    {
        const float w = right - left;
        const float h = top - bottom;
        const float d = far - near;

        const float tx = -(right + left) / w;
        const float ty = -(top + bottom) / h;
        const float tz = -(far + near) / d;

        return mat4f(
            2.0f / w, 0.0f,     0.0f,     tx,
            0.0f,     2.0f / h, 0.0f,     ty,
            0.0f,     0.0f,    -2.0f / d, tz,
            0.0f,     0.0f,     0.0f,     1.0f
        );
    }

} // rob
