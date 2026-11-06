#ifndef GRIND_MATH_H
#define GRIND_MATH_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#define GRIND_PI 3.14159265358979323846f
#define GRIND_DEG2RAD(d) ((d) * (GRIND_PI / 180.0f))
#define GRIND_RAD2DEG(r) ((r) * (180.0f / GRIND_PI))

typedef struct {
    float x, y;
} Vec2;

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    float r, g, b, a;
} Color4;

typedef struct {
    float m[16]; /* Column-major 4x4 matrix for OpenGL */
} Mat4;

/* Deterministic xoshiro128++ PRNG */
typedef struct {
    uint32_t s[4];
} GrindRng;

static inline void grind_rng_seed(GrindRng *rng, uint64_t seed) {
    uint64_t z = (seed += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    uint64_t r1 = z ^ (z >> 31);

    z = (seed += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    uint64_t r2 = z ^ (z >> 31);

    rng->s[0] = (uint32_t)(r1 & 0xffffffffu) | 1u;
    rng->s[1] = (uint32_t)(r1 >> 32) | 3u;
    rng->s[2] = (uint32_t)(r2 & 0xffffffffu) | 5u;
    rng->s[3] = (uint32_t)(r2 >> 32) | 7u;
}

static inline uint32_t grind_rotl32(const uint32_t x, int k) {
    return (x << k) | (x >> (32 - k));
}

static inline uint32_t grind_rng_u32(GrindRng *rng) {
    const uint32_t result = grind_rotl32(rng->s[0] + rng->s[3], 7) + rng->s[0];
    const uint32_t t = rng->s[1] << 9;
    rng->s[2] ^= rng->s[0];
    rng->s[3] ^= rng->s[1];
    rng->s[1] ^= rng->s[2];
    rng->s[0] ^= rng->s[3];
    rng->s[2] ^= t;
    rng->s[3] = grind_rotl32(rng->s[3], 11);
    return result;
}

/* Returns float in [0.0f, 1.0f) */
static inline float grind_rng_f32(GrindRng *rng) {
    return (float)(grind_rng_u32(rng) >> 8) * (1.0f / 16777216.0f);
}

/* Returns float in [min_val, max_val] */
static inline float grind_rng_range(GrindRng *rng, float min_val, float max_val) {
    return min_val + (max_val - min_val) * grind_rng_f32(rng);
}

static inline float grind_clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static inline float grind_lerpf(float a, float b, float t) {
    return a + (b - a) * grind_clampf(t, 0.0f, 1.0f);
}

static inline Vec3 vec3_make(float x, float y, float z) {
    Vec3 v = {x, y, z};
    return v;
}

static inline Vec3 vec3_add(Vec3 a, Vec3 b) {
    return vec3_make(a.x + b.x, a.y + b.y, a.z + b.z);
}

static inline Vec3 vec3_sub(Vec3 a, Vec3 b) {
    return vec3_make(a.x - b.x, a.y - b.y, a.z - b.z);
}

static inline Vec3 vec3_scale(Vec3 v, float s) {
    return vec3_make(v.x * s, v.y * s, v.z * s);
}

static inline float vec3_dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return vec3_make(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

static inline float vec3_len_sq(Vec3 v) {
    return vec3_dot(v, v);
}

static inline float vec3_len(Vec3 v) {
    return sqrtf(vec3_len_sq(v));
}

static inline float vec3_dist_xz(Vec3 a, Vec3 b) {
    float dx = a.x - b.x;
    float dz = a.z - b.z;
    return sqrtf(dx * dx + dz * dz);
}

static inline Vec3 vec3_norm(Vec3 v) {
    float l = vec3_len(v);
    if (l < 1e-6f) return vec3_make(0.0f, 0.0f, 0.0f);
    return vec3_scale(v, 1.0f / l);
}

static inline Mat4 mat4_identity(void) {
    Mat4 r = {{
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    }};
    return r;
}

static inline Mat4 mat4_mul(Mat4 a, Mat4 b) {
    Mat4 r = {{0}};
    for (int c = 0; c < 4; ++c) {
        for (int row = 0; row < 4; ++row) {
            r.m[c * 4 + row] =
                a.m[0 * 4 + row] * b.m[c * 4 + 0] +
                a.m[1 * 4 + row] * b.m[c * 4 + 1] +
                a.m[2 * 4 + row] * b.m[c * 4 + 2] +
                a.m[3 * 4 + row] * b.m[c * 4 + 3];
        }
    }
    return r;
}

static inline Mat4 mat4_translate(Vec3 t) {
    Mat4 r = mat4_identity();
    r.m[12] = t.x;
    r.m[13] = t.y;
    r.m[14] = t.z;
    return r;
}

static inline Mat4 mat4_scale(Vec3 s) {
    Mat4 r = mat4_identity();
    r.m[0] = s.x;
    r.m[5] = s.y;
    r.m[10] = s.z;
    return r;
}

static inline Mat4 mat4_rotate_y(float rad) {
    Mat4 r = mat4_identity();
    float c = cosf(rad);
    float s = sinf(rad);
    r.m[0] = c;
    r.m[2] = -s;
    r.m[8] = s;
    r.m[10] = c;
    return r;
}

static inline Mat4 mat4_rotate_x(float rad) {
    Mat4 r = mat4_identity();
    float c = cosf(rad);
    float s = sinf(rad);
    r.m[5] = c;
    r.m[6] = s;
    r.m[9] = -s;
    r.m[10] = c;
    return r;
}

static inline Mat4 mat4_rotate_z(float rad) {
    Mat4 r = mat4_identity();
    float c = cosf(rad);
    float s = sinf(rad);
    r.m[0] = c;
    r.m[1] = s;
    r.m[4] = -s;
    r.m[5] = c;
    return r;
}

static inline Mat4 mat4_perspective(float fov_y_rad, float aspect, float z_near, float z_far) {
    Mat4 r = {{0}};
    float f = 1.0f / tanf(fov_y_rad * 0.5f);
    r.m[0] = f / (aspect > 0.01f ? aspect : 1.0f);
    r.m[5] = f;
    r.m[10] = (z_far + z_near) / (z_near - z_far);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * z_far * z_near) / (z_near - z_far);
    return r;
}

static inline Mat4 mat4_look_at(Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = vec3_norm(vec3_sub(center, eye));
    Vec3 s = vec3_norm(vec3_cross(f, up));
    Vec3 u = vec3_cross(s, f);

    Mat4 r = mat4_identity();
    r.m[0] = s.x;
    r.m[4] = s.y;
    r.m[8] = s.z;
    r.m[1] = u.x;
    r.m[5] = u.y;
    r.m[9] = u.z;
    r.m[2] = -f.x;
    r.m[6] = -f.y;
    r.m[10] = -f.z;
    r.m[12] = -vec3_dot(s, eye);
    r.m[13] = -vec3_dot(u, eye);
    r.m[14] = vec3_dot(f, eye);
    return r;
}

/* Projects a 3D world position into 2D viewport pixel coordinates (top-left origin).
 * Returns true if the point is in front of the camera and inside the view frustum. */
static inline bool grind_project_world_to_screen(
    Vec3 world_pos, Mat4 vp, int viewport_w, int viewport_h, Vec2 *out_screen
) {
    float cx = vp.m[0] * world_pos.x + vp.m[4] * world_pos.y + vp.m[8]  * world_pos.z + vp.m[12];
    float cy = vp.m[1] * world_pos.x + vp.m[5] * world_pos.y + vp.m[9]  * world_pos.z + vp.m[13];
    float cz = vp.m[2] * world_pos.x + vp.m[6] * world_pos.y + vp.m[10] * world_pos.z + vp.m[14];
    float cw = vp.m[3] * world_pos.x + vp.m[7] * world_pos.y + vp.m[11] * world_pos.z + vp.m[15];

    if (cw <= 0.1f) return false;
    float inv_w = 1.0f / cw;
    float ndc_x = cx * inv_w;
    float ndc_y = cy * inv_w;
    float ndc_z = cz * inv_w;

    if (ndc_x < -1.15f || ndc_x > 1.15f || ndc_y < -1.15f || ndc_y > 1.15f || ndc_z > 1.0f) {
        return false;
    }
    if (out_screen) {
        out_screen->x = (ndc_x * 0.5f + 0.5f) * (float)viewport_w;
        out_screen->y = (1.0f - (ndc_y * 0.5f + 0.5f)) * (float)viewport_h;
    }
    return true;
}

#endif /* GRIND_MATH_H */
