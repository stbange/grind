#ifndef GRIND_RENDERER_H
#define GRIND_RENDERER_H

#include "grind_core/grind_core.h"
#include "grind_app/grind_input.h"

typedef struct {
    unsigned int prog_3d;
    unsigned int prog_2d;
    unsigned int vao_cube;
    unsigned int vbo_cube;
    int cube_vertex_count;

    unsigned int vao_quad;
    unsigned int vbo_quad;

    unsigned int tex_city;
    unsigned int tex_sprites;

    unsigned int vao_2d;
    unsigned int vbo_2d;

    int u_mvp_3d;
    int u_model_3d;
    int u_color_3d;
    int u_light_dir_3d;
    int u_ambient_3d;
    int u_uv_rect_3d;
    int u_tex_mode_3d;
    int u_tex_3d;

    int u_res_2d;
    bool initialized;
} GrindRenderer;

bool grind_renderer_init(GrindRenderer *r);
void grind_renderer_shutdown(GrindRenderer *r);
void grind_renderer_draw_frame(GrindRenderer *r, const GrindWorld *world, const GrindInputState *input);

#endif /* GRIND_RENDERER_H */
