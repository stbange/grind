#include "grind_app/grind_renderer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#include <emscripten.h>
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
    "layout(location = 2) in vec2 a_uv;\n"
    "uniform mat4 u_mvp;\n"
    "uniform mat4 u_model;\n"
    "uniform vec4 u_uv_rect;\n"
    "out vec3 v_norm;\n"
    "out vec3 v_world_pos;\n"
    "out vec2 v_uv;\n"
    "void main() {\n"
    "    vec4 wp = u_model * vec4(a_pos, 1.0);\n"
    "    v_world_pos = wp.xyz;\n"
    "    v_norm = mat3(u_model) * a_norm;\n"
    "    v_uv = u_uv_rect.xy + fract(a_uv) * u_uv_rect.zw;\n"
    "    gl_Position = u_mvp * vec4(a_pos, 1.0);\n"
    "}\n";

static const char *k_fs_3d =
    SHADER_HEADER
    "in vec3 v_norm;\n"
    "in vec3 v_world_pos;\n"
    "in vec2 v_uv;\n"
    "uniform vec4 u_color;\n"
    "uniform vec3 u_light_dir;\n"
    "uniform vec3 u_ambient;\n"
    "uniform int u_tex_mode;\n"
    "uniform sampler2D u_tex;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    vec3 n = normalize(v_norm);\n"
    "    float ndl = max(dot(n, normalize(u_light_dir)), 0.0);\n"
    "    float hemi = 0.5 + 0.5 * n.y;\n"
    "    vec3 light = u_ambient * (0.68 + 0.32 * hemi) + vec3(0.96, 0.93, 0.86) * ndl * 0.78;\n"
    "    vec3 tex_c = texture(u_tex, v_uv).rgb;\n"
    "    vec3 base = u_color.rgb;\n"
    "    if (u_tex_mode == 1) {\n"
    "        base = mix(base, tex_c * base * 1.35, 0.88);\n"
    "    } else if (u_tex_mode == 2) {\n"
    "        if (tex_c.r > 0.65 && tex_c.b > 0.65 && tex_c.g < 0.38 && (tex_c.r + tex_c.b - tex_c.g * 2.0) > 0.80) {\n"
    "            discard;\n"
    "        }\n"
    "        frag_color = vec4(tex_c * (0.72 + 0.28 * light), 1.0);\n"
    "        return;\n"
    "    }\n"
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

/* Unit cube centered on X/Z [-0.5, 0.5] and resting on Y [0.0, 1.0], with UV coordinates (8 floats/vert) */
static const float k_cube_verts[] = {
    /* Front (+Z) */
    -0.5f,0.0f, 0.5f,  0,0,1,  0.02f,0.98f,   0.5f,0.0f, 0.5f,  0,0,1,  0.98f,0.98f,   0.5f,1.0f, 0.5f,  0,0,1,  0.98f,0.02f,
    -0.5f,0.0f, 0.5f,  0,0,1,  0.02f,0.98f,   0.5f,1.0f, 0.5f,  0,0,1,  0.98f,0.02f,  -0.5f,1.0f, 0.5f,  0,0,1,  0.02f,0.02f,
    /* Back (-Z) */
     0.5f,0.0f,-0.5f,  0,0,-1, 0.02f,0.98f,  -0.5f,0.0f,-0.5f,  0,0,-1, 0.98f,0.98f,  -0.5f,1.0f,-0.5f,  0,0,-1, 0.98f,0.02f,
     0.5f,0.0f,-0.5f,  0,0,-1, 0.02f,0.98f,  -0.5f,1.0f,-0.5f,  0,0,-1, 0.98f,0.02f,   0.5f,1.0f,-0.5f,  0,0,-1, 0.02f,0.02f,
    /* Left (-X) */
    -0.5f,0.0f,-0.5f, -1,0,0,  0.02f,0.98f,  -0.5f,0.0f, 0.5f, -1,0,0,  0.98f,0.98f,  -0.5f,1.0f, 0.5f, -1,0,0,  0.98f,0.02f,
    -0.5f,0.0f,-0.5f, -1,0,0,  0.02f,0.98f,  -0.5f,1.0f, 0.5f, -1,0,0,  0.98f,0.02f,  -0.5f,1.0f,-0.5f, -1,0,0,  0.02f,0.02f,
    /* Right (+X) */
     0.5f,0.0f, 0.5f,  1,0,0,  0.02f,0.98f,   0.5f,0.0f,-0.5f,  1,0,0,  0.98f,0.98f,   0.5f,1.0f,-0.5f,  1,0,0,  0.98f,0.02f,
     0.5f,0.0f, 0.5f,  1,0,0,  0.02f,0.98f,   0.5f,1.0f,-0.5f,  1,0,0,  0.98f,0.02f,   0.5f,1.0f, 0.5f,  1,0,0,  0.02f,0.02f,
    /* Top (+Y) */
    -0.5f,1.0f, 0.5f,  0,1,0,  0.02f,0.98f,   0.5f,1.0f, 0.5f,  0,1,0,  0.98f,0.98f,   0.5f,1.0f,-0.5f,  0,1,0,  0.98f,0.02f,
    -0.5f,1.0f, 0.5f,  0,1,0,  0.02f,0.98f,   0.5f,1.0f,-0.5f,  0,1,0,  0.98f,0.02f,  -0.5f,1.0f,-0.5f,  0,1,0,  0.02f,0.02f,
    /* Bottom (-Y) */
    -0.5f,0.0f,-0.5f,  0,-1,0, 0.02f,0.98f,   0.5f,0.0f,-0.5f,  0,-1,0, 0.98f,0.98f,   0.5f,0.0f, 0.5f,  0,-1,0, 0.98f,0.02f,
    -0.5f,0.0f,-0.5f,  0,-1,0, 0.02f,0.98f,   0.5f,0.0f, 0.5f,  0,-1,0, 0.98f,0.02f,  -0.5f,0.0f, 0.5f,  0,-1,0, 0.02f,0.02f
};

/* Vertical 3D Billboard Quad on X [-0.5, 0.5], Y [0.0, 1.0], Z = 0.0 */
static const float k_quad_verts[] = {
    -0.5f, 0.0f, 0.0f,  0,0,1,  0.03f, 0.97f,
     0.5f, 0.0f, 0.0f,  0,0,1,  0.97f, 0.97f,
     0.5f, 1.0f, 0.0f,  0,0,1,  0.97f, 0.03f,
    -0.5f, 0.0f, 0.0f,  0,0,1,  0.03f, 0.97f,
     0.5f, 1.0f, 0.0f,  0,0,1,  0.97f, 0.03f,
    -0.5f, 1.0f, 0.0f,  0,0,1,  0.03f, 0.03f
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

/* Static BSS buffer so procedural texture generation uses zero WASM/C stack space */
static uint8_t s_atlas_pix[256 * 256 * 4];

/* Generates a rich procedural 4x4 fallback texture atlas in C (used on native Linux/Windows or while web images load) */
static unsigned int create_procedural_city_atlas(void) {
    enum { W = 256, H = 256, T = 64 };
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int tx = x / T;
            int ty = y / T;
            int lx = x % T;
            int ly = y % T;
            int tile = ty * 4 + tx;
            uint8_t r = 180, g = 180, b = 185;
            if (tile < 4) {
                /* Building facade tiles with procedural window grids */
                bool win = (lx % 16 >= 4 && lx % 16 <= 11 && ly % 16 >= 4 && ly % 16 <= 12);
                if (tile == 0) { r = 175; g = 88; b = 72; }
                else if (tile == 1) { r = 95; g = 140; b = 185; }
                else if (tile == 2) { r = 205; g = 185; b = 140; }
                else { r = 145; g = 150; b = 158; }
                if (win) { r = 55; g = 85; b = 120; }
            } else if (tile == 4) {
                /* Asphalt road */
                uint8_t n = (uint8_t)(((x * 37 + y * 13) & 15));
                r = 52 + n; g = 55 + n; b = 62 + n;
                if (lx >= 29 && lx <= 34 && (ly % 16 < 10)) { r = 235; g = 210; b = 55; }
            } else if (tile == 5) {
                /* Sidewalk pavers */
                bool seam = (lx % 16 == 0 || ly % 16 == 0);
                r = seam ? 120 : 175; g = seam ? 122 : 177; b = seam ? 126 : 180;
            } else if (tile == 6) {
                /* Park grass & cobblestones */
                uint8_t n = (uint8_t)(((x * 19 + y * 53) & 31));
                r = 65 + n / 2; g = 145 + n; b = 65 + n / 2;
            } else if (tile == 8) {
                /* Wood crate planks */
                r = (ly % 8 == 0) ? 105 : 168; g = (ly % 8 == 0) ? 70 : 118; b = 62;
            } else if (tile == 11) {
                /* Turquoise swimming pool ripples */
                uint8_t wv = (uint8_t)(((lx ^ ly) & 15) * 4);
                r = 35 + wv; g = 200 + wv / 2; b = 235;
            }
            int idx = (y * W + x) * 4;
            s_atlas_pix[idx + 0] = r;
            s_atlas_pix[idx + 1] = g;
            s_atlas_pix[idx + 2] = b;
            s_atlas_pix[idx + 3] = 255;
        }
    }
    unsigned int tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, s_atlas_pix);
    glGenerateMipmap(GL_TEXTURE_2D);
    return tex;
}

/* Generates a procedural 4x4 chroma-keyed sprite sheet (magenta background) for native or pre-load fallback */
static unsigned int create_procedural_sprites_atlas(void) {
    enum { W = 256, H = 256, T = 64 };
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int tx = x / T;
            int ty = y / T;
            int lx = x % T;
            int ly = y % T;
            int tile = ty * 4 + tx;
            /* Pure magenta chroma-key transparent background */
            uint8_t r = 255, g = 0, b = 255;
            int dx = lx - 32;
            int dy = ly - 32;
            if (tile < 8) {
                /* Character sprite silhouette (head + torso) */
                if (dx * dx + (ly - 16) * (ly - 16) <= 90) {
                    r = 235; g = 190; b = 155;
                } else if (abs(dx) <= 10 && ly >= 25 && ly <= 46) {
                    r = (uint8_t)(60 + (tile * 45) % 180);
                    g = (uint8_t)(120 + (tile * 70) % 120);
                    b = (uint8_t)(210 - (tile * 30) % 140);
                }
            } else if (tile == 14) {
                /* Tree foliage canopy + trunk */
                if (dx * dx + (ly - 24) * (ly - 24) <= 360) {
                    r = 55; g = 168; b = 65;
                } else if (abs(dx) <= 4 && ly >= 36 && ly <= 58) {
                    r = 125; g = 85; b = 48;
                }
            } else if (tile == 15) {
                /* Streetlamp post + glowing lantern */
                if (abs(dx) <= 2 && ly >= 14 && ly <= 58) {
                    r = 60; g = 65; b = 72;
                } else if (dx * dx + (ly - 14) * (ly - 14) <= 36) {
                    r = 250; g = 230; b = 95;
                }
            } else if (dx * dx + dy * dy <= 196) {
                /* Props (lemonade umbrella, cash stack, parcel, bicycle) */
                r = 240; g = 205; b = 65;
            }
            int idx = (y * W + x) * 4;
            s_atlas_pix[idx + 0] = r;
            s_atlas_pix[idx + 1] = g;
            s_atlas_pix[idx + 2] = b;
            s_atlas_pix[idx + 3] = 255;
        }
    }
    unsigned int tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, s_atlas_pix);
    glGenerateMipmap(GL_TEXTURE_2D);
    return tex;
}

#ifdef __EMSCRIPTEN__
/* On Web (GitHub Pages), asynchronously load the Nano Banana generated atlases resampled to 1024x1024 POT */
EM_JS(void, grind_web_load_atlas_Async, (unsigned int tex_id, const char *url_ptr, int linear_filter), {
    try {
        var url = UTF8ToString(url_ptr);
        var img = new Image();
        img.crossOrigin = "anonymous";
        img.onload = function() {
            try {
                var gl = (typeof GLctx !== "undefined" && GLctx) ? GLctx : Module.ctx;
                if (!gl || typeof GL === "undefined" || !GL.textures) return;
                var tex = GL.textures[tex_id];
                if (!tex) return;
                var pot = document.createElement("canvas");
                pot.width = 1024;
                pot.height = 1024;
                var ctx2d = pot.getContext("2d");
                ctx2d.drawImage(img, 0, 0, 1024, 1024);
                gl.bindTexture(gl.TEXTURE_2D, tex);
                gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, false);
                gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, pot);
                gl.generateMipmap(gl.TEXTURE_2D);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, linear_filter ? gl.LINEAR_MIPMAP_LINEAR : gl.LINEAR);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
            } catch (e) {
                console.warn("WebGL atlas upload warning:", e);
            }
        };
        img.src = url;
    } catch (e) {
        console.warn("WebGL atlas fetch warning:", e);
    }
})
#endif

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
    r->u_uv_rect_3d = glGetUniformLocation(r->prog_3d, "u_uv_rect");
    r->u_tex_mode_3d = glGetUniformLocation(r->prog_3d, "u_tex_mode");
    r->u_tex_3d = glGetUniformLocation(r->prog_3d, "u_tex");
    r->u_res_2d = glGetUniformLocation(r->prog_2d, "u_res");

    r->cube_vertex_count = 36;
    glGenVertexArrays(1, &r->vao_cube);
    glGenBuffers(1, &r->vbo_cube);
    glBindVertexArray(r->vao_cube);
    glBindBuffer(GL_ARRAY_BUFFER, r->vbo_cube);
    glBufferData(GL_ARRAY_BUFFER, sizeof(k_cube_verts), k_cube_verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));

    glGenVertexArrays(1, &r->vao_quad);
    glGenBuffers(1, &r->vbo_quad);
    glBindVertexArray(r->vao_quad);
    glBindBuffer(GL_ARRAY_BUFFER, r->vbo_quad);
    glBufferData(GL_ARRAY_BUFFER, sizeof(k_quad_verts), k_quad_verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));

    glGenVertexArrays(1, &r->vao_2d);
    glGenBuffers(1, &r->vbo_2d);
    glBindVertexArray(r->vao_2d);
    glBindBuffer(GL_ARRAY_BUFFER, r->vbo_2d);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_verts_2d), NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void *)(2 * sizeof(float)));

    r->tex_city = create_procedural_city_atlas();
    r->tex_sprites = create_procedural_sprites_atlas();

#ifdef __EMSCRIPTEN__
    grind_web_load_atlas_Async(r->tex_city, "assets/textures/city_atlas.jpg", 1);
    grind_web_load_atlas_Async(r->tex_sprites, "assets/textures/sprites_atlas.jpg", 0);
#endif

    glBindVertexArray(0);
    r->initialized = true;
    return true;
}

void grind_renderer_shutdown(GrindRenderer *r) {
    if (!r->initialized) return;
    glDeleteTextures(1, &r->tex_city);
    glDeleteTextures(1, &r->tex_sprites);
    glDeleteBuffers(1, &r->vbo_cube);
    glDeleteVertexArrays(1, &r->vao_cube);
    glDeleteBuffers(1, &r->vbo_quad);
    glDeleteVertexArrays(1, &r->vao_quad);
    glDeleteBuffers(1, &r->vbo_2d);
    glDeleteVertexArrays(1, &r->vao_2d);
    glDeleteProgram(r->prog_3d);
    glDeleteProgram(r->prog_2d);
    r->initialized = false;
}

/* Sets the active 4x4 atlas tile [0..15] and texture mode (0=solid, 1=city_atlas, 2=sprites_atlas cutout) */
static void set_atlas_tile(const GrindRenderer *r, int mode, int tile_idx) {
    glUniform1i(r->u_tex_mode_3d, mode);
    if (mode > 0) {
        int col = tile_idx & 3;
        int row = (tile_idx >> 2) & 3;
        glUniform4f(r->u_uv_rect_3d, (float)col * 0.25f + 0.014f, (float)row * 0.25f + 0.014f, 0.222f, 0.222f);
    } else {
        glUniform4f(r->u_uv_rect_3d, 0.0f, 0.0f, 1.0f, 1.0f);
    }
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

static void draw_textured_box_3d(const GrindRenderer *r, Mat4 vp, Vec3 pos, Vec3 size, float yaw_rad, Color4 col, int tile_idx) {
    set_atlas_tile(r, 1, tile_idx);
    draw_box_3d(r, vp, pos, size, yaw_rad, col);
    set_atlas_tile(r, 0, 0);
}

/* Renders a camera-facing 3D billboard sprite from the 4x4 Nano Banana sprites_atlas.jpg */
static void draw_sprite_billboard_3d(const GrindRenderer *r, Mat4 vp, Vec3 pos, Vec2 size, float cam_yaw, int sprite_tile) {
    glBindVertexArray(r->vao_quad);
    glBindTexture(GL_TEXTURE_2D, r->tex_sprites);
    set_atlas_tile(r, 2, sprite_tile);

    Mat4 m = mat4_mul(mat4_translate(pos), mat4_mul(mat4_rotate_y(cam_yaw), mat4_scale(vec3_make(size.x, size.y, 1.0f))));
    Mat4 mvp = mat4_mul(vp, m);
    glUniformMatrix4fv(r->u_mvp_3d, 1, GL_FALSE, mvp.m);
    glUniformMatrix4fv(r->u_model_3d, 1, GL_FALSE, m.m);
    glUniform4f(r->u_color_3d, 1.0f, 1.0f, 1.0f, 1.0f);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    set_atlas_tile(r, 0, 0);
    glBindTexture(GL_TEXTURE_2D, r->tex_city);
    glBindVertexArray(r->vao_cube);
}

static void draw_child_box(const GrindRenderer *r, Mat4 vp, Mat4 parent, Vec3 local_pos, Vec3 rot_xyz, Vec3 size, Color4 col) {
    Mat4 m = mat4_mul(parent, mat4_translate(local_pos));
    if (fabsf(rot_xyz.y) > 1e-4f) m = mat4_mul(m, mat4_rotate_y(rot_xyz.y));
    if (fabsf(rot_xyz.x) > 1e-4f) m = mat4_mul(m, mat4_rotate_x(rot_xyz.x));
    if (fabsf(rot_xyz.z) > 1e-4f) m = mat4_mul(m, mat4_rotate_z(rot_xyz.z));
    m = mat4_mul(m, mat4_scale(size));
    draw_mat_3d(r, vp, m, col);
}

static void draw_tree_3d(const GrindRenderer *r, Mat4 vp, Vec3 pos, float scale, float cam_yaw) {
    draw_textured_box_3d(r, vp, pos, vec3_make(0.50f * scale, 2.0f * scale, 0.50f * scale), 0.0f, (Color4){0.62f, 0.45f, 0.28f, 1.0f}, 15);
    draw_textured_box_3d(r, vp, vec3_make(pos.x, pos.y + 1.8f * scale, pos.z),
                         vec3_make(2.4f * scale, 1.8f * scale, 2.4f * scale), 0.25f, (Color4){0.35f, 0.82f, 0.42f, 1.0f}, 15);
    /* Overlay Nano Banana tree sprite billboard at center for rich foliage silhouette */
    draw_sprite_billboard_3d(r, vp, pos, (Vec2){3.2f * scale, 3.8f * scale}, cam_yaw, 14);
}

static void draw_streetlamp_3d(const GrindRenderer *r, Mat4 vp, Vec3 pos, bool evening, float cam_yaw) {
    draw_box_3d(r, vp, pos, vec3_make(0.20f, 4.6f, 0.20f), 0.0f, (Color4){0.22f, 0.25f, 0.28f, 1.0f});
    Color4 bulb = evening ? (Color4){1.0f, 0.92f, 0.45f, 1.0f} : (Color4){0.75f, 0.80f, 0.85f, 1.0f};
    draw_box_3d(r, vp, vec3_make(pos.x, 4.35f, pos.z), vec3_make(0.45f, 0.35f, 0.45f), 0.0f, bulb);
    draw_sprite_billboard_3d(r, vp, pos, (Vec2){1.8f, 4.8f}, cam_yaw, 15);
}

static void draw_detailed_building(const GrindRenderer *r, Mat4 vp, const GrindBuilding *b, bool evening) {
    /* Concrete sidewalk base (Tile 5) */
    draw_textured_box_3d(r, vp, vec3_make(b->pos.x, 0.0f, b->pos.z),
                         vec3_make(b->size.x + 2.6f, 0.22f, b->size.z + 2.6f), 0.0f,
                         (Color4){0.85f, 0.86f, 0.88f, 1.0f}, 5);

    /* Main building body textured with its assigned Nano Banana facade tile (0..3) */
    draw_textured_box_3d(r, vp, b->pos, b->size, 0.0f, b->color, b->tex_tile);

    /* Roof gravel & AC unit (Tile 7) */
    draw_textured_box_3d(r, vp, vec3_make(b->pos.x, b->size.y, b->pos.z),
                         vec3_make(b->size.x + 0.4f, 0.40f, b->size.z + 0.4f), 0.0f,
                         (Color4){0.65f, 0.65f, 0.68f, 1.0f}, 7);

    if (b->size.y < 5.0f) return;

    /* Entrance double doors & shop awning on +Z front face */
    float front_z = b->pos.z + b->size.z * 0.5f + 0.08f;
    draw_box_3d(r, vp, vec3_make(b->pos.x, 0.2f, front_z),
                vec3_make(2.4f, 2.6f, 0.2f), 0.0f, (Color4){0.22f, 0.16f, 0.12f, 1.0f});
    draw_box_3d(r, vp, vec3_make(b->pos.x, 2.9f, front_z + 0.55f),
                vec3_make(b->size.x * 0.65f, 0.42f, 1.3f), 0.0f,
                (Color4){fminf(1.0f, b->color.r * 1.2f), b->color.g * 0.85f, b->color.b * 0.85f, 1.0f});

    if (evening) {
        /* Warm glowing windows at night */
        draw_box_3d(r, vp, vec3_make(b->pos.x, b->size.y * 0.55f, front_z),
                    vec3_make(b->size.x * 0.5f, 1.8f, 0.12f), 0.0f, (Color4){0.98f, 0.86f, 0.42f, 1.0f});
    }
}

static void draw_articulated_human(
    const GrindRenderer *r, Mat4 vp, Vec3 pos, float yaw_rad, float walk_phase,
    Color4 shirt_col, Color4 pants_col, bool holding_cup, bool holding_parcel, bool envy_aura,
    float cam_yaw, int sprite_tile
) {
    /* Render Nano Banana character sprite billboard facing camera */
    draw_sprite_billboard_3d(r, vp, vec3_make(pos.x, pos.y + 0.02f, pos.z), (Vec2){1.45f, 2.05f}, cam_yaw, sprite_tile);

    /* Plus 3D articulated limbs/props for depth & shadow */
    Mat4 root = mat4_mul(mat4_translate(pos), mat4_rotate_y(yaw_rad));
    float swing = sinf(walk_phase) * 0.55f;
    draw_child_box(r, vp, root, vec3_make(-0.14f, 0.0f, sinf(swing) * 0.16f),
                   vec3_make(swing * 0.5f, 0.0f, 0.0f), vec3_make(0.18f, 0.70f, 0.18f), pants_col);
    draw_child_box(r, vp, root, vec3_make( 0.14f, 0.0f, -sinf(swing) * 0.16f),
                   vec3_make(-swing * 0.5f, 0.0f, 0.0f), vec3_make(0.18f, 0.70f, 0.18f), pants_col);
    draw_child_box(r, vp, root, vec3_make(0.0f, 0.70f, -0.05f),
                   vec3_make(0.0f, 0.0f, 0.0f), vec3_make(0.42f, 0.68f, 0.22f), shirt_col);

    if (holding_cup) {
        draw_child_box(r, vp, root, vec3_make(0.35f, 1.12f, -0.32f),
                       vec3_make(0.0f, 0.0f, 0.0f), vec3_make(0.16f, 0.24f, 0.16f),
                       (Color4){0.98f, 0.92f, 0.18f, 1.0f});
    }
    if (holding_parcel) {
        draw_child_box(r, vp, root, vec3_make(0.0f, 0.85f, 0.24f),
                       vec3_make(0.0f, 0.0f, 0.0f), vec3_make(0.45f, 0.48f, 0.25f),
                       (Color4){0.85f, 0.55f, 0.22f, 1.0f});
    }
    if (envy_aura) {
        draw_child_box(r, vp, root, vec3_make(0.0f, 2.15f, 0.0f),
                       vec3_make(0.0f, walk_phase, 0.0f), vec3_make(0.28f, 0.12f, 0.28f),
                       (Color4){0.95f, 0.20f, 0.20f, 1.0f});
    }
}

static void draw_bicycle_3d(const GrindRenderer *r, Mat4 vp, Vec3 pos, float yaw_rad, Color4 frame_col, float cam_yaw) {
    Mat4 root = mat4_mul(mat4_translate(pos), mat4_rotate_y(yaw_rad));
    Color4 tire_col = {0.14f, 0.15f, 0.17f, 1.0f};
    draw_child_box(r, vp, root, vec3_make(0.0f, 0.0f, -0.65f), vec3_make(0,0,0), vec3_make(0.12f, 0.65f, 0.65f), tire_col);
    draw_child_box(r, vp, root, vec3_make(0.0f, 0.0f,  0.65f), vec3_make(0,0,0), vec3_make(0.12f, 0.65f, 0.65f), tire_col);
    draw_child_box(r, vp, root, vec3_make(0.0f, 0.55f, 0.0f), vec3_make(0,0,0), vec3_make(0.10f, 0.14f, 1.25f), frame_col);
    draw_child_box(r, vp, root, vec3_make(0.0f, 1.15f, -0.60f), vec3_make(0,0,0), vec3_make(0.75f, 0.08f, 0.10f), tire_col);
    draw_sprite_billboard_3d(r, vp, pos, (Vec2){1.8f, 1.4f}, cam_yaw, 12);
}

static void draw_sports_car_3d(const GrindRenderer *r, Mat4 vp, Vec3 pos, float yaw_rad) {
    Mat4 root = mat4_mul(mat4_translate(pos), mat4_rotate_y(yaw_rad));
    Color4 red_paint = {0.95f, 0.22f, 0.25f, 1.0f};
    Color4 glass_col = {0.16f, 0.22f, 0.32f, 1.0f};
    Color4 tire_col  = {0.12f, 0.13f, 0.15f, 1.0f};
    draw_child_box(r, vp, root, vec3_make(-0.95f, 0.0f, -1.25f), vec3_make(0,0,0), vec3_make(0.32f, 0.65f, 0.65f), tire_col);
    draw_child_box(r, vp, root, vec3_make( 0.95f, 0.0f, -1.25f), vec3_make(0,0,0), vec3_make(0.32f, 0.65f, 0.65f), tire_col);
    draw_child_box(r, vp, root, vec3_make(-0.95f, 0.0f,  1.25f), vec3_make(0,0,0), vec3_make(0.32f, 0.65f, 0.65f), tire_col);
    draw_child_box(r, vp, root, vec3_make( 0.95f, 0.0f,  1.25f), vec3_make(0,0,0), vec3_make(0.32f, 0.65f, 0.65f), tire_col);
    set_atlas_tile(r, 1, 12);
    draw_child_box(r, vp, root, vec3_make(0.0f, 0.22f, 0.0f), vec3_make(0,0,0), vec3_make(1.95f, 0.58f, 3.8f), red_paint);
    set_atlas_tile(r, 0, 0);
    draw_child_box(r, vp, root, vec3_make(0.0f, 0.78f, 0.15f), vec3_make(0,0,0), vec3_make(1.55f, 0.48f, 1.85f), glass_col);
    draw_child_box(r, vp, root, vec3_make(0.0f, 0.92f, 1.72f), vec3_make(0,0,0), vec3_make(1.85f, 0.08f, 0.35f), tire_col);
}

static void draw_lifestyle_courtyard(const GrindRenderer *r, Mat4 vp, const GrindPlayer *p) {
    Vec3 terrace = vec3_make(45.0f, 0.0f, 39.0f);
    if (p->housing == HOUSING_PENTHOUSE_VILLA) {
        draw_textured_box_3d(r, vp, terrace, vec3_make(11.0f, 0.25f, 8.5f), 0.0f, (Color4){0.95f, 0.95f, 0.95f, 1.0f}, 10);
        draw_textured_box_3d(r, vp, vec3_make(terrace.x, 0.26f, terrace.z), vec3_make(7.2f, 0.08f, 4.5f), 0.0f, (Color4){0.65f, 0.95f, 1.0f, 1.0f}, 11);
    } else if (p->housing == HOUSING_STUDIO_APT) {
        draw_textured_box_3d(r, vp, terrace, vec3_make(6.5f, 0.18f, 5.5f), 0.0f, (Color4){0.85f, 0.80f, 0.72f, 1.0f}, 10);
    }
    draw_sports_car_3d(r, vp, vec3_make(39.0f, 0.0f, 42.0f), 0.45f);
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

    const GrindPlayer *p = &world->player;
    float cam_yaw = p->yaw_rad;

    glUseProgram(r->prog_3d);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, r->tex_city);
    glUniform1i(r->u_tex_3d, 0);
    set_atlas_tile(r, 0, 0);
    glBindVertexArray(r->vao_cube);

    float sun_angle = ((world->time_of_day_hours - 6.0f) / 15.0f) * GRIND_PI;
    glUniform3f(r->u_light_dir_3d, cosf(sun_angle) * 0.6f, fmaxf(0.25f, sinf(sun_angle)), 0.45f);
    glUniform3f(r->u_ambient_3d, 0.38f + 0.25f * day_t, 0.39f + 0.25f * day_t, 0.44f + 0.20f * day_t);

    /* 1. Textured Ground Terrain, Asphalt Boulevards (Tile 4) & Central Plaza (Tile 6) */
    draw_textured_box_3d(r, vp, vec3_make(0.0f, -0.5f, 0.0f), vec3_make(260.0f, 0.5f, 260.0f), 0.0f, (Color4){0.55f, 0.65f, 0.55f, 1.0f}, 5);
    draw_textured_box_3d(r, vp, vec3_make(-22.0f, -0.04f, 0.0f), vec3_make(12.0f, 0.06f, 220.0f), 0.0f, (Color4){0.75f, 0.75f, 0.78f, 1.0f}, 4);
    draw_textured_box_3d(r, vp, vec3_make( 22.0f, -0.04f, 0.0f), vec3_make(12.0f, 0.06f, 220.0f), 0.0f, (Color4){0.75f, 0.75f, 0.78f, 1.0f}, 4);
    draw_textured_box_3d(r, vp, vec3_make(0.0f, -0.04f, -26.0f), vec3_make(220.0f, 0.06f, 12.0f), 0.0f, (Color4){0.75f, 0.75f, 0.78f, 1.0f}, 4);
    draw_textured_box_3d(r, vp, vec3_make(0.0f, -0.04f,  26.0f), vec3_make(220.0f, 0.06f, 12.0f), 0.0f, (Color4){0.75f, 0.75f, 0.78f, 1.0f}, 4);

    for (int d = -90; d <= 90; d += 12) {
        draw_box_3d(r, vp, vec3_make(-22.0f, 0.03f, (float)d), vec3_make(0.35f, 0.02f, 4.5f), 0.0f, (Color4){0.95f, 0.82f, 0.22f, 1.0f});
        draw_box_3d(r, vp, vec3_make( 22.0f, 0.03f, (float)d), vec3_make(0.35f, 0.02f, 4.5f), 0.0f, (Color4){0.95f, 0.82f, 0.22f, 1.0f});
        draw_box_3d(r, vp, vec3_make((float)d, 0.03f, -26.0f), vec3_make(4.5f, 0.02f, 0.35f), 0.0f, (Color4){0.95f, 0.82f, 0.22f, 1.0f});
        draw_box_3d(r, vp, vec3_make((float)d, 0.03f,  26.0f), vec3_make(4.5f, 0.02f, 0.35f), 0.0f, (Color4){0.95f, 0.82f, 0.22f, 1.0f});
    }

    draw_textured_box_3d(r, vp, vec3_make(0.0f, -0.02f, 0.0f), vec3_make(30.0f, 0.08f, 30.0f), 0.0f, (Color4){0.85f, 0.92f, 0.78f, 1.0f}, 6);

    /* 2. Dense Procedural City Buildings (~85+ Textured Multi-Story Buildings) */
    for (int i = 0; i < world->building_count; ++i) {
        draw_detailed_building(r, vp, &world->buildings[i], evening);
    }

    /* 3. Procedural City Trees & Streetlamps with Nano Banana Sprites */
    for (int i = 0; i < world->tree_count; ++i) {
        draw_tree_3d(r, vp, world->tree_positions[i], 1.1f, cam_yaw);
    }
    const Vec3 lamp_locs[] = {
        {-15.0f, 0.0f, -20.0f}, { 15.0f, 0.0f, -20.0f}, {-15.0f, 0.0f,  20.0f},
        { 15.0f, 0.0f,  20.0f}, {-42.0f, 0.0f, -40.0f}, { 28.0f, 0.0f,  22.0f}
    };
    for (int i = 0; i < 6; ++i) {
        draw_streetlamp_3d(r, vp, lamp_locs[i], evening, cam_yaw);
    }

    /* 4. Physical 3D Market Shelf Counters & Display Bicycles */
    draw_textured_box_3d(r, vp, vec3_make(-32.0f, 0.0f, -3.2f), vec3_make(9.5f, 0.95f, 1.4f), 0.0f, (Color4){0.85f, 0.72f, 0.55f, 1.0f}, 8);
    draw_box_3d(r, vp, vec3_make(-35.2f, 0.95f, -3.2f), vec3_make(1.4f, 0.65f, 1.1f), 0.0f, (Color4){0.98f, 0.88f, 0.16f, 1.0f});
    draw_box_3d(r, vp, vec3_make(-33.2f, 0.95f, -3.2f), vec3_make(1.4f, 0.65f, 1.1f), 0.0f, (Color4){0.95f, 0.95f, 0.98f, 1.0f});
    draw_box_3d(r, vp, vec3_make(-31.2f, 0.95f, -3.2f), vec3_make(1.4f, 0.75f, 1.1f), 0.0f, (Color4){0.22f, 0.68f, 0.95f, 1.0f});
    draw_textured_box_3d(r, vp, vec3_make(-28.8f, 0.95f, -3.2f), vec3_make(1.8f, 0.45f, 1.2f), 0.0f, (Color4){0.85f, 0.68f, 0.45f, 1.0f}, 8);

    draw_bicycle_3d(r, vp, vec3_make(34.0f, 0.0f, -10.0f), 0.5f, (Color4){0.18f, 0.72f, 0.92f, 1.0f}, cam_yaw);
    draw_bicycle_3d(r, vp, vec3_make(37.0f, 0.0f, -10.0f), 0.5f, (Color4){0.92f, 0.32f, 0.25f, 1.0f}, cam_yaw);
    draw_lifestyle_courtyard(r, vp, &world->player);

    /* 5. Found Money on Ground (Textured Cash + Nano Banana Cash Stack Billboard) */
    if (world->ground_money_available) {
        float bob = 0.15f + 0.12f * sinf(world->anim_time_sec * 3.0f);
        Vec3 mp = vec3_make(world->ground_money_pos.x, bob, world->ground_money_pos.z);
        draw_textured_box_3d(r, vp, mp, vec3_make(1.2f, 0.30f, 0.65f), world->anim_time_sec, (Color4){0.55f, 1.0f, 0.65f, 1.0f}, 14);
        draw_sprite_billboard_3d(r, vp, vec3_make(mp.x, 0.35f, mp.z), (Vec2){1.4f, 1.4f}, cam_yaw, 9);
    }

    /* 6. Deployed Lemonade Stand (Wood Table Tile 8 + Fresh Lemonade Banner Tile 9 + Sprite Cart) */
    if (p->stand_deployed) {
        Vec3 sp = p->stand_pos;
        if (p->owns_storefront) {
            draw_textured_box_3d(r, vp, sp, vec3_make(6.2f, 4.5f, 5.0f), 0.0f, (Color4){0.98f, 0.92f, 0.55f, 1.0f}, 9);
        } else {
            draw_textured_box_3d(r, vp, vec3_make(sp.x, 0.0f, sp.z), vec3_make(2.5f, 1.05f, 1.4f), 0.0f, (Color4){0.95f, 0.88f, 0.55f, 1.0f}, 9);
            draw_box_3d(r, vp, vec3_make(sp.x - 0.4f, 1.10f, sp.z), vec3_make(0.45f, 0.65f, 0.45f), 0.0f, (Color4){0.98f, 0.92f, 0.15f, 1.0f});
            draw_box_3d(r, vp, vec3_make(sp.x, 1.10f, sp.z), vec3_make(0.10f, 2.2f, 0.10f), 0.0f, (Color4){0.80f, 0.80f, 0.82f, 1.0f});
            draw_box_3d(r, vp, vec3_make(sp.x, 3.20f, sp.z), vec3_make(3.4f, 0.35f, 3.4f), 0.0f, (Color4){0.95f, 0.26f, 0.22f, 1.0f});
            draw_sprite_billboard_3d(r, vp, vec3_make(sp.x, 0.0f, sp.z + 0.8f), (Vec2){2.4f, 2.6f}, cam_yaw, 8);
        }
    }

    if (world->rival_stand_active) {
        Vec3 rp = world->rival_stand_pos;
        draw_textured_box_3d(r, vp, vec3_make(rp.x, 0.0f, rp.z), vec3_make(2.4f, 1.05f, 1.4f), 0.0f, (Color4){0.75f, 0.45f, 0.65f, 1.0f}, 8);
        draw_box_3d(r, vp, vec3_make(rp.x, 3.10f, rp.z), vec3_make(3.2f, 0.35f, 3.2f), 0.0f, (Color4){0.75f, 0.18f, 0.55f, 1.0f});
    }

    if (world->weather == WEATHER_RAINY) {
        float rain_fall = fmodf(world->anim_time_sec * 14.0f, 8.0f);
        for (int rx = -3; rx <= 3; ++rx) {
            for (int rz = -3; rz <= 3; ++rz) {
                float drop_x = p->pos.x + (float)rx * 3.5f + sinf((float)rz) * 1.2f;
                float drop_z = p->pos.z + (float)rz * 3.5f + cosf((float)rx) * 1.2f;
                float drop_y = 8.0f - fmodf(rain_fall + (float)((rx + rz) & 3) * 2.0f, 8.0f);
                draw_box_3d(r, vp, vec3_make(drop_x, drop_y, drop_z),
                            vec3_make(0.03f, 0.75f, 0.03f), 0.0f, (Color4){0.70f, 0.82f, 0.96f, 0.75f});
            }
        }
    }

    /* 7. Active Courier Parcels (Cardboard Texture Tile 13 + Sprite Tile 10) */
    for (int i = 0; i < GRIND_MAX_CONTRACTS; ++i) {
        const GrindCourierContract *c = &world->contracts[i];
        if (!c->active) continue;
        if (!c->picked_up) {
            draw_textured_box_3d(r, vp, c->pickup_pos, vec3_make(1.0f, 0.9f, 1.0f), 0.2f, (Color4){0.95f, 0.85f, 0.72f, 1.0f}, 13);
            draw_sprite_billboard_3d(r, vp, vec3_make(c->pickup_pos.x, 0.9f, c->pickup_pos.z), (Vec2){1.1f, 1.1f}, cam_yaw, 10);
        } else if (c->claimed_by_player) {
            draw_box_3d(r, vp, c->dropoff_pos, vec3_make(1.4f, 6.5f, 1.4f), world->anim_time_sec, (Color4){0.20f, 0.95f, 0.88f, 1.0f});
        }
    }

    /* 8. Hired Employees (Sprite Tile 6 = Barista, Tile 7 = Bicycle Courier) */
    for (int i = 0; i < world->employee_count; ++i) {
        const GrindEmployee *e = &world->employees[i];
        if (!e->hired) continue;
        bool is_courier = (e->role == EMP_ROLE_COURIER);
        draw_articulated_human(
            r, vp, e->pos, e->facing_yaw, e->walk_phase,
            (Color4){0.15f, 0.78f, 0.92f, 1.0f},
            (Color4){0.18f, 0.22f, 0.30f, 1.0f},
            !is_courier, is_courier, false,
            cam_yaw, is_courier ? 7 : 6
        );
    }

    /* 9. NPCs (Sprite Tiles 0=Marcus, 1=Devon, 2=Chloe, 3=Banker Vance, 4=Citizen, 5=Lemonade Drinker) */
    for (int i = 0; i < world->npc_count; ++i) {
        const GrindNpc *npc = &world->npcs[i];
        Color4 shirt_col = {0.35f, 0.55f, 0.78f, 1.0f};
        Color4 pants_col = {0.20f, 0.24f, 0.32f, 1.0f};
        int stile = (npc->drink_timer > 0.0f) ? 5 : 4;
        if (npc->archetype == ARCHETYPE_LOYAL_FRIEND) {
            shirt_col = (Color4){0.22f, 0.78f, 0.38f, 1.0f};
            stile = 0;
        } else if (npc->archetype == ARCHETYPE_ENVIOUS_PEER) {
            shirt_col = (npc->envy > 0.35f) ? (Color4){0.88f, 0.22f, 0.22f, 1.0f} : (Color4){0.68f, 0.52f, 0.32f, 1.0f};
            stile = 1;
        } else if (npc->archetype == ARCHETYPE_OPPORTUNIST) {
            shirt_col = (npc->affection > 0.3f) ? (Color4){0.95f, 0.78f, 0.18f, 1.0f} : (Color4){0.65f, 0.35f, 0.68f, 1.0f};
            stile = 2;
        } else if (npc->archetype == ARCHETYPE_BANKER) {
            shirt_col = (Color4){0.16f, 0.20f, 0.28f, 1.0f};
            stile = 3;
        }

        draw_articulated_human(
            r, vp, npc->pos, npc->facing_yaw, npc->walk_phase,
            shirt_col, pants_col,
            npc->drink_timer > 0.0f, false, npc->envy > 0.45f,
            cam_yaw, stile
        );
    }

    /* 10. Player Avatar (in Map View) OR First-Person Viewmodel */
    if (input->map_transition_t > 0.15f) {
        draw_articulated_human(
            r, vp, p->pos, p->yaw_rad, (p->pos.x + p->pos.z) * 1.5f,
            (Color4){0.98f, 0.92f, 0.20f, 1.0f},
            (Color4){0.18f, 0.22f, 0.32f, 1.0f},
            false, p->has_satchel, false,
            cam_yaw, 0
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

        draw_child_box(r, vp, cam_root, vec3_make(-0.34f, -0.38f, -0.55f), vec3_make(0.2f, -0.15f, 0.0f), vec3_make(0.12f, 0.12f, 0.36f), sleeve_col);
        draw_child_box(r, vp, cam_root, vec3_make(-0.32f, -0.36f, -0.74f), vec3_make(0.0f,  0.0f,  0.0f), vec3_make(0.11f, 0.11f, 0.12f), hand_skin);
        draw_child_box(r, vp, cam_root, vec3_make( 0.34f, -0.38f, -0.55f), vec3_make(0.2f,  0.15f, 0.0f), vec3_make(0.12f, 0.12f, 0.36f), sleeve_col);
        draw_child_box(r, vp, cam_root, vec3_make( 0.32f, -0.36f, -0.74f), vec3_make(0.0f,  0.0f,  0.0f), vec3_make(0.11f, 0.11f, 0.12f), hand_skin);

        if (p->has_luxury_watch) {
            draw_child_box(r, vp, cam_root, vec3_make(-0.33f, -0.31f, -0.66f), vec3_make(0,0,0), vec3_make(0.14f, 0.04f, 0.06f), (Color4){0.98f, 0.84f, 0.18f, 1.0f});
        }
        if (p->inventory[ITEM_FOLDING_TABLE] > 0) {
            draw_child_box(r, vp, cam_root, vec3_make(0.0f, -0.35f, -0.82f), vec3_make(0,0,0), vec3_make(0.65f, 0.42f, 0.12f), (Color4){0.65f, 0.42f, 0.22f, 1.0f});
        } else if (p->inventory[ITEM_LEMON] > 0 || p->inventory[ITEM_SUGAR] > 0) {
            draw_child_box(r, vp, cam_root, vec3_make(0.28f, -0.28f, -0.78f), vec3_make(0,0,0), vec3_make(0.22f, 0.18f, 0.22f), (Color4){0.98f, 0.90f, 0.18f, 1.0f});
        }

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
        char stand_tag[96];
        if (p->unlocked_insights & GRIND_INSIGHT_UNIT_ECONOMICS) {
            snprintf(stand_tag, sizeof(stand_tag), "LEMONADE $%.2f (COGS $%.2f, MARGIN %.0f%%, %d CUPS)",
                     p->stand_price, grind_player_unit_cost_per_cup(world),
                     grind_player_profit_margin_pct(world), p->stand_lemonade_stock);
        } else {
            snprintf(stand_tag, sizeof(stand_tag), "LEMONADE $%.2f (%d CUPS)", p->stand_price, p->stand_lemonade_stock);
        }
        Vec3 sp = vec3_make(p->stand_pos.x, p->owns_storefront ? 5.2f : 4.1f, p->stand_pos.z);
        draw_world_label_2d(sp, vp, vw_i, vh_i, 2.1f,
                            (Color4){0.22f, 0.16f, 0.04f, 0.85f},
                            (Color4){1.0f, 0.92f, 0.25f, 1.0f}, stand_tag);
    }

    if (world->rival_stand_active) {
        char rtag[64];
        snprintf(rtag, sizeof(rtag), world->rival_allied_franchise ? "ALLIED FRANCHISE $%.2f" : "RIVAL STAND $%.2f (QUAL %.0f%%)",
                 world->rival_stand_price, world->rival_stand_quality * 100.0f);
        Vec3 rp = vec3_make(world->rival_stand_pos.x, 3.9f, world->rival_stand_pos.z);
        draw_world_label_2d(rp, vp, vw_i, vh_i, 2.0f,
                            (Color4){0.32f, 0.06f, 0.22f, 0.86f},
                            (Color4){1.0f, 0.65f, 0.88f, 1.0f}, rtag);
    }

    if (p->stamina < 25.0f || p->stress > 70.0f) {
        Color4 warn_c = {0.92f, 0.18f, 0.15f, 0.35f};
        push_quad_2d(0.0f, 0.0f, vw, 8.0f, warn_c);
        push_quad_2d(0.0f, vh - 8.0f, vw, 8.0f, warn_c);
        draw_text_2d(vw * 0.5f - 165.0f, vh - 28.0f, 2.3f, (Color4){1.0f, 0.45f, 0.40f, 1.0f},
                     "GRIND EXHAUSTION! REST [R] OR EAT A MEAL");
    }

    if (world->sale_popup_timer > 0.0f) {
        char sale_txt[32];
        snprintf(sale_txt, sizeof(sale_txt), "+$%.2f SALE!", world->sale_popup_amount);
        draw_world_label_2d(world->sale_popup_pos, vp, vw_i, vh_i, 2.5f,
                            (Color4){0.06f, 0.38f, 0.12f, 0.90f},
                            (Color4){0.45f, 1.0f, 0.55f, 1.0f}, sale_txt);
    }

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

    /* Top Status Bar */
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

    if (input->biz_drawer_open) {
        float tx = vw * 0.16f;
        float ty = vh * 0.16f;
        float tw = vw * 0.68f;
        float th = vh * 0.58f;
        push_quad_2d(tx, ty, tw, th, (Color4){0.05f, 0.08f, 0.14f, 0.94f});
        push_quad_2d(tx, ty, tw, 34.0f, (Color4){0.14f, 0.32f, 0.52f, 0.98f});
        draw_text_2d(tx + 16.0f, ty + 10.0f, 2.5f, (Color4){1.0f, 0.95f, 0.45f, 1.0f},
                     "BUSINESS & LOGISTICS TABLET (CLICK ROW OR PRESS 1-6, [B] TO CLOSE)");

        char rows[6][128];
        snprintf(rows[0], sizeof(rows[0]), "[1] RECIPE & CRAFT: %d LEMON / %d SUGAR (QUAL %.0f%%) -> CRAFT BATCH",
                 p->recipe_lemons_per_batch, p->recipe_sugar_per_batch, p->stand_lemonade_quality * 100.0f);
        snprintf(rows[1], sizeof(rows[1]), "[2] RETAIL PRICING: CURRENT $%.2f/CUP -> CYCLE PRICE ($1.20-$2.80)",
                 p->stand_price);
        snprintf(rows[2], sizeof(rows[2]), "[3] HIRE STAND BARISTA ($45 FEE, $3.50/HR) -> AUTO-CRAFTS & SELLS");
        snprintf(rows[3], sizeof(rows[3]), "[4] HIRE COURIER RIDER ($65 FEE, $4.50/HR) -> AUTO-DELIVERS PARCELS");
        snprintf(rows[4], sizeof(rows[4]), "[5] LAUNCH MARKETING CAMPAIGN ($50) -> BOOSTS BRAND AWARENESS (%.0f%%)",
                 p->marketing_awareness * 100.0f);
        snprintf(rows[5], sizeof(rows[5]), "[6] CAPITAL INVESTMENT: %s",
                 !p->owns_storefront ? "LEASE FLAGSHIP STOREFRONT ($350)" : "BUY WHOLESALE SUPPLIER EQUITY ($200)");

        for (int r_i = 0; r_i < 6; ++r_i) {
            float ry = vh * (0.22f + (float)r_i * 0.08f);
            float rh = vh * 0.068f;
            Color4 rbg = (r_i % 2 == 0)
                ? (Color4){0.10f, 0.15f, 0.24f, 0.92f}
                : (Color4){0.13f, 0.19f, 0.29f, 0.92f};
            push_quad_2d(tx + 14.0f, ry, tw - 28.0f, rh, rbg);
            draw_text_2d(tx + 26.0f, ry + rh * 0.32f, 2.1f, (Color4){0.88f, 0.96f, 1.0f, 1.0f}, rows[r_i]);
        }
    } else if (input->map_transition_t < 0.3f) {
        push_quad_2d(vw * 0.5f - 6.0f, vh * 0.5f - 1.0f, 12.0f, 2.0f, (Color4){1.0f, 1.0f, 1.0f, 0.75f});
        push_quad_2d(vw * 0.5f - 1.0f, vh * 0.5f - 6.0f, 2.0f, 12.0f, (Color4){1.0f, 1.0f, 1.0f, 0.75f});
    } else {
        draw_text_2d(vw * 0.5f - 140.0f, 98.0f, 2.8f, (Color4){0.98f, 0.92f, 0.25f, 1.0f}, "3D TACTICAL MANAGEMENT MAP");
    }

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
