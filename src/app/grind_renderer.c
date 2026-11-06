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
    "    vec3 light = u_ambient * (0.65 + 0.35 * hemi) + vec3(0.95, 0.92, 0.85) * ndl * 0.78;\n"
    "    /* Subtle street grid lines on ground plane */\n"
    "    vec3 base = u_color.rgb;\n"
    "    if (v_world_pos.y < 0.05 && n.y > 0.9) {\n"
    "        float gx = abs(fract(v_world_pos.x * 0.125) - 0.5);\n"
    "        float gz = abs(fract(v_world_pos.z * 0.125) - 0.5);\n"
    "        if (gx < 0.02 || gz < 0.02) base *= 0.88;\n"
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

/* Unit cube centered on X/Z [-0.5, 0.5] and resting on Y [0.0, 1.0] for intuitive 3D building/prop placement */
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

/* Immediate-mode 2D batch buffer for HUD panels, bars, vector font, and touch controls */
typedef struct {
    float x, y;
    float r, g, b, a;
} Vertex2D;

#define MAX_2D_VERTS 16384
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

/* Compact 3x5 bitmap font encoded in 15-bit integers for zero-asset HUD text */
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
        case ',': return 0x0024; case '&': return 0x3AA7;
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

static void draw_box_3d(const GrindRenderer *r, Mat4 vp, Vec3 pos, Vec3 size, float yaw_rad, Color4 col) {
    Mat4 m = mat4_mul(mat4_translate(pos), mat4_mul(mat4_rotate_y(yaw_rad), mat4_scale(size)));
    Mat4 mvp = mat4_mul(vp, m);
    glUniformMatrix4fv(r->u_mvp_3d, 1, GL_FALSE, mvp.m);
    glUniformMatrix4fv(r->u_model_3d, 1, GL_FALSE, m.m);
    glUniform4f(r->u_color_3d, col.r, col.g, col.b, col.a);
    glDrawArrays(GL_TRIANGLES, 0, r->cube_vertex_count);
}

void grind_renderer_draw_frame(GrindRenderer *r, const GrindWorld *world, const GrindInputState *input) {
    if (!r->initialized) return;

    glViewport(0, 0, input->viewport_w, input->viewport_h);

    /* Sky color varies by time of day and weather */
    float day_t = sinf(grind_clampf((world->time_of_day_hours - 6.0f) / 16.0f, 0.0f, 1.0f) * GRIND_PI);
    float sky_r = 0.12f + 0.36f * day_t;
    float sky_g = 0.16f + 0.52f * day_t;
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

    /* 1. Ground Plane, Central Plaza & Road Network */
    draw_box_3d(r, vp, vec3_make(0.0f, -0.5f, 0.0f), vec3_make(260.0f, 0.5f, 260.0f), 0.0f, (Color4){0.24f, 0.27f, 0.29f, 1.0f});
    draw_box_3d(r, vp, vec3_make(0.0f, -0.08f, 0.0f), vec3_make(36.0f, 0.1f, 36.0f), 0.0f, (Color4){0.62f, 0.58f, 0.48f, 1.0f});

    /* 2. City Buildings & Functional Landmarks */
    for (int i = 0; i < world->building_count; ++i) {
        const GrindBuilding *b = &world->buildings[i];
        draw_box_3d(r, vp, b->pos, b->size, 0.0f, b->color);
        /* Roof accent */
        Vec3 roof_pos = vec3_make(b->pos.x, b->size.y, b->pos.z);
        Vec3 roof_sz = vec3_make(b->size.x * 0.85f, 0.8f, b->size.z * 0.85f);
        draw_box_3d(r, vp, roof_pos, roof_sz, 0.0f, (Color4){b->color.r * 0.7f, b->color.g * 0.7f, b->color.b * 0.7f, 1.0f});
    }

    /* 3. Found Money on Ground (Glowing Green Cash Bundle) */
    if (world->ground_money_available) {
        draw_box_3d(r, vp, world->ground_money_pos, vec3_make(1.2f, 0.35f, 0.7f), 0.4f, (Color4){0.15f, 0.92f, 0.32f, 1.0f});
        draw_box_3d(r, vp, vec3_make(world->ground_money_pos.x, 0.35f, world->ground_money_pos.z),
                    vec3_make(0.3f, 2.2f, 0.3f), 0.0f, (Color4){0.95f, 0.92f, 0.20f, 1.0f});
    }

    /* 4. Deployed Lemonade Stand or Flagship Storefront */
    const GrindPlayer *p = &world->player;
    if (p->stand_deployed) {
        Vec3 sp = p->stand_pos;
        if (p->owns_storefront) {
            draw_box_3d(r, vp, sp, vec3_make(5.5f, 4.2f, 4.5f), 0.0f, (Color4){0.95f, 0.82f, 0.22f, 1.0f});
        } else {
            /* Table + Yellow Lemonade Pitcher + Umbrella */
            draw_box_3d(r, vp, sp, vec3_make(2.4f, 1.1f, 1.4f), 0.0f, (Color4){0.65f, 0.42f, 0.22f, 1.0f});
            draw_box_3d(r, vp, vec3_make(sp.x, 1.1f, sp.z), vec3_make(0.55f, 0.65f, 0.55f), 0.0f, (Color4){0.98f, 0.92f, 0.15f, 1.0f});
            draw_box_3d(r, vp, vec3_make(sp.x, 2.5f, sp.z), vec3_make(3.0f, 0.35f, 3.0f), 0.0f, (Color4){0.95f, 0.28f, 0.22f, 1.0f});
        }
    }

    /* 5. Active Courier Parcels & Dropoff Beacons */
    for (int i = 0; i < GRIND_MAX_CONTRACTS; ++i) {
        const GrindCourierContract *c = &world->contracts[i];
        if (!c->active) continue;
        if (!c->picked_up) {
            draw_box_3d(r, vp, c->pickup_pos, vec3_make(1.1f, 1.1f, 1.1f), 0.2f, (Color4){0.85f, 0.58f, 0.25f, 1.0f});
        } else if (c->claimed_by_player) {
            /* Glowing dropoff beacon */
            draw_box_3d(r, vp, c->dropoff_pos, vec3_make(1.5f, 6.0f, 1.5f), 0.0f, (Color4){0.20f, 0.95f, 0.88f, 1.0f});
        }
    }

    /* 6. Hired Employees Executing Task Graphs */
    for (int i = 0; i < world->employee_count; ++i) {
        const GrindEmployee *e = &world->employees[i];
        if (!e->hired) continue;
        draw_box_3d(r, vp, e->pos, vec3_make(0.8f, 1.75f, 0.8f), 0.0f, (Color4){0.18f, 0.82f, 0.95f, 1.0f});
    }

    /* 7. NPCs (Color-coded by social reaction: Green=Friendly/Respect, Red=Envious, Gold=Opportunist, Blue=Citizen) */
    for (int i = 0; i < world->npc_count; ++i) {
        const GrindNpc *npc = &world->npcs[i];
        Color4 body_col = {0.35f, 0.55f, 0.78f, 1.0f};
        if (npc->archetype == ARCHETYPE_LOYAL_FRIEND) body_col = (Color4){0.22f, 0.82f, 0.38f, 1.0f};
        else if (npc->archetype == ARCHETYPE_ENVIOUS_PEER) {
            body_col = (npc->envy > 0.35f) ? (Color4){0.92f, 0.22f, 0.22f, 1.0f} : (Color4){0.68f, 0.52f, 0.32f, 1.0f};
        } else if (npc->archetype == ARCHETYPE_OPPORTUNIST) {
            body_col = (npc->affection > 0.3f) ? (Color4){0.95f, 0.78f, 0.18f, 1.0f} : (Color4){0.55f, 0.45f, 0.62f, 1.0f};
        } else if (npc->archetype == ARCHETYPE_BANKER) {
            body_col = (Color4){0.22f, 0.26f, 0.35f, 1.0f};
        }
        draw_box_3d(r, vp, npc->pos, vec3_make(0.75f, 1.45f, 0.75f), 0.0f, body_col);
        draw_box_3d(r, vp, vec3_make(npc->pos.x, 1.48f, npc->pos.z), vec3_make(0.50f, 0.45f, 0.50f), 0.0f, (Color4){0.92f, 0.76f, 0.64f, 1.0f});
    }

    /* 8. Player Marker (in Top-Down Map View) or First-Person Viewmodel (Handlebars / Satchel / Suit / Watch) */
    if (input->map_transition_t > 0.15f) {
        draw_box_3d(r, vp, p->pos, vec3_make(2.2f, 3.5f, 2.2f), p->yaw_rad, (Color4){0.98f, 0.95f, 0.20f, 1.0f});
    } else {
        /* First-Person Viewmodel in front of player camera */
        float fx = sinf(p->yaw_rad);
        float fz = -cosf(p->yaw_rad);
        float rx = cosf(p->yaw_rad);
        float rz = sinf(p->yaw_rad);

        /* Right hand / sleeve showing clothing & luxury watch */
        Color4 sleeve_col = {0.45f, 0.45f, 0.48f, 1.0f};
        if (p->apparel == APPAREL_COURIER_GEAR) sleeve_col = (Color4){0.92f, 0.55f, 0.15f, 1.0f};
        else if (p->apparel == APPAREL_TAILORED_SUIT) sleeve_col = (Color4){0.15f, 0.18f, 0.28f, 1.0f};

        Vec3 rh_pos = vec3_make(p->pos.x + fx * 0.65f + rx * 0.35f, 1.25f, p->pos.z + fz * 0.65f + rz * 0.35f);
        draw_box_3d(r, vp, rh_pos, vec3_make(0.16f, 0.18f, 0.38f), p->yaw_rad, sleeve_col);
        if (p->has_luxury_watch) {
            Vec3 w_pos = vec3_make(rh_pos.x, 1.43f, rh_pos.z);
            draw_box_3d(r, vp, w_pos, vec3_make(0.18f, 0.05f, 0.08f), p->yaw_rad, (Color4){0.98f, 0.84f, 0.18f, 1.0f});
        }

        /* Vehicle handlebars / dashboard in First-Person */
        if (p->vehicle == VEHICLE_BICYCLE || p->vehicle == VEHICLE_CARGO_EBIKE) {
            Vec3 bar_pos = vec3_make(p->pos.x + fx * 0.72f, 1.22f, p->pos.z + fz * 0.72f);
            draw_box_3d(r, vp, bar_pos, vec3_make(0.75f, 0.06f, 0.08f), p->yaw_rad, (Color4){0.20f, 0.72f, 0.90f, 1.0f});
        } else if (p->vehicle == VEHICLE_SPORTS_CAR) {
            Vec3 hood_pos = vec3_make(p->pos.x + fx * 1.1f, 1.05f, p->pos.z + fz * 1.1f);
            draw_box_3d(r, vp, hood_pos, vec3_make(1.4f, 0.18f, 0.9f), p->yaw_rad, (Color4){0.92f, 0.15f, 0.18f, 1.0f});
        }
    }

    /* ================= 2D HUD & ADAPTIVE ON-SCREEN TOUCH CONTROLS ================= */
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    g_vert_2d_count = 0;
    float vw = (float)input->viewport_w;
    float vh = (float)input->viewport_h;

    /* Top Status Bar */
    push_quad_2d(10.0f, 10.0f, vw - 20.0f, 76.0f, (Color4){0.06f, 0.08f, 0.12f, 0.82f});

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

    /* Social Circle Reaction Panel (Top-Right) */
    push_quad_2d(vw - 360.0f, 94.0f, 350.0f, 108.0f, (Color4){0.06f, 0.08f, 0.12f, 0.76f});
    draw_text_2d(vw - 348.0f, 102.0f, 2.2f, (Color4){0.95f, 0.82f, 0.35f, 1.0f}, "SOCIAL REACTIONS TO WEALTH:");
    for (int i = 0; i < 4; ++i) {
        char sline[96];
        const GrindNpc *n = &world->npcs[i];
        snprintf(sline, sizeof(sline), "%s: AFF %.1f ENV %.1f", n->name, n->affection, n->envy);
        Color4 scol = (n->envy > 0.4f) ? (Color4){0.98f, 0.45f, 0.45f, 1.0f} : (Color4){0.78f, 0.92f, 0.82f, 1.0f};
        draw_text_2d(vw - 348.0f, 122.0f + i * 19.0f, 2.0f, scol, sline);
    }

    /* First-Person Crosshair */
    if (input->map_transition_t < 0.3f) {
        push_quad_2d(vw * 0.5f - 6.0f, vh * 0.5f - 1.0f, 12.0f, 2.0f, (Color4){1.0f, 1.0f, 1.0f, 0.75f});
        push_quad_2d(vw * 0.5f - 1.0f, vh * 0.5f - 6.0f, 2.0f, 12.0f, (Color4){1.0f, 1.0f, 1.0f, 0.75f});
    } else {
        draw_text_2d(vw * 0.5f - 140.0f, 98.0f, 2.8f, (Color4){0.98f, 0.92f, 0.25f, 1.0f}, "3D TACTICAL MANAGEMENT MAP");
    }

    /* Adaptive On-Screen Touch Controls for Small Devices / Web Mobile */
    if (input->touch_controls_active) {
        /* Left Virtual Thumbstick Base + Knob */
        float jx = input->joy_origin_x * vw;
        float jy = input->joy_origin_y * vh;
        float jr = input->joy_radius * vh;
        push_quad_2d(jx - jr, jy - jr, jr * 2.0f, jr * 2.0f, (Color4){0.85f, 0.90f, 0.98f, 0.22f});

        float kx = input->joy_current_x * vw;
        float ky = input->joy_current_y * vh;
        float kr = jr * 0.45f;
        push_quad_2d(kx - kr, ky - kr, kr * 2.0f, kr * 2.0f, (Color4){0.25f, 0.88f, 0.98f, 0.68f});
        draw_text_2d(jx - 28.0f, jy + jr + 8.0f, 2.0f, (Color4){0.85f, 0.92f, 0.98f, 0.85f}, "MOVE");

        /* Right-Side Action Buttons */
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
