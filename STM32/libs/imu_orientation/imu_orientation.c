#include "imu_orientation.h"
#include <math.h>

#define DEG_TO_RAD_F 0.01745329251994329577f
#define RAD_TO_DEG_F 57.29577951308232088f
#define KP 1.5f

typedef struct {
    float w, x, y, z;
} Quaternion;

static Quaternion q = {1.0f, 0.0f, 0.0f, 0.0f};
static Quaternion q_zero = {1.0f, 0.0f, 0.0f, 0.0f};
static bool is_initialized = false;

static Quaternion multiply(Quaternion a, Quaternion b)
{
    Quaternion r;
    r.w = a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z;
    r.x = a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y;
    r.y = a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x;
    r.z = a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w;
    return r;
}

static void normalize_quaternion(Quaternion *p)
{
    float length = sqrtf(p->w*p->w + p->x*p->x + p->y*p->y + p->z*p->z);
    if (length > 1e-9f) {
        p->w /= length;
        p->x /= length;
        p->y /= length;
        p->z /= length;
    }
}

bool IMU_OrientationBegin(float ax, float ay, float az)
{
    is_initialized = false;
    float norm = sqrtf(ax*ax + ay*ay + az*az);
    if (!isfinite(norm) || norm < 0.85f || norm > 1.15f) {
        return false;
    }
    ax /= norm;
    ay /= norm;
    az /= norm;

    /* Rotacao minima que leva a gravidade medida para o eixo +Z.
       Ela compensa Roll/Pitch de montagem, mas nao identifica Yaw de montagem. */
    if (az < -0.9999f) {
        q.w = 0.0f;
        q.x = 1.0f;
        q.y = 0.0f;
        q.z = 0.0f;
    } else {
        float root = sqrtf(2.0f * (1.0f + az));
        q.w = 0.5f * root;
        q.x = ay / root;
        q.y = -ax / root;
        q.z = 0.0f;
    }
    normalize_quaternion(&q);
    q_zero = q;
    is_initialized = true;
    return true;
}

bool IMU_OrientationUpdate(float ax, float ay, float az,
                           float gx, float gy, float gz,
                           float dt, IMU_Attitude *out)
{
    if (!is_initialized || out == 0 || !isfinite(dt) || dt <= 0.0f || dt > 0.2f) {
        return false;
    }
    gx *= DEG_TO_RAD_F;
    gy *= DEG_TO_RAD_F;
    gz *= DEG_TO_RAD_F;

    /* Mahony IMU (6DoF): gravidade corrige Roll/Pitch,
       giroscopio integra todas as componentes, inclusive Yaw relativo. */
    float anorm = sqrtf(ax*ax + ay*ay + az*az);
    if (isfinite(anorm) && anorm > 0.85f && anorm < 1.15f) {
        ax /= anorm;
        ay /= anorm;
        az /= anorm;

        float vx = 2.0f*(q.x*q.z - q.w*q.y);
        float vy = 2.0f*(q.w*q.x + q.y*q.z);
        float vz = q.w*q.w - q.x*q.x - q.y*q.y + q.z*q.z;

        float ex = ay*vz - az*vy;
        float ey = az*vx - ax*vz;
        float ez = ax*vy - ay*vx;

        gx += KP * ex;
        gy += KP * ey;
        gz += KP * ez;
    }

    Quaternion old = q;
    float half_dt = 0.5f * dt;
    q.w += (-old.x*gx - old.y*gy - old.z*gz) * half_dt;
    q.x += ( old.w*gx + old.y*gz - old.z*gy) * half_dt;
    q.y += ( old.w*gy - old.x*gz + old.z*gx) * half_dt;
    q.z += ( old.w*gz + old.x*gy - old.y*gx) * half_dt;
    normalize_quaternion(&q);

    /* Orientacao relativa: montagem inicial e considerada zero. */
    Quaternion inverse_zero = {q_zero.w, -q_zero.x, -q_zero.y, -q_zero.z};
    Quaternion r = multiply(q, inverse_zero);
    normalize_quaternion(&r);

    float sin_pitch = 2.0f * (r.w*r.y - r.z*r.x);
    if (sin_pitch > 1.0f) sin_pitch = 1.0f;
    if (sin_pitch < -1.0f) sin_pitch = -1.0f;

    out->roll_deg = atan2f(2.0f*(r.w*r.x + r.y*r.z),
                           1.0f-2.0f*(r.x*r.x+r.y*r.y)) * RAD_TO_DEG_F;
    out->pitch_deg = asinf(sin_pitch) * RAD_TO_DEG_F;
    out->yaw_deg = atan2f(2.0f*(r.w*r.z + r.x*r.y),
                          1.0f-2.0f*(r.y*r.y+r.z*r.z)) * RAD_TO_DEG_F;
    return true;
}
