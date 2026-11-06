#include "grind_app/grind_renderer.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengles2.h>
#endif

#ifdef __EMSCRIPTEN__
#define SHADER_HEADER "#version 300 es\nprecision mediump float;\n"
#else
#define SHADER_HEADER "#version 330 core\n"
#endif

static const char *k_vs_3d =
    SHADER_HEADER
    "layout(location = 0) in vec3 a_pos;\n"
    "layout(location = 1) in vec3 a_norm;\n"
    "uniform mat4 u_mvp;\n"
    "uniform mat4 u_model;\n"
    "out vec3 v_norm;\n"
    "out vec3 v_world_pos;\n"
    "void main() {\n"
    "    vec4 wp = u_model * vec4(a_pos, 1.0);\n"
    "    v_world_pos = wp.xyz;\n"
    "    v_norm = mat3(u_model) * a_norm;\n"
    "    gl_Position = u_mvp * vec4(a_pos, 1.0);\n"
    "}\n";

static const char *k_fs_3d =
    SHADER_HEADER
    "in vec3 v_norm;\n"
    "in vec3 v_world_pos;\n"
    "uniform vec4 u_color;\n"
    "uniform vec3 u_light_dir;\n"
    "uniform vec3 u_ambient;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    vec3 n = normalize(v_norm);\n"
    "    float ndl = max(dot(n, normalize(u_light_dir)), 0.0);\n"
    "    float hemi = 0.5 + 0.5 * n.y;\n"
    "    vec3 light = u_ambient * (0.65 + 0.35 * hemi) + vec3(0.96, 0.93, 0.86) * ndl * 0.78;\n"
    "    vec3 base = u_color.rgb;\n"
    "    if (v_world_pos.y < -0.02 && n.y > 0.9) {\n"
    "        float gx = abs(fract(v_world_pos.x * 0.125) - 0.5);\n"
    "        float gz = abs(fract(v_world_pos.z * 0.125) - 0.5);\n"
    "        if (gx < 0.018 || gz < 0.018) base *= 0.90;\n"
    "    }\n"
    "    frag_color = vec4(base * light, u_color.a);\n"
    "}\n";

static const char *k_vs_2d =
    SHADER_HEADER
    "layout(location = 0) in vec2 a_pos;\n"
    "layout(location = 1) in vec4 a_col;\n"
    "uniform vec2 u_res;\n"
    "out vec4 v_col;\n"
    "void main() {\n"
    "    vec2 ndc = (a_pos / u_res) * 2.0 - 1.0;\n"
    "    ndc.y = -ndc.y;\n"
    "    v_col = a_col;\n"
    "    gl_Position = vec4(ndc, 0.0, 1.0);\n"
    "}\n";

static const char *k_fs_2d =
    SHADER_HEADER
    "in vec4 v_col;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    frag_color = v_col;\n"
    "}\n";

/* Unit cube centered on X/Z [-0.5, 0.5] and resting on Y [0.0, 1.0] */
static const float k_cube_verts[] = {
    /* Front (+Z) */
    -0.5f,0.0f, 0.5f, 0,0,1,   0.5f,0.0f, 0.5f, 0,0,1,   0.5f,1.0f, 0.5f, 0,0,1,
    -0.5f,0.0f, 0.5f, 0,0,1,   0.5f,1.0f, 0.5f, 0,0,1,  -0.5f,1.0f, 0.5f, 0,0,1,
    /* Back (-Z) */
     0.5f,0.0f,-0.5f, 0,0,-1, -0.5f,0.0f,-0.5f, 0,0,-1, -0.5f,1.0f,-0.5f, 0,0,-1,
     0.5f,0.0f,-0.5f, 0,0,-1, -0.5f,1.0f,-0.5f, 0,0,-1,  0.5f,1.0f,-0.5f, 0,0,-1,
    /* Left (-X) */
    -0.5f,0.0f,-0.5f,-1,0,0,  -0.5f,0.0f, 0.5f,-1,0,0,  -0.5f,1.0f, 0.5f,-1,0,0,
    -0.5f,0.0f,-0.5f,-1,0,0,  -0.5f,1.0f, 0.5f,-1,0,0,  -0.5f,1.0f,-0.5f,-1,0,0,
    /* Right (+X) */
     0.5f,0.0f, 0.5f, 1,0,0,   0.5f,0.0f,-0.5f, 1,0,0,   0.5f,1.0f,-0.5f, 1,0,0,
     0.5f,0.0f, 0.5f, 1,0,0,   0.5f,1.0f,-0.5f, 1,0,0,   0.5f,1.0f, 0.5f, 1,0,0,
    /* Top (+Y) */
    -0.5f,1.0f, 0.5f, 0,1,0,   0.5f,1.0f, 0.5f, 0,1,0,   0.5f,1.0f,-0.5f, 0,1,0,
    -0.5f,1.0f, 0.5f, 0,1,0,   0.5f,1.0f,-0.5f, 0,1,0,  -0.5f,1.0f,-0.5f, 0,1,0,
    /* Bottom (-Y) */
    -0.5f,0.0f,-0.5f, 0,-1,0,  0.5f,0.0f,-0.5f, 0,-1,0,  0.5f,0.0f, 0.5f, 0,-1,0,
    -0.5f,0.0f,-0.5f, 0,-1,0,  0.5f,0.0f, 0.5f, 0,-1,0, -0.5f,0.0f, 0.5f, 0,-1,0
};

static unsigned int compile_shader(GLenum type, const char *src) {
    unsigned int s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, sizeof(log), NULL, log);
        fprintf(stderr, "Shader compile error: %s\n", log);
    }
    return s;
}

static unsigned int link_program(const char *vs_src, const char *fs_src) {
    unsigned int vs = compile_shader(GL_VERTEX_SHADER, vs_src);
    unsigned int fs = compile_shader(GL_FRAGMENT_SHADER, fs_src);
    unsigned int p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

typedef struct {
    float x, y;
    float r, g, b, a;
} Vertex2D;

#define MAX_2D_VERTS 32768
static Vertex2D g_verts_2d[MAX_2D_VERTS];
static int g_vert_2d_count = 0;

static void push_quad_2d(float x, float y, float w, float h, Color4 c) {
    if (g_vert_2d_count + 6 > MAX_2D_VERTS) return;
    Vertex2D v[6] = {
        {x,     y,     c.r, c.g, c.b, c.a},
        {x + w, y,     c.r, c.g, c.b, c.a},
        {x + w, y + h, c.r, c.g, c.b, c.a},
        {x,     y,     c.r, c.g, c.b, c.a},
        {x + w, y + h, c.r, c.g, c.b, c.a},
        {x,     y + h, c.r, c.g, c.b, c.a}
    };
    memcpy(&g_verts_2d[g_vert_2d_count], v, sizeof(v));
    g_vert_2d_count += 6;
}

static uint16_t glyph_bits(char ch) {
    ch = (char)toupper((unsigned char)ch);
    switch (ch) {
        case '0': return 0x7B6F; case '1': return 0x2492; case '2': return 0x73E7;
        case '3': return 0x73CF; case '4': return 0x5BC9; case '5': return 0x79CF;
        case '6': return 0x79EF; case '7': return 0x7249; case '8': return 0x7BEF;
        case '9': return 0x7BC9; case 'A': return 0x7BED; case 'B': return 0x3BEB;
        case 'C': return 0x7927; case 'D': return 0x3B6B; case 'E': return 0x79E7;
        case 'F': return 0x79E4; case 'G': return 0x796F; case 'H': return 0x5BED;
        case 'I': return 0x7497; case 'J': return 0x126F; case 'K': return 0x5AED;
        case 'L': return 0x4927; case 'M': return 0x5FED; case 'N': return 0x5B6D;
        case 'O': return 0x7B6F; case 'P': return 0x7BE4; case 'Q': return 0x7B6B;
        case 'R': return 0x7BED; case 'S': return 0x79CF; case 'T': return 0x7492;
        case 'U': return 0x5B6F; case 'V': return 0x5B6A; case 'W': return 0x5B7D;
        case 'X': return 0x5AAD; case 'Y': return 0x5ACA; case 'Z': return 0x72A7;
        case '$': return 0x27D2; case '.': return 0x0002; case ':': return 0x0410;
        case '-': return 0x0380; case '+': return 0x05D0; case '/': return 0x12A4;
        case '%': return 0x52A5; case '[': return 0x6926; case ']': return 0x324B;
        case '(': return 0x2922; case ')': return 0x2492; case '#': return 0x5F7D;
        case '!': return 0x2482; case '"': return 0x5400; case '\'':return 0x2400;
        case ',': return 0x0024; case '&': return 0x3AA7; case '?': return 0x70C2;
        default:  return 0x0000;
    }
}

static void draw_text_2d(float x, float y, float scale, Color4 col, const char *text) {
    float cx = x;
    for (const char *p = text; *p; ++p) {
        if (*p == ' ') {
            cx += 4.0f * scale;
            continue;
        }
        uint16_t bits = glyph_bits(*p);
        for (int row = 0; row < 5; ++row) {
            for (int col_i = 0; col_i < 3; ++col_i) {
                int bit_idx = 14 - (row * 3 + col_i);
                if ((bits >> bit_idx) & 1) {
                    push_quad_2d(cx + col_i * scale, y + row * scale, scale, scale, col);
                }
            }
        }
        cx += 4.0f * scale;
    }
}

static void draw_world_label_2d(Vec3 world_pos, Mat4 vp, int vw, int vh, float scale, Color4 bg, Color4 fg, const char *text) {
    Vec2 scr;
    if (!grind_project_world_to_screen(world_pos, vp, vw, vh, &scr)) return;
    float len = (float)strlen(text);
    float w = len * 4.0f * scale + 10.0f;
    float h = 5.0f * scale + 8.0f;
    float x = scr.x - w * 0.5f;
    float y = scr.y - h * 0.5f;
    push_quad_2d(x, y, w, h, bg);
    draw_text_2d(x + 5.0f, y + 4.0f, scale, fg, text);
}

bool grind_renderer_init(GrindRenderer *r) {
    memset(r, 0, sizeof(*r));
    r->prog_3d = link_program(k_vs_3d, k_fs_3d);
    r->prog_2d = link_program(k_vs_2d, k_fs_2d);

    r->u_mvp_3d = glGetUniformLocation(r->prog_3d, "u_mvp");
    r->u_model_3d = glGetUniformLocation(r->prog_3d, "u_model");
    r->u_color_3d = glGetUniformLocation(r->prog_3d, "u_color");
    r->u_light_dir_3d = glGetUniformLocation(r->prog_3d, "u_light_dir");
    r->u_ambient_3d = glGetUniformLocation(r->prog_3d, "u_ambient");
    r->u_res_2d = glGetUniformLocation(r->prog_2d, "u_res");

    r->cube_vertex_count = 36;
    glGenVertexArrays(1, &r->vao_cube);
    glGenBuffers(1, &r->vbo_cube);
    glBindVertexArray(r->vao_cube);
    glBindBuffer(GL_ARRAY_BUFFER, r->vbo_cube);
    glBufferData(GL_ARRAY_BUFFER, sizeof(k_cube_verts), k_cube_verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));

    glGenVertexArrays(1, &r->vao_2d);
    glGenBuffers(1, &r->vbo_2d);
    glBindVertexArray(r->vao_2d);
    glBindBuffer(GL_ARRAY_BUFFER, r->vbo_2d);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_verts_2d), NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void *)(2 * sizeof(float)));

    glBindVertexArray(0);
    r->initialized = true;
    return true;
}

void grind_renderer_shutdown(GrindRenderer *r) {
    if (!r->initialized) return;
    glDeleteBuffers(1, &r->vbo_cube);
    glDeleteVertexArrays(1, &r->vao_cube);
    glDeleteBuffers(1, &r->vbo_2d);
    glDeleteVertexArrays(1, &r->vao_2d);
    glDeleteProgram(r->prog_3d);
    glDeleteProgram(r->prog_2d);
    r->initialized = false;
}

static void draw_mat_3d(const GrindRenderer *r, Mat4 vp, Mat4 m, Color4 col) {
    Mat4 mvp = mat4_mul(vp, m);
    glUniformMatrix4fv(r->u_mvp_3d, 1, GL_FALSE, mvp.m);
    glUniformMatrix4fv(r->u_model_3d, 1, GL_FALSE, m.m);
    glUniform4f(r->u_color_3d, col.r, col.g, col.b, col.a);
    glDrawArrays(GL_TRIANGLES, 0, r->cube_vertex_count);
}

static void draw_box_3d(const GrindRenderer *r, Mat4 vp, Vec3 pos, Vec3 size, float yaw_rad, Color4 col) {
    Mat4 m = mat4_mul(mat4_translate(pos), mat4_mul(mat4_rotate_y(yaw_rad), mat4_scale(size)));
    draw_mat_3d(r, vp, m, col);
}

/* Renders a box in a parent transform's local space with X/Y/Z rotation */
static void draw_child_box(const GrindRenderer *r, Mat4 vp, Mat4 parent, Vec3 local_pos, Vec3 rot_xyz, Vec3 size, Color4 col) {
    Mat4 m = mat4_mul(parent, mat4_translate(local_pos));
    if (fabsf(rot_xyz.y) > 1e-4f) m = mat4_mul(m, mat4_rotate_y(rot_xyz.y));
    if (fabsf(rot_xyz.x) > 1e-4f) m = mat4_mul(m, mat4_rotate_x(rot_xyz.x));
    if (fabsf(rot_xyz.z) > 1e-4f) m = mat4_mul(m, mat4_rotate_z(rot_xyz.z));
    m = mat4_mul(m, mat4_scale(size));
    draw_mat_3d(r, vp, m, col);
}

/* Draws a low-poly park tree (trunk + 2-tier leafy crown) */
static void draw_tree_3d(const GrindRenderer *r, Mat4 vp, Vec3 pos, float scale) {
    draw_box_3d(r, vp, pos, vec3_make(0.55f * scale, 2.2f * scale, 0.55f * scale), 0.0f, (Color4){0.42f, 0.28f, 0.16f, 1.0f});
    draw_box_3d(r, vp, vec3_make(pos.x, pos.y + 1.9f * scale, pos.z),
                vec3_make(2.6f * scale, 1.9f * scale, 2.6f * scale), 0.25f, (Color4){0.20f, 0.58f, 0.26f, 1.0f});
    draw_box_3d(r, vp, vec3_make(pos.x, pos.y + 3.4f * scale, pos.z),
                vec3_make(1.7f * scale, 1.4f * scale, 1.7f * scale), 0.6f, (Color4){0.28f, 0.68f, 0.32f, 1.0f});
}

/* Draws a streetlamp with dusk/evening glow */
static void draw_streetlamp_3d(const GrindRenderer *r, Mat4 vp, Vec3 pos, bool evening) {
    draw_box_3d(r, vp, pos, vec3_make(0.22f, 4.8f, 0.22f), 0.0f, (Color4){0.22f, 0.25f, 0.28f, 1.0f});
    draw_box_3d(r, vp, vec3_make(pos.x, 4.6f, pos.z + 0.4f), vec3_make(0.25f, 0.18f, 1.0f), 0.0f, (Color4){0.22f, 0.25f, 0.28f, 1.0f});
    Color4 bulb = evening ? (Color4){1.0f, 0.92f, 0.45f, 1.0f} : (Color4){0.75f, 0.80f, 0.85f, 1.0f};
    draw_box_3d(r, vp, vec3_make(pos.x, 4.35f, pos.z + 0.75f), vec3_make(0.45f, 0.30f, 0.45f), 0.0f, bulb);
}

/* Draws a detailed 3D building with sidewalk plinth, door, shop awning, window grid, and rooftop AC */
static void draw_detailed_building(const GrindRenderer *r, Mat4 vp, const GrindBuilding *b, bool evening) {
    /* Concrete sidewalk base */
    draw_box_3d(r, vp, vec3_make(b->pos.x, 0.0f, b->pos.z),
                vec3_make(b->size.x + 3.6f, 0.22f, b->size.z + 3.6f), 0.0f, (Color4){0.55f, 0.56f, 0.58f, 1.0f});

    /* Main building body */
    draw_box_3d(r, vp, b->pos, b->size, 0.0f, b->color);

    /* Roof cornice & rooftop unit */
    draw_box_3d(r, vp, vec3_make(b->pos.x, b->size.y, b->pos.z),
                vec3_make(b->size.x + 0.6f, 0.45f, b->size.z + 0.6f), 0.0f,
                (Color4){b->color.r * 0.65f, b->color.g * 0.65f, b->color.b * 0.65f, 1.0f});
    draw_box_3d(r, vp, vec3_make(b->pos.x - b->size.x * 0.2f, b->size.y + 0.45f, b->pos.z),
                vec3_make(b->size.x * 0.3f, 1.3f, b->size.z * 0.3f), 0.0f, (Color4){0.38f, 0.40f, 0.44f, 1.0f});

    if (b->size.y < 5.0f) return; /* Central Plaza pavilion doesn't need multi-story windows */

    /* Entrance double doors on +Z front face */
    float front_z = b->pos.z + b->size.z * 0.5f + 0.08f;
    draw_box_3d(r, vp, vec3_make(b->pos.x, 0.2f, front_z),
                vec3_make(2.8f, 2.7f, 0.2f), 0.0f, (Color4){0.22f, 0.16f, 0.12f, 1.0f});

    /* Shopfront awning above door */
    draw_box_3d(r, vp, vec3_make(b->pos.x, 3.0f, front_z + 0.65f),
                vec3_make(b->size.x * 0.65f, 0.45f, 1.5f), 0.0f,
                (Color4){fminf(1.0f, b->color.r * 1.25f), b->color.g * 0.85f, b->color.b * 0.85f, 1.0f});

    /* Window Grid across front (+Z) and back (-Z) facades */
    Color4 win_col = evening
        ? (Color4){0.98f, 0.86f, 0.42f, 1.0f}
        : (Color4){0.22f, 0.32f, 0.45f, 1.0f};

    int floors = (int)((b->size.y - 4.0f) / 3.2f);
    if (floors > 6) floors = 6;
    int cols = (int)((b->size.x - 3.0f) / 3.4f);
    if (cols > 4) cols = 4;

    for (int f = 0; f < floors; ++f) {
        float wy = 4.2f + (float)f * 3.1f;
        for (int c = 0; c < cols; ++c) {
            float wx = b->pos.x - ((float)(cols - 1) * 1.6f) + (float)c * 3.2f;
            draw_box_3d(r, vp, vec3_make(wx, wy, b->pos.z + b->size.z * 0.5f + 0.06f),
                        vec3_make(1.5f, 1.7f, 0.12f), 0.0f, win_col);
            draw_box_3d(r, vp, vec3_make(wx, wy, b->pos.z - b->size.z * 0.5f - 0.06f),
                        vec3_make(1.5f, 1.7f, 0.12f), 0.0f, win_col);
        }
    }
}

/* Draws an articulated 3D human character (head, hair, torso, swinging arms & legs, held cup or parcel) */
static void draw_articulated_human(
    const GrindRenderer *r, Mat4 vp, Vec3 pos, float yaw_rad, float walk_phase,
    Color4 shirt_col, Color4 pants_col, bool holding_cup, bool holding_parcel, bool envy_aura
) {
    Mat4 root = mat4_mul(mat4_translate(pos), mat4_rotate_y(yaw_rad));
    float swing = sinf(walk_phase) * 0.55f;
    Color4 skin_col = {0.92f, 0.76f, 0.64f, 1.0f};
    Color4 hair_col = {0.18f, 0.14f, 0.12f, 1.0f};

    /* Left & Right Legs (pivoting around hip y=0.75) */
    draw_child_box(r, vp, root, vec3_make(-0.16f, 0.0f, sinf(swing) * 0.18f),
                   vec3_make(swing * 0.5f, 0.0f, 0.0f), vec3_make(0.22f, 0.75f, 0.22f), pants_col);
    draw_child_box(r, vp, root, vec3_make( 0.16f, 0.0f, -sinf(swing) * 0.18f),
                   vec3_make(-swing * 0.5f, 0.0f, 0.0f), vec3_make(0.22f, 0.75f, 0.22f), pants_col);

    /* Torso */
    draw_child_box(r, vp, root, vec3_make(0.0f, 0.72f, 0.0f),
                   vec3_make(0.0f, 0.0f, 0.0f), vec3_make(0.56f, 0.72f, 0.32f), shirt_col);

    /* Head & Hair */
    draw_child_box(r, vp, root, vec3_make(0.0f, 1.46f, 0.0f),
                   vec3_make(0.0f, 0.0f, 0.0f), vec3_make(0.38f, 0.38f, 0.38f), skin_col);
    draw_child_box(r, vp, root, vec3_make(0.0f, 1.78f, 0.0f),
                   vec3_make(0.0f, 0.0f, 0.0f), vec3_make(0.42f, 0.12f, 0.42f), hair_col);

    /* Left & Right Arms */
    float r_arm_pitch = holding_cup ? -0.95f : (holding_parcel ? -0.75f : -swing * 0.6f);
    float l_arm_pitch = holding_parcel ? -0.75f : swing * 0.6f;

    draw_child_box(r, vp, root, vec3_make(-0.38f, 0.78f, 0.0f),
                   vec3_make(l_arm_pitch, 0.0f, 0.0f), vec3_make(0.18f, 0.62f, 0.18f), shirt_col);
    draw_child_box(r, vp, root, vec3_make( 0.38f, 0.78f, 0.0f),
                   vec3_make(r_arm_pitch, 0.0f, 0.0f), vec3_make(0.18f, 0.62f, 0.18f), shirt_col);

    /* Held Lemonade Cup in Right Hand */
    if (holding_cup) {
        draw_child_box(r, vp, root, vec3_make(0.38f, 1.15f, -0.35f),
                       vec3_make(0.0f, 0.0f, 0.0f), vec3_make(0.16f, 0.24f, 0.16f),
                       (Color4){0.98f, 0.92f, 0.18f, 1.0f});
    }

    /* Held Courier Parcel / Backpack */
    if (holding_parcel) {
        draw_child_box(r, vp, root, vec3_make(0.0f, 0.85f, 0.28f),
                       vec3_make(0.0f, 0.0f, 0.0f), vec3_make(0.48f, 0.52f, 0.28f),
                       (Color4){0.85f, 0.55f, 0.22f, 1.0f});
    }

    /* Social status ring above head for envious/hostile reaction */
    if (envy_aura) {
        draw_child_box(r, vp, root, vec3_make(0.0f, 2.02f, 0.0f),
                       vec3_make(0.0f, walk_phase, 0.0f), vec3_make(0.28f, 0.12f, 0.28f),
                       (Color4){0.95f, 0.20f, 0.20f, 1.0f});
    }
}

/* Draws a 3D Bicycle prop (two wheels, frame, seat, handlebars, rear courier basket) */
static void draw_bicycle_3d(const GrindRenderer *r, Mat4 vp, Vec3 pos, float yaw_rad, Color4 frame_col) {
    Mat4 root = mat4_mul(mat4_translate(pos), mat4_rotate_y(yaw_rad));
    Color4 tire_col = {0.14f, 0.15f, 0.17f, 1.0f};
    /* Front & Rear Wheels */
    draw_child_box(r, vp, root, vec3_make(0.0f, 0.0f, -0.65f), vec3_make(0,0,0), vec3_make(0.12f, 0.65f, 0.65f), tire_col);
    draw_child_box(r, vp, root, vec3_make(0.0f, 0.0f,  0.65f), vec3_make(0,0,0), vec3_make(0.12f, 0.65f, 0.65f), tire_col);
    /* Frame bar & fork */
    draw_child_box(r, vp, root, vec3_make(0.0f, 0.55f, 0.0f), vec3_make(0,0,0), vec3_make(0.10f, 0.14f, 1.25f), frame_col);
    draw_child_box(r, vp, root, vec3_make(0.0f, 0.35f, -0.60f), vec3_make(0,0,0), vec3_make(0.10f, 0.85f, 0.12f), frame_col);
    /* Handlebars & Saddle */
    draw_child_box(r, vp, root, vec3_make(0.0f, 1.15f, -0.60f), vec3_make(0,0,0), vec3_make(0.75f, 0.08f, 0.10f), tire_col);
    draw_child_box(r, vp, root, vec3_make(0.0f, 0.82f,  0.25f), vec3_make(0,0,0), vec3_make(0.26f, 0.10f, 0.35f), tire_col);
    /* Rear Cargo Crate */
    draw_child_box(r, vp, root, vec3_make(0.0f, 0.72f,  0.68f), vec3_make(0,0,0), vec3_make(0.55f, 0.38f, 0.55f),
                   (Color4){0.62f, 0.42f, 0.22f, 1.0f});
}

void grind_renderer_draw_frame(GrindRenderer *r, const GrindWorld *world, const GrindInputState *input) {
    if (!r->initialized) return;

    int vw_i = input->viewport_w;
    int vh_i = input->viewport_h;
    glViewport(0, 0, vw_i, vh_i);

    bool evening = (world->time_of_day_hours >= 18.0f || world->time_of_day_hours < 6.5f);
    float day_t = sinf(grind_clampf((world->time_of_day_hours - 6.0f) / 16.0f, 0.0f, 1.0f) * GRIND_PI);
    float sky_r = 0.10f + 0.38f * day_t;
    float sky_g = 0.15f + 0.54f * day_t;
    float sky_b = 0.28f + 0.64f * day_t;
    if (world->weather == WEATHER_RAINY) {
        sky_r *= 0.55f; sky_g *= 0.58f; sky_b *= 0.65f;
    }
    glClearColor(sky_r, sky_g, sky_b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    Mat4 view = grind_input_compute_view_matrix(input, world);
    Mat4 proj = grind_input_compute_proj_matrix(input);
    Mat4 vp = mat4_mul(proj, view);

    glUseProgram(r->prog_3d);
    glBindVertexArray(r->vao_cube);

    float sun_angle = ((world->time_of_day_hours - 6.0f) / 15.0f) * GRIND_PI;
    glUniform3f(r->u_light_dir_3d, cosf(sun_angle) * 0.6f, fmaxf(0.25f, sinf(sun_angle)), 0.45f);
    glUniform3f(r->u_ambient_3d, 0.35f + 0.25f * day_t, 0.36f + 0.25f * day_t, 0.42f + 0.20f * day_t);

    /* 1. Ground Terrain, Asphalt Roads with Lane Dashes & Central Plaza */
    draw_box_3d(r, vp, vec3_make(0.0f, -0.5f, 0.0f), vec3_make(260.0f, 0.5f, 260.0f), 0.0f, (Color4){0.24f, 0.32f, 0.24f, 1.0f});
    /* Main North-South and East-West Asphalt Boulevards */
    draw_box_3d(r, vp, vec3_make(-22.0f, -0.04f, 0.0f), vec3_make(12.0f, 0.06f, 220.0f), 0.0f, (Color4){0.18f, 0.20f, 0.23f, 1.0f});
    draw_box_3d(r, vp, vec3_make( 22.0f, -0.04f, 0.0f), vec3_make(12.0f, 0.06f, 220.0f), 0.0f, (Color4){0.18f, 0.20f, 0.23f, 1.0f});
    draw_box_3d(r, vp, vec3_make(0.0f, -0.04f, -26.0f), vec3_make(220.0f, 0.06f, 12.0f), 0.0f, (Color4){0.18f, 0.20f, 0.23f, 1.0f});
    draw_box_3d(r, vp, vec3_make(0.0f, -0.04f,  26.0f), vec3_make(220.0f, 0.06f, 12.0f), 0.0f, (Color4){0.18f, 0.20f, 0.23f, 1.0f});

    /* Yellow Road Center Dashes */
    for (int d = -90; d <= 90; d += 12) {
        draw_box_3d(r, vp, vec3_make(-22.0f, 0.02f, (float)d), vec3_make(0.35f, 0.02f, 4.5f), 0.0f, (Color4){0.92f, 0.80f, 0.20f, 1.0f});
        draw_box_3d(r, vp, vec3_make( 22.0f, 0.02f, (float)d), vec3_make(0.35f, 0.02f, 4.5f), 0.0f, (Color4){0.92f, 0.80f, 0.20f, 1.0f});
        draw_box_3d(r, vp, vec3_make((float)d, 0.02f, -26.0f), vec3_make(4.5f, 0.02f, 0.35f), 0.0f, (Color4){0.92f, 0.80f, 0.20f, 1.0f});
        draw_box_3d(r, vp, vec3_make((float)d, 0.02f,  26.0f), vec3_make(4.5f, 0.02f, 0.35f), 0.0f, (Color4){0.92f, 0.80f, 0.20f, 1.0f});
    }

    /* Central Sunny Plaza Paving & Park Trees/Streetlamps */
    draw_box_3d(r, vp, vec3_make(0.0f, -0.02f, 0.0f), vec3_make(32.0f, 0.08f, 32.0f), 0.0f, (Color4){0.72f, 0.68f, 0.56f, 1.0f});
    const Vec3 tree_locs[] = {
        {-13.0f, 0.0f, -13.0f}, { 13.0f, 0.0f, -13.0f}, {-13.0f, 0.0f,  13.0f}, { 13.0f, 0.0f,  13.0f},
        {-38.0f, 0.0f, -35.0f}, { 48.0f, 0.0f,   5.0f}, {-45.0f, 0.0f,  20.0f}, {  8.0f, 0.0f, -45.0f}
    };
    for (int i = 0; i < 8; ++i) {
        draw_tree_3d(r, vp, tree_locs[i], 1.15f);
    }
    const Vec3 lamp_locs[] = {
        {-15.0f, 0.0f, -20.0f}, { 15.0f, 0.0f, -20.0f}, {-15.0f, 0.0f,  20.0f},
        { 15.0f, 0.0f,  20.0f}, {-42.0f, 0.0f, -40.0f}, { 28.0f, 0.0f,  22.0f}
    };
    for (int i = 0; i < 6; ++i) {
        draw_streetlamp_3d(r, vp, lamp_locs[i], evening);
    }

    /* 2. Detailed 3D City Buildings & Landmarks */
    for (int i = 0; i < world->building_count; ++i) {
        draw_detailed_building(r, vp, &world->buildings[i], evening);
    }

    /* 3. Display Bicycles Outside the Bike Shop */
    draw_bicycle_3d(r, vp, vec3_make(34.0f, 0.0f, -10.0f), 0.5f, (Color4){0.18f, 0.72f, 0.92f, 1.0f});
    draw_bicycle_3d(r, vp, vec3_make(37.0f, 0.0f, -10.0f), 0.5f, (Color4){0.92f, 0.32f, 0.25f, 1.0f});

    /* 4. Found Money on Ground (Spinning/Bobbing Green Cash Bundle + Golden Beacon) */
    if (world->ground_money_available) {
        float bob = 0.15f + 0.12f * sinf(world->anim_time_sec * 3.0f);
        Vec3 mp = vec3_make(world->ground_money_pos.x, bob, world->ground_money_pos.z);
        draw_box_3d(r, vp, mp, vec3_make(1.2f, 0.30f, 0.65f), world->anim_time_sec, (Color4){0.15f, 0.92f, 0.32f, 1.0f});
        draw_box_3d(r, vp, vec3_make(mp.x, 0.0f, mp.z), vec3_make(0.25f, 2.8f, 0.25f), 0.0f, (Color4){0.98f, 0.92f, 0.22f, 1.0f});
    }

    /* 5. Deployed Lemonade Stand (Table Legs, Tablecloth, Glass Pitcher, Lemons, Cups, Striped Umbrella) */
    const GrindPlayer *p = &world->player;
    if (p->stand_deployed) {
        Vec3 sp = p->stand_pos;
        if (p->owns_storefront) {
            draw_box_3d(r, vp, sp, vec3_make(6.2f, 4.5f, 5.0f), 0.0f, (Color4){0.95f, 0.82f, 0.22f, 1.0f});
            draw_box_3d(r, vp, vec3_make(sp.x, 3.2f, sp.z + 2.6f), vec3_make(5.8f, 0.6f, 1.2f), 0.0f, (Color4){0.92f, 0.25f, 0.22f, 1.0f});
        } else {
            /* 4 Table legs + Tabletop */
            draw_box_3d(r, vp, vec3_make(sp.x - 1.0f, 0.0f, sp.z - 0.5f), vec3_make(0.15f, 1.0f, 0.15f), 0.0f, (Color4){0.35f, 0.25f, 0.18f, 1.0f});
            draw_box_3d(r, vp, vec3_make(sp.x + 1.0f, 0.0f, sp.z - 0.5f), vec3_make(0.15f, 1.0f, 0.15f), 0.0f, (Color4){0.35f, 0.25f, 0.18f, 1.0f});
            draw_box_3d(r, vp, vec3_make(sp.x - 1.0f, 0.0f, sp.z + 0.5f), vec3_make(0.15f, 1.0f, 0.15f), 0.0f, (Color4){0.35f, 0.25f, 0.18f, 1.0f});
            draw_box_3d(r, vp, vec3_make(sp.x + 1.0f, 0.0f, sp.z + 0.5f), vec3_make(0.15f, 1.0f, 0.15f), 0.0f, (Color4){0.35f, 0.25f, 0.18f, 1.0f});
            draw_box_3d(r, vp, vec3_make(sp.x, 0.95f, sp.z), vec3_make(2.6f, 0.20f, 1.5f), 0.0f, (Color4){0.95f, 0.95f, 0.92f, 1.0f});
            /* Lemonade Pitcher & Cups on table */
            draw_box_3d(r, vp, vec3_make(sp.x - 0.4f, 1.15f, sp.z), vec3_make(0.45f, 0.65f, 0.45f), 0.0f, (Color4){0.98f, 0.92f, 0.15f, 1.0f});
            int visible_cups = p->stand_lemonade_stock < 6 ? p->stand_lemonade_stock : 6;
            for (int c = 0; c < visible_cups; ++c) {
                draw_box_3d(r, vp, vec3_make(sp.x + 0.15f + (float)c * 0.18f, 1.15f, sp.z + 0.25f),
                            vec3_make(0.12f, 0.22f, 0.12f), 0.0f, (Color4){0.98f, 0.88f, 0.25f, 1.0f});
            }
            /* Umbrella Pole & Canopy */
            draw_box_3d(r, vp, vec3_make(sp.x, 1.15f, sp.z), vec3_make(0.10f, 2.2f, 0.10f), 0.0f, (Color4){0.80f, 0.80f, 0.82f, 1.0f});
            draw_box_3d(r, vp, vec3_make(sp.x, 3.25f, sp.z), vec3_make(3.4f, 0.35f, 3.4f), 0.0f, (Color4){0.95f, 0.26f, 0.22f, 1.0f});
        }
    }

    /* 6. Active Courier Parcels & Dropoff Beacons */
    for (int i = 0; i < GRIND_MAX_CONTRACTS; ++i) {
        const GrindCourierContract *c = &world->contracts[i];
        if (!c->active) continue;
        if (!c->picked_up) {
            draw_box_3d(r, vp, c->pickup_pos, vec3_make(1.0f, 0.9f, 1.0f), 0.2f, (Color4){0.82f, 0.56f, 0.26f, 1.0f});
            /* Packing tape stripe */
            draw_box_3d(r, vp, vec3_make(c->pickup_pos.x, 0.91f, c->pickup_pos.z), vec3_make(1.02f, 0.05f, 0.22f), 0.2f, (Color4){0.95f, 0.88f, 0.65f, 1.0f});
        } else if (c->claimed_by_player) {
            draw_box_3d(r, vp, c->dropoff_pos, vec3_make(1.4f, 6.5f, 1.4f), world->anim_time_sec, (Color4){0.20f, 0.95f, 0.88f, 1.0f});
        }
    }

    /* 7. Articulated Hired Employees Executing Task Graphs */
    for (int i = 0; i < world->employee_count; ++i) {
        const GrindEmployee *e = &world->employees[i];
        if (!e->hired) continue;
        bool is_courier = (e->role == EMP_ROLE_COURIER);
        draw_articulated_human(
            r, vp, e->pos, e->facing_yaw, e->walk_phase,
            (Color4){0.15f, 0.78f, 0.92f, 1.0f},
            (Color4){0.18f, 0.22f, 0.30f, 1.0f},
            !is_courier, is_courier, false
        );
    }

    /* 8. Articulated 3D NPCs (Swinging limbs, drinking lemonade cups, envy/status indicators) */
    for (int i = 0; i < world->npc_count; ++i) {
        const GrindNpc *npc = &world->npcs[i];
        Color4 shirt_col = {0.35f, 0.55f, 0.78f, 1.0f};
        Color4 pants_col = {0.20f, 0.24f, 0.32f, 1.0f};
        if (npc->archetype == ARCHETYPE_LOYAL_FRIEND) {
            shirt_col = (Color4){0.22f, 0.78f, 0.38f, 1.0f};
        } else if (npc->archetype == ARCHETYPE_ENVIOUS_PEER) {
            shirt_col = (npc->envy > 0.35f) ? (Color4){0.88f, 0.22f, 0.22f, 1.0f} : (Color4){0.68f, 0.52f, 0.32f, 1.0f};
        } else if (npc->archetype == ARCHETYPE_OPPORTUNIST) {
            shirt_col = (npc->affection > 0.3f) ? (Color4){0.95f, 0.78f, 0.18f, 1.0f} : (Color4){0.65f, 0.35f, 0.68f, 1.0f};
        } else if (npc->archetype == ARCHETYPE_BANKER) {
            shirt_col = (Color4){0.16f, 0.20f, 0.28f, 1.0f};
            pants_col = (Color4){0.14f, 0.16f, 0.22f, 1.0f};
        }

        draw_articulated_human(
            r, vp, npc->pos, npc->facing_yaw, npc->walk_phase,
            shirt_col, pants_col,
            npc->drink_timer > 0.0f, false, npc->envy > 0.45f
        );
    }

    /* 9. Player Avatar (in Top-Down Map) OR Tactile First-Person Viewmodel (Both Hands, Held Props, Bike/Car) */
    if (input->map_transition_t > 0.15f) {
        draw_articulated_human(
            r, vp, p->pos, p->yaw_rad, (p->pos.x + p->pos.z) * 1.5f,
            (Color4){0.98f, 0.92f, 0.20f, 1.0f},
            (Color4){0.18f, 0.22f, 0.32f, 1.0f},
            false, p->has_satchel, false
        );
    } else {
        float stride = sinf((p->pos.x + p->pos.z) * 1.4f) * 0.03f;
        float eye_h = (p->vehicle == VEHICLE_BICYCLE || p->vehicle == VEHICLE_CARGO_EBIKE) ? 1.85f : 1.68f;
        Mat4 cam_root = mat4_mul(
            mat4_translate(vec3_make(p->pos.x, p->pos.y + eye_h + stride, p->pos.z)),
            mat4_mul(mat4_rotate_y(p->yaw_rad), mat4_rotate_x(p->pitch_rad * 0.4f))
        );

        Color4 sleeve_col = {0.45f, 0.45f, 0.48f, 1.0f};
        if (p->apparel == APPAREL_CLEAN_CASUAL) sleeve_col = (Color4){0.25f, 0.58f, 0.85f, 1.0f};
        else if (p->apparel == APPAREL_COURIER_GEAR) sleeve_col = (Color4){0.92f, 0.55f, 0.15f, 1.0f};
        else if (p->apparel == APPAREL_TAILORED_SUIT) sleeve_col = (Color4){0.14f, 0.18f, 0.28f, 1.0f};
        Color4 hand_skin = {0.92f, 0.76f, 0.64f, 1.0f};

        /* Left & Right Forearms + Hands */
        draw_child_box(r, vp, cam_root, vec3_make(-0.34f, -0.38f, -0.55f), vec3_make(0.2f, -0.15f, 0.0f), vec3_make(0.12f, 0.12f, 0.36f), sleeve_col);
        draw_child_box(r, vp, cam_root, vec3_make(-0.32f, -0.36f, -0.74f), vec3_make(0.0f,  0.0f,  0.0f), vec3_make(0.11f, 0.11f, 0.12f), hand_skin);

        draw_child_box(r, vp, cam_root, vec3_make( 0.34f, -0.38f, -0.55f), vec3_make(0.2f,  0.15f, 0.0f), vec3_make(0.12f, 0.12f, 0.36f), sleeve_col);
        draw_child_box(r, vp, cam_root, vec3_make( 0.32f, -0.36f, -0.74f), vec3_make(0.0f,  0.0f,  0.0f), vec3_make(0.11f, 0.11f, 0.12f), hand_skin);

        /* Swiss Luxury Chronograph Watch on Left Wrist */
        if (p->has_luxury_watch) {
            draw_child_box(r, vp, cam_root, vec3_make(-0.33f, -0.31f, -0.66f), vec3_make(0,0,0), vec3_make(0.14f, 0.04f, 0.06f), (Color4){0.98f, 0.84f, 0.18f, 1.0f});
        }

        /* Held Physical Items in Player's Hands (Table, Ingredients, or Courier Parcel) */
        if (p->inventory[ITEM_FOLDING_TABLE] > 0) {
            draw_child_box(r, vp, cam_root, vec3_make(0.0f, -0.35f, -0.82f), vec3_make(0,0,0), vec3_make(0.65f, 0.42f, 0.12f), (Color4){0.65f, 0.42f, 0.22f, 1.0f});
        } else if (p->inventory[ITEM_LEMON] > 0 || p->inventory[ITEM_SUGAR] > 0) {
            draw_child_box(r, vp, cam_root, vec3_make(0.28f, -0.28f, -0.78f), vec3_make(0,0,0), vec3_make(0.22f, 0.18f, 0.22f), (Color4){0.98f, 0.90f, 0.18f, 1.0f});
        }

        /* Vehicle Cockpit / Handlebars in First-Person */
        if (p->vehicle == VEHICLE_BICYCLE || p->vehicle == VEHICLE_CARGO_EBIKE) {
            Color4 bike_c = (p->vehicle == VEHICLE_CARGO_EBIKE) ? (Color4){0.20f, 0.88f, 0.48f, 1.0f} : (Color4){0.20f, 0.72f, 0.92f, 1.0f};
            draw_child_box(r, vp, cam_root, vec3_make(0.0f, -0.42f, -0.68f), vec3_make(0,0,0), vec3_make(0.76f, 0.05f, 0.06f), (Color4){0.18f, 0.20f, 0.22f, 1.0f});
            draw_child_box(r, vp, cam_root, vec3_make(0.0f, -0.75f, -0.68f), vec3_make(0,0,0), vec3_make(0.08f, 0.35f, 0.08f), bike_c);
        } else if (p->vehicle == VEHICLE_DELIVERY_VAN || p->vehicle == VEHICLE_SPORTS_CAR) {
            Color4 car_c = (p->vehicle == VEHICLE_SPORTS_CAR) ? (Color4){0.92f, 0.16f, 0.18f, 1.0f} : (Color4){0.88f, 0.88f, 0.90f, 1.0f};
            draw_child_box(r, vp, cam_root, vec3_make(0.0f, -0.52f, -0.95f), vec3_make(0,0,0), vec3_make(1.4f, 0.16f, 0.65f), car_c);
            draw_child_box(r, vp, cam_root, vec3_make(0.0f, -0.38f, -0.62f), vec3_make(0.4f,0,0), vec3_make(0.36f, 0.36f, 0.05f), (Color4){0.15f, 0.16f, 0.18f, 1.0f});
        }
    }

    /* ================= 2D HUD, 3D WORLD LABELS & TOUCH CONTROLS ================= */
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    g_vert_2d_count = 0;
    float vw = (float)vw_i;
    float vh = (float)vh_i;

    /* A. 3D World-Space Projected Landmark Labels, Stand Price Sign & Speech Bubbles */
    for (int i = 0; i < ZONE_COUNT; ++i) {
        const GrindBuilding *b = &world->buildings[i];
        float d = vec3_dist_xz(p->pos, b->pos);
        if (d < 85.0f || input->map_transition_t > 0.3f) {
            Vec3 label_pos = vec3_make(b->pos.x, b->size.y + 2.8f, b->pos.z);
            draw_world_label_2d(label_pos, vp, vw_i, vh_i, 2.0f,
                                (Color4){0.05f, 0.08f, 0.14f, 0.78f},
                                (Color4){0.98f, 0.94f, 0.65f, 1.0f}, b->name);
        }
    }

    if (world->ground_money_available) {
        Vec3 mp = vec3_make(world->ground_money_pos.x, 3.2f, world->ground_money_pos.z);
        draw_world_label_2d(mp, vp, vw_i, vh_i, 2.2f,
                            (Color4){0.08f, 0.35f, 0.14f, 0.88f},
                            (Color4){0.55f, 1.0f, 0.65f, 1.0f}, "FOUND CASH $12 [PRESS E]");
    }

    if (p->stand_deployed) {
        char stand_tag[64];
        snprintf(stand_tag, sizeof(stand_tag), "LEMONADE $%.2f (%d CUPS)", p->stand_price, p->stand_lemonade_stock);
        Vec3 sp = vec3_make(p->stand_pos.x, p->owns_storefront ? 5.2f : 4.1f, p->stand_pos.z);
        draw_world_label_2d(sp, vp, vw_i, vh_i, 2.1f,
                            (Color4){0.22f, 0.16f, 0.04f, 0.85f},
                            (Color4){1.0f, 0.92f, 0.25f, 1.0f}, stand_tag);
    }

    if (world->sale_popup_timer > 0.0f) {
        char sale_txt[32];
        snprintf(sale_txt, sizeof(sale_txt), "+$%.2f SALE!", world->sale_popup_amount);
        draw_world_label_2d(world->sale_popup_pos, vp, vw_i, vh_i, 2.5f,
                            (Color4){0.06f, 0.38f, 0.12f, 0.90f},
                            (Color4){0.45f, 1.0f, 0.55f, 1.0f}, sale_txt);
    }

    /* Named NPC Speech / Status Bubbles when within 28m */
    for (int i = 0; i < 4; ++i) {
        const GrindNpc *npc = &world->npcs[i];
        float d = vec3_dist_xz(p->pos, npc->pos);
        if (d < 28.0f) {
            Vec3 head_pos = vec3_make(npc->pos.x, 2.35f, npc->pos.z);
            Color4 tag_col = (npc->envy > 0.4f)
                ? (Color4){1.0f, 0.55f, 0.55f, 1.0f}
                : (Color4){0.75f, 0.98f, 0.82f, 1.0f};
            draw_world_label_2d(head_pos, vp, vw_i, vh_i, 1.9f,
                                (Color4){0.06f, 0.08f, 0.12f, 0.82f}, tag_col,
                                d < 9.0f ? npc->last_reaction : npc->name);
        }
    }

    /* B. Top Status Bar */
    push_quad_2d(10.0f, 10.0f, vw - 20.0f, 76.0f, (Color4){0.06f, 0.08f, 0.12f, 0.84f});

    char line1[160];
    int hr = (int)world->time_of_day_hours;
    int mn = (int)((world->time_of_day_hours - (float)hr) * 60.0f);
    snprintf(line1, sizeof(line1), "DAY %d  %02d:%02d  TEMP %.0fC   CASH: $%.2f   NET WORTH: $%.0f   STATUS: %.0f",
             world->day_index, hr, mn, world->ambient_temp_c,
             p->cash, grind_calc_net_worth(world), p->perceived_wealth);
    draw_text_2d(22.0f, 20.0f, 2.8f, (Color4){0.98f, 0.95f, 0.45f, 1.0f}, line1);

    char line2[160];
    snprintf(line2, sizeof(line2), "STAMINA: %.0f%%  STRESS: %.0f%%  LOAD: %.1f/%.0fKG   LEMONADE: %d CUPS   STAFF: %d",
             p->stamina, p->stress,
             grind_player_carried_mass_kg(world), grind_player_max_mass_kg(world),
             p->stand_lemonade_stock, world->employee_count);
    draw_text_2d(22.0f, 44.0f, 2.4f, (Color4){0.82f, 0.94f, 0.98f, 1.0f}, line2);

    draw_text_2d(22.0f, 66.0f, 2.2f, (Color4){0.55f, 0.98f, 0.62f, 1.0f}, world->status_message);

    /* C. Social Circle Reaction Panel (Top-Right) */
    push_quad_2d(vw - 360.0f, 94.0f, 350.0f, 108.0f, (Color4){0.06f, 0.08f, 0.12f, 0.76f});
    draw_text_2d(vw - 348.0f, 102.0f, 2.2f, (Color4){0.95f, 0.82f, 0.35f, 1.0f}, "SOCIAL REACTIONS TO WEALTH:");
    for (int i = 0; i < 4; ++i) {
        char sline[96];
        const GrindNpc *n = &world->npcs[i];
        snprintf(sline, sizeof(sline), "%s: AFF %.1f ENV %.1f", n->name, n->affection, n->envy);
        Color4 scol = (n->envy > 0.4f) ? (Color4){0.98f, 0.45f, 0.45f, 1.0f} : (Color4){0.78f, 0.92f, 0.82f, 1.0f};
        draw_text_2d(vw - 348.0f, 122.0f + i * 19.0f, 2.0f, scol, sline);
    }

    /* D. Crosshair or Map Header */
    if (input->map_transition_t < 0.3f) {
        push_quad_2d(vw * 0.5f - 6.0f, vh * 0.5f - 1.0f, 12.0f, 2.0f, (Color4){1.0f, 1.0f, 1.0f, 0.75f});
        push_quad_2d(vw * 0.5f - 1.0f, vh * 0.5f - 6.0f, 2.0f, 12.0f, (Color4){1.0f, 1.0f, 1.0f, 0.75f});
    } else {
        draw_text_2d(vw * 0.5f - 140.0f, 98.0f, 2.8f, (Color4){0.98f, 0.92f, 0.25f, 1.0f}, "3D TACTICAL MANAGEMENT MAP");
    }

    /* E. Adaptive On-Screen Touch Controls for Small Devices / Web Mobile */
    if (input->touch_controls_active) {
        float jx = input->joy_origin_x * vw;
        float jy = input->joy_origin_y * vh;
        float jr = input->joy_radius * vh;
        push_quad_2d(jx - jr, jy - jr, jr * 2.0f, jr * 2.0f, (Color4){0.85f, 0.90f, 0.98f, 0.22f});

        float kx = input->joy_current_x * vw;
        float ky = input->joy_current_y * vh;
        float kr = jr * 0.45f;
        push_quad_2d(kx - kr, ky - kr, kr * 2.0f, kr * 2.0f, (Color4){0.25f, 0.88f, 0.98f, 0.68f});
        draw_text_2d(jx - 28.0f, jy + jr + 8.0f, 2.0f, (Color4){0.85f, 0.92f, 0.98f, 0.85f}, "MOVE");

        for (int i = 0; i < TOUCH_BTN_COUNT; ++i) {
            const GrindTouchButton *b = &input->buttons[i];
            float bx = b->cx * vw;
            float by = b->cy * vh;
            float br = b->radius * vh;
            Color4 bcol = b->pressed
                ? (Color4){0.98f, 0.85f, 0.22f, 0.85f}
                : (Color4){0.16f, 0.24f, 0.36f, 0.72f};
            push_quad_2d(bx - br, by - br, br * 2.0f, br * 2.0f, bcol);
            draw_text_2d(bx - br * 0.72f, by - 5.0f, 2.1f, (Color4){1.0f, 1.0f, 1.0f, 0.95f}, b->label);
        }
    }

    glUseProgram(r->prog_2d);
    glUniform2f(r->u_res_2d, vw, vh);
    glBindVertexArray(r->vao_2d);
    glBindBuffer(GL_ARRAY_BUFFER, r->vbo_2d);
    glBufferSubData(GL_ARRAY_BUFFER, 0, g_vert_2d_count * (int)sizeof(Vertex2D), g_verts_2d);
    glDrawArrays(GL_TRIANGLES, 0, g_vert_2d_count);

    glBindVertexArray(0);
    glDisable(GL_BLEND);
}
