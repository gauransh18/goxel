/* Goxel 3D voxels editor
 *
 * copyright (c) 2019 Guillaume Chereau <guillaume@noctua-software.com>
 *
 * Goxel is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.

 * Goxel is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.

 * You should have received a copy of the GNU General Public License along with
 * goxel.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "goxel.h"

#include "file_format.h"
#include "utils/vec.h"
#include "utils/color.h"
#include <math.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
#define CGLTF_IMPLEMENTATION
#define CGLTF_WRITE_IMPLEMENTATION
#include "../ext_src/cgltf/cgltf_write.h"
#pragma GCC diagnostic pop

typedef struct {
    cgltf_data *data;
    palette_t palette;
    cgltf_material *default_mat;
} gltf_t;

typedef struct {
    float   pos[3];
    float   normal[3];

    // XXX: for vertex color we are wasting space here.
    union {
        float color[4];
        float texcoord[2];
    };
} gltf_vertex_t;

typedef struct {
    bool vertex_color;
    bool visible_only;
    float simplify;
} export_options_t;

static export_options_t g_export_options = {
    .vertex_color = true,
};


// Return the next power of 2 larger or equal to x.
static int next_pow2(int x)
{
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;
    return x;
}

static size_t base64_encode(const uint8_t *data, size_t len, char *buf)
{
    const char table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                          'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                          'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                          'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                          'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                          'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                          'w', 'x', 'y', 'z', '0', '1', '2', '3',
                          '4', '5', '6', '7', '8', '9', '+', '/'};
    const int mod_table[] = {0, 2, 1};
    uint32_t a, b, c, triple;
    int i, j;
    size_t out_len = 4 * ((len + 2) / 3);
    if (!buf) return out_len;
    for (i = 0, j = 0; i < len;) {
        a = i < len ? data[i++] : 0;
        b = i < len ? data[i++] : 0;
        c = i < len ? data[i++] : 0;
        triple = (a << 0x10) + (b << 0x08) + c;

        buf[j++] = table[(triple >> 3 * 6) & 0x3F];
        buf[j++] = table[(triple >> 2 * 6) & 0x3F];
        buf[j++] = table[(triple >> 1 * 6) & 0x3F];
        buf[j++] = table[(triple >> 0 * 6) & 0x3F];
    }
    for (i = 0; i < mod_table[len % 3]; i++)
        buf[out_len - 1 - i] = '=';

    return out_len;
}

static char *data_new(const void *data, uint32_t len, const char *mime)
{
    char *string;
    if (!mime) mime = "application/octet-stream";
    string = calloc(strlen("data:") + strlen(mime) + strlen(";base64,") +
                    base64_encode(data, len, NULL) + 1, 1);
    sprintf(string, "data:%s;base64,", mime);
    base64_encode(data, len, string + strlen(string));
    return string;
}

#define DL_SIZE(head) ({ \
    int size = 0; \
    typeof(*(head)) *tmp; \
    DL_COUNT(head, tmp, size); \
    size; \
})

#define ALLOC(array, size_)                                                   \
    ({                                                                        \
        int size = size_;                                                     \
        if (size > 0) {                                                       \
            (array) = calloc(size, sizeof(*(array)));                         \
        }                                                                     \
    })

static void gltf_init(gltf_t *g, const export_options_t *options,
                      const image_t *img)
{
    const layer_t *layer;
    volume_iterator_t iter;
    int bpos[3], nb_blocks = 0;

    g->data = calloc(1, sizeof(*g->data));
    g->data->memory.free_func = &cgltf_default_free;
    g->data->asset.version = strdup("2.0");
    g->data->asset.generator = strdup("goxel");

    // Count the total number of blocks.
    DL_FOREACH(img->layers, layer) {
        iter = volume_get_iterator(layer->volume,
                VOLUME_ITER_TILES | VOLUME_ITER_INCLUDES_NEIGHBORS);
        while (volume_iter(&iter, bpos)) {
            nb_blocks++;
        }
    }

    // Initialize all the gltf base object arrays.
    ALLOC(g->data->materials, DL_SIZE(img->materials) + 1);
    ALLOC(g->data->scenes, 1);
    ALLOC(g->data->nodes, 1 + nb_blocks + DL_SIZE(img->layers));
    ALLOC(g->data->meshes, nb_blocks);
    ALLOC(g->data->accessors, nb_blocks * 4);
    ALLOC(g->data->buffers, nb_blocks * 2 + 1);
    ALLOC(g->data->buffer_views, nb_blocks * 2 + 1);
    ALLOC(g->data->images, 1);
    ALLOC(g->data->textures, 1);
}

#define add_item(data, list) ({ &(data)->list[(data)->list##_count++]; })

// Create a buffer view and attribute.
static void make_attribute(gltf_t *g, cgltf_buffer_view *buffer_view,
                           cgltf_primitive *primitive,
                           const char *name,
                           cgltf_component_type component_type,
                           cgltf_type type,
                           bool normalized, int count, int ofs,
                           const float v_min[3], const float v_max[3])
{
    cgltf_accessor *accessor;
    cgltf_attribute *attribute;

    accessor = add_item(g->data, accessors);
    accessor->buffer_view = buffer_view;
    accessor->component_type = component_type;
    accessor->offset = ofs;
    accessor->type = type;
    accessor->count = count;
    accessor->normalized = normalized;
    if (v_min) {
        vec3_copy(v_min, accessor->min);
        accessor->has_min = true;
    }
    if (v_max) {
        vec3_copy(v_max, accessor->max);
        accessor->has_max = true;
    }
    attribute = add_item(primitive, attributes);
    attribute->data = accessor;
    attribute->name = strdup(name);
    cgltf_parse_attribute_type(name, &attribute->type, &attribute->index);
}

static int get_material_idx(const image_t *img, const material_t *mat)
{
    int i;
    const material_t *m;
    for (i = 0, m = img->materials; m; m = m->next, i++) {
        if (m == mat) return i;
    }
    return 0;
}

static cgltf_material *save_material(
        gltf_t *g, const material_t *mat, const export_options_t *options)
{
    cgltf_material *material;
    cgltf_pbr_metallic_roughness *pbr;

    material = add_item(g->data, materials);
    material->alpha_cutoff = 0.5;
    material->has_pbr_metallic_roughness = true;
    pbr = &material->pbr_metallic_roughness;
    material->name = strdup(mat->name);
    vec3_copy(mat->emission, material->emissive_factor);
    vec4_copy(mat->base_color, pbr->base_color_factor);
    pbr->metallic_factor = mat->metallic;
    pbr->roughness_factor = mat->roughness;

    if (!options->vertex_color) {
        pbr->base_color_texture.texture = &g->data->textures[0];
        pbr->base_color_texture.scale = 1;
    }
    return material;
}

static cgltf_material *get_default_mat(
        gltf_t *g, const export_options_t *options)
{
    material_t mat;
    if (!g->default_mat) {
        mat = (material_t) {
            .base_color = {1, 1, 1, 1},
            .metallic = 1,
            .roughness = 1,
        };
        g->default_mat = save_material(g, &mat, options);
    }
    return g->default_mat;
}

static void save_layer(gltf_t *g, cgltf_node *root_node,
                       const image_t *img, const layer_t *layer,
                       const palette_t *palette,
                       int palette_pix_size,
                       const export_options_t *options)
{
    volume_mesh_t *mesh;
    cgltf_mesh *gmesh;
    cgltf_node *node;
    cgltf_primitive *primitive;
    cgltf_buffer *buffer;
    cgltf_buffer_view *buffer_view;
    cgltf_accessor *accessor;

    mesh = volume_generate_mesh(
            layer->volume, goxel.rend.settings.effects, palette,
            g_export_options.simplify);

    if (mesh->vertices_count == 0) return;

    gmesh = add_item(g->data, meshes);
    ALLOC(gmesh->primitives, 1);
    primitive = add_item(gmesh, primitives);
    primitive->type = cgltf_primitive_type_triangles;
    ALLOC(primitive->attributes, 3);
    if (layer->material) {
        primitive->material = g->data->materials +
                              get_material_idx(img, layer->material);
    } else {
        primitive->material = get_default_mat(g, options);
    }


    buffer = add_item(g->data, buffers);
    buffer->size = mesh->vertices_count * sizeof(*mesh->vertices);
    buffer->uri = data_new(mesh->vertices, buffer->size, NULL);
    buffer_view = add_item(g->data, buffer_views);
    buffer_view->buffer = buffer;
    buffer_view->size = buffer->size;
    buffer_view->stride = sizeof(*mesh->vertices);
    buffer_view->type = cgltf_buffer_view_type_vertices;

    make_attribute(
            g, buffer_view, primitive,
            "POSITION",
            cgltf_component_type_r_32f,
            cgltf_type_vec3, false,
            mesh->vertices_count, offsetof(typeof(*mesh->vertices), pos),
            mesh->pos_min, mesh->pos_max);
    make_attribute(
            g, buffer_view, primitive,
            "NORMAL",
            cgltf_component_type_r_32f,
            cgltf_type_vec3, false,
            mesh->vertices_count, offsetof(typeof(*mesh->vertices), normal),
            NULL, NULL);
    if (options->vertex_color) {
        make_attribute(g, buffer_view, primitive,
                       "COLOR_0",
                       cgltf_component_type_r_32f,
                       cgltf_type_vec4, false,
                       mesh->vertices_count,
                       offsetof(typeof(*mesh->vertices), color),
                       NULL, NULL);
    } else {
        make_attribute(g, buffer_view, primitive,
                       "TEXCOORD_0",
                       cgltf_component_type_r_32f, cgltf_type_vec2, false,
                       mesh->vertices_count,
                       offsetof(typeof(*mesh->vertices), texcoord),
                       NULL, NULL);
    }

    buffer = add_item(g->data, buffers);
    buffer->size = mesh->indices_count * sizeof(*mesh->indices);
    buffer->uri = data_new(mesh->indices, buffer->size, NULL);
    buffer_view = add_item(g->data, buffer_views);
    buffer_view->buffer = buffer;
    buffer_view->size = buffer->size;
    buffer_view->type = cgltf_buffer_view_type_indices;

    accessor = add_item(g->data, accessors);
    accessor->buffer_view = buffer_view;
    accessor->component_type = cgltf_component_type_r_32u;
    accessor->count = mesh->indices_count;
    accessor->type = cgltf_type_scalar;
    primitive->indices = accessor;

    node = add_item(g->data, nodes);
    node->mesh = gmesh;
    node->name = strdup(layer->name);
    *add_item(root_node, children) = node;

    volume_mesh_free(mesh);
}

static void create_palette_texture(
        gltf_t *g, const image_t *img, int pix_size)
{
    // Create the global palette with all the colors.
    layer_t *layer;
    volume_iterator_t iter;
    int i, s, pos[3], size, x, y, j, k;
    uint8_t c[4];
    uint8_t (*data)[3];
    uint8_t *png;
    cgltf_buffer *buffer;
    cgltf_buffer_view *buffer_view;
    cgltf_image *image;
    cgltf_texture *texture;

    DL_FOREACH(img->layers, layer) {
        iter = volume_get_iterator(layer->volume, 0);
        while (volume_iter(&iter, pos)) {
            volume_get_at(layer->volume, &iter, pos, c);
            palette_insert(&g->palette, c, NULL);
        }
    }

    s = ceil(sqrt(g->palette.size));
    s = max(next_pow2(s), 16);
    s *= pix_size;
    data = calloc(s * s, sizeof(*data));
    // Copy colors as blocks of pix_size x pix_size.
    for (k = 0; k < g->palette.size; k++) {
        x = (k % (s / pix_size)) * pix_size;
        y = (k / (s / pix_size)) * pix_size;
        for (i = 0; i < pix_size; i++) {
            for (j = 0; j < pix_size; j++) {
                memcpy(data[(y + i) * s + x + j],
                        g->palette.entries[k].color, 3);
            }
        }
    }
    png = img_write_to_mem((void*)data, s, s, 3, &size);
    free(data);
    buffer = add_item(g->data, buffers);
    buffer->size = size;
    buffer->uri = data_new(png, size, NULL);
    buffer_view = add_item(g->data, buffer_views);
    buffer_view->buffer = buffer;
    buffer_view->size = size;
    image = add_item(g->data, images);
    image->mime_type = strdup("image/png");
    image->buffer_view = buffer_view;
    texture = add_item(g->data, textures);
    texture->image = image;
    free(png);
}

static void gltf_export(const image_t *img, const char *path,
                        const export_options_t *options)
{
    gltf_t g = {};
    const layer_t *layer;
    cgltf_scene *scene;
    cgltf_node *root_node;
    cgltf_options gltf_options = {};
    material_t *mat;
    const palette_t *palette = NULL;
    const int palette_pix_size = 4;

    gltf_init(&g, options, img);

    if (!options->vertex_color) {
        create_palette_texture(&g, img, palette_pix_size);
        palette = &g.palette;
    }

    DL_FOREACH(img->materials, mat) {
        save_material(&g, mat, options);
    }

    root_node = add_item(g.data, nodes);
    mat4_set((void*)root_node->matrix,
             1, 0,  0, 0,
             0, 0, -1, 0,
             0, 1,  0, 0,
             0, 0,  0, 1);
    root_node->has_matrix = true;
    scene = add_item(g.data, scenes);
    ALLOC(scene->nodes, 1);
    *add_item(scene, nodes) = root_node;

    ALLOC(root_node->children, DL_SIZE(img->layers));
    DL_FOREACH(img->layers, layer) {
        if (options->visible_only && !layer->visible) continue;
        save_layer(&g, root_node, img, layer,
                   palette, palette_pix_size, options);
    }

    if (path) {
        const char *ext = strrchr(path, '.');
        if (ext && strcasecmp(ext, ".glb") == 0) {
            gltf_options.type = cgltf_file_type_glb;
        } else if (ext && strcasecmp(ext, ".gltf") == 0) {
            gltf_options.type = cgltf_file_type_gltf;
        }
    }

    cgltf_write_file(&gltf_options, path, g.data);
    cgltf_free(g.data);
    free(g.palette.entries);
}

static int export_as_gltf(const file_format_t *format, const image_t *img,
                          const char *path)
{
    gltf_export(img, path, &g_export_options);
    return 0;
}

static void export_gui(file_format_t *format)
{
    gui_checkbox(_("Vertex Color"), &g_export_options.vertex_color,
                 _("Save colors as vertex attribute"));
    gui_checkbox(_("Visible Only"), &g_export_options.visible_only,
                 _("Exclude hidden layers"));
    gui_input_float(_("Simplify"), &g_export_options.simplify, 0.1,
                    0, 1, "%.1f");
}

// Import support

typedef struct {
    const cgltf_primitive *primitive;
    cgltf_float world_matrix[16];
} gltf_prim_ref_t;

static bool append_prim_ref(gltf_prim_ref_t **items, size_t *len, size_t *cap,
                            const cgltf_primitive *primitive,
                            const cgltf_float world_matrix[16])
{
    gltf_prim_ref_t *new_items;
    size_t new_cap;
    if (*len >= *cap) {
        new_cap = *cap ? *cap * 2 : 16;
        new_items = realloc(*items, new_cap * sizeof(**items));
        if (!new_items) return false;
        *items = new_items;
        *cap = new_cap;
    }
    (*items)[*len].primitive = primitive;
    memcpy((*items)[*len].world_matrix, world_matrix, sizeof((*items)[*len].world_matrix));
    (*len)++;
    return true;
}

static size_t primitive_triangle_index_count(const cgltf_primitive *prim)
{
    size_t n;
    if (!prim || !prim->indices) return 0;
    n = prim->indices->count;
    switch (prim->type) {
    case cgltf_primitive_type_triangles:      return n;
    case cgltf_primitive_type_triangle_strip: return n >= 3 ? (n - 2) * 3 : 0;
    case cgltf_primitive_type_triangle_fan:   return n >= 3 ? (n - 2) * 3 : 0;
    default:                                  return 0;
    }
}

static void transform_point(const cgltf_float m[16],
                            const cgltf_float in[3], cgltf_float out[3])
{
    out[0] = m[0] * in[0] + m[4] * in[1] + m[8]  * in[2] + m[12];
    out[1] = m[1] * in[0] + m[5] * in[1] + m[9]  * in[2] + m[13];
    out[2] = m[2] * in[0] + m[6] * in[1] + m[10] * in[2] + m[14];
}

static bool collect_node_primitives(const cgltf_node *node,
                                    gltf_prim_ref_t **refs,
                                    size_t *refs_len, size_t *refs_cap)
{
    size_t i;
    cgltf_float world_matrix[16];
    if (!node) return true;

    if (node->mesh) {
        cgltf_node_transform_world(node, world_matrix);
        for (i = 0; i < node->mesh->primitives_count; i++) {
            const cgltf_primitive *prim = &node->mesh->primitives[i];
            const cgltf_accessor *pos_accessor;
            if (!prim->indices) continue;
            pos_accessor = cgltf_find_accessor(prim, cgltf_attribute_type_position, 0);
            if (!pos_accessor) continue;
            if (primitive_triangle_index_count(prim) == 0) continue;
            if (!append_prim_ref(refs, refs_len, refs_cap, prim, world_matrix)) {
                return false;
            }
        }
    }

    for (i = 0; i < node->children_count; i++) {
        if (!collect_node_primitives(node->children[i], refs, refs_len, refs_cap)) {
            return false;
        }
    }
    return true;
}

static bool append_triangle_indices(uint32_t *dst, size_t *dst_offset,
                                    const cgltf_uint *src, size_t src_count,
                                    size_t base_vertex,
                                    cgltf_primitive_type type)
{
    size_t k;
    if (type == cgltf_primitive_type_triangles) {
        for (k = 0; k < src_count; k++) {
            dst[(*dst_offset)++] = (uint32_t)(base_vertex + src[k]);
        }
        return true;
    }
    if (type == cgltf_primitive_type_triangle_strip) {
        for (k = 2; k < src_count; k++) {
            if (k & 1) {
                dst[(*dst_offset)++] = (uint32_t)(base_vertex + src[k - 1]);
                dst[(*dst_offset)++] = (uint32_t)(base_vertex + src[k - 2]);
                dst[(*dst_offset)++] = (uint32_t)(base_vertex + src[k]);
            } else {
                dst[(*dst_offset)++] = (uint32_t)(base_vertex + src[k - 2]);
                dst[(*dst_offset)++] = (uint32_t)(base_vertex + src[k - 1]);
                dst[(*dst_offset)++] = (uint32_t)(base_vertex + src[k]);
            }
        }
        return true;
    }
    if (type == cgltf_primitive_type_triangle_fan) {
        for (k = 2; k < src_count; k++) {
            dst[(*dst_offset)++] = (uint32_t)(base_vertex + src[0]);
            dst[(*dst_offset)++] = (uint32_t)(base_vertex + src[k - 1]);
            dst[(*dst_offset)++] = (uint32_t)(base_vertex + src[k]);
        }
        return true;
    }
    return false;
}

static int gltf_import(const file_format_t *format, image_t *image,
                       const char *path)
{
    cgltf_data *data = NULL;
    cgltf_options options = {0};
    cgltf_result result;
    int i, pos[3];
    uint8_t color[4];
    volume_iterator_t iter = {0};
    layer_t *layer;
    size_t total_vertices = 0, total_indices = 0;
    float *all_vertices = NULL;
    uint8_t *all_colors = NULL;
    uint32_t *all_indices = NULL;
    size_t vertex_offset = 0, index_offset = 0;
    gltf_prim_ref_t *prim_refs = NULL;
    size_t prim_refs_len = 0, prim_refs_cap = 0;
    size_t node_idx, ref_idx;

    if (!path) {
        LOG_E("glTF import: NULL path");
        return -1;
    }

    // Parse the glTF/GLB file
    result = cgltf_parse_file(&options, path, &data);
    if (result != cgltf_result_success || !data) {
        LOG_E("Failed to parse glTF file: %s (error %d)", path, (int)result);
        return -1;
    }

    // Validate the parsed data
    result = cgltf_validate(data);
    if (result != cgltf_result_success) {
        LOG_W("glTF validation warning (error %d), attempting to continue",
              (int)result);
    }

    // Load the buffer data (base64, external .bin, or GLB binary chunk)
    result = cgltf_load_buffers(&options, data, path);
    if (result != cgltf_result_success) {
        LOG_E("Failed to load glTF buffers (error %d)", (int)result);
        cgltf_free(data);
        return -1;
    }

    // Collect primitives from all scenes/nodes
    if (data->scene && data->scene->nodes_count > 0 && data->scene->nodes) {
        for (node_idx = 0; node_idx < data->scene->nodes_count; node_idx++) {
            if (!data->scene->nodes[node_idx]) continue;
            if (!collect_node_primitives(data->scene->nodes[node_idx],
                                         &prim_refs, &prim_refs_len,
                                         &prim_refs_cap)) {
                LOG_E("Failed to collect glTF primitives from default scene");
                goto fail;
            }
        }
    } else {
        // Fallback: try all nodes at root level
        for (node_idx = 0; node_idx < data->nodes_count; node_idx++) {
            if (!collect_node_primitives(&data->nodes[node_idx],
                                         &prim_refs, &prim_refs_len,
                                         &prim_refs_cap)) {
                LOG_E("Failed to collect glTF primitives from nodes");
                goto fail;
            }
        }
    }

    if (prim_refs_len == 0) {
        LOG_E("No mesh primitives found in glTF file: %s", path);
        goto fail;
    }

    // Count totals for allocation
    for (ref_idx = 0; ref_idx < prim_refs_len; ref_idx++) {
        const cgltf_primitive *prim = prim_refs[ref_idx].primitive;
        const cgltf_accessor *pos_accessor;
        if (!prim || !prim->indices) continue;
        pos_accessor = cgltf_find_accessor(prim,
                                           cgltf_attribute_type_position, 0);
        if (!pos_accessor) continue;
        total_vertices += pos_accessor->count;
        total_indices += primitive_triangle_index_count(prim);
    }

    if (total_vertices == 0 || total_indices == 0) {
        LOG_E("No mesh data found in glTF file");
        goto fail;
    }

    all_vertices = calloc(total_vertices * 3, sizeof(float));
    all_colors = calloc(total_vertices * 3, sizeof(uint8_t));
    all_indices = calloc(total_indices, sizeof(uint32_t));

    if (!all_vertices || !all_indices || !all_colors) {
        LOG_E("Failed to allocate memory for glTF mesh data");
        goto fail;
    }

    // Read vertex data from each primitive
    for (ref_idx = 0; ref_idx < prim_refs_len; ref_idx++) {
        const cgltf_primitive *prim = prim_refs[ref_idx].primitive;
        const cgltf_accessor *pos_accessor;
        const cgltf_accessor *color_accessor;
        cgltf_uint *raw_indices = NULL;
        size_t local_vertex_count, local_index_count;
        size_t local_v;

        if (!prim || !prim->indices) continue;

        pos_accessor = cgltf_find_accessor(prim,
                                           cgltf_attribute_type_position, 0);
        if (!pos_accessor) continue;

        color_accessor = cgltf_find_accessor(prim,
                                             cgltf_attribute_type_color, 0);

        local_vertex_count = pos_accessor->count;
        local_index_count = prim->indices->count;

        // Read positions and colors
        for (local_v = 0; local_v < local_vertex_count; local_v++) {
            cgltf_float p_in[3] = {0};
            cgltf_float p_world[3] = {0};
            cgltf_float c_in[4] = {1, 1, 1, 1};
            size_t dst_v = vertex_offset + local_v;

            if (dst_v * 3 + 2 >= total_vertices * 3) break;

            if (!cgltf_accessor_read_float(pos_accessor, local_v, p_in, 3))
                continue;
            transform_point(prim_refs[ref_idx].world_matrix, p_in, p_world);
            all_vertices[dst_v * 3 + 0] = p_world[0];
            all_vertices[dst_v * 3 + 1] = p_world[1];
            all_vertices[dst_v * 3 + 2] = p_world[2];

            if (color_accessor &&
                cgltf_accessor_read_float(color_accessor, local_v, c_in, 4)) {
                float r = c_in[0], g = c_in[1], b = c_in[2];
                if (r < 0) r = 0;
                if (r > 1) r = 1;
                if (g < 0) g = 0;
                if (g > 1) g = 1;
                if (b < 0) b = 0;
                if (b > 1) b = 1;
                // glTF vertex colors are in linear space; convert to sRGB.
                float lin[3] = {r, g, b};
                uint8_t srgb[3];
                rgb_to_srgb8(lin, srgb);
                all_colors[dst_v * 3 + 0] = srgb[0];
                all_colors[dst_v * 3 + 1] = srgb[1];
                all_colors[dst_v * 3 + 2] = srgb[2];
            } else {
                // Try to get color from material base color (also linear)
                if (prim->material &&
                    prim->material->has_pbr_metallic_roughness) {
                    float *bc = prim->material->pbr_metallic_roughness
                                    .base_color_factor;
                    float lin[3] = {
                        fminf(fmaxf(bc[0], 0), 1),
                        fminf(fmaxf(bc[1], 0), 1),
                        fminf(fmaxf(bc[2], 0), 1)
                    };
                    uint8_t srgb[3];
                    rgb_to_srgb8(lin, srgb);
                    all_colors[dst_v * 3 + 0] = srgb[0];
                    all_colors[dst_v * 3 + 1] = srgb[1];
                    all_colors[dst_v * 3 + 2] = srgb[2];
                } else {
                    all_colors[dst_v * 3 + 0] = 255;
                    all_colors[dst_v * 3 + 1] = 255;
                    all_colors[dst_v * 3 + 2] = 255;
                }
            }
        }

        // Read and expand indices
        raw_indices = malloc(local_index_count * sizeof(*raw_indices));
        if (!raw_indices) goto fail;
        if (cgltf_accessor_unpack_indices(prim->indices, raw_indices,
                                          sizeof(*raw_indices),
                                          local_index_count) == 0) {
            free(raw_indices);
            vertex_offset += local_vertex_count;
            continue;
        }

        if (!append_triangle_indices(all_indices, &index_offset,
                                     raw_indices, local_index_count,
                                     vertex_offset, prim->type)) {
            free(raw_indices);
            vertex_offset += local_vertex_count;
            continue;
        }
        free(raw_indices);
        vertex_offset += local_vertex_count;
    }

    if (index_offset == 0) {
        LOG_E("No valid triangle indices found in glTF");
        goto fail;
    }

    // Direct voxel extraction from mesh triangles.
    layer = image_add_layer(image, NULL);
    for (i = 0; (size_t)(i + 2) < index_offset; i += 3) {
        uint32_t i0 = all_indices[i];
        uint32_t i1 = all_indices[i + 1];
        uint32_t i2 = all_indices[i + 2];
        float v0x, v0y, v0z, v1x, v1y, v1z, v2x, v2y, v2z;
        float cx, cy, cz;
        float e1x, e1y, e1z, e2x, e2y, e2z;
        float nx, ny, nz, len;
        float vcx, vcy, vcz;

        if (i0 >= vertex_offset || i1 >= vertex_offset || i2 >= vertex_offset)
            continue;

        v0x = all_vertices[i0*3+0]; v0y = all_vertices[i0*3+1];
        v0z = all_vertices[i0*3+2];
        v1x = all_vertices[i1*3+0]; v1y = all_vertices[i1*3+1];
        v1z = all_vertices[i1*3+2];
        v2x = all_vertices[i2*3+0]; v2y = all_vertices[i2*3+1];
        v2z = all_vertices[i2*3+2];

        // Centroid in glTF world space (Y-up)
        cx = (v0x + v1x + v2x) / 3.0f;
        cy = (v0y + v1y + v2y) / 3.0f;
        cz = (v0z + v1z + v2z) / 3.0f;

        // Face normal via cross product
        e1x = v1x - v0x; e1y = v1y - v0y; e1z = v1z - v0z;
        e2x = v2x - v0x; e2y = v2y - v0y; e2z = v2z - v0z;
        nx = e1y * e2z - e1z * e2y;
        ny = e1z * e2x - e1x * e2z;
        nz = e1x * e2y - e1y * e2x;
        len = sqrtf(nx * nx + ny * ny + nz * nz);
        if (len > 1e-6f) {
            nx /= len;
            ny /= len;
            nz /= len;
        }

        // Step back half unit along normal -> voxel center
        vcx = cx - nx * 0.5f;
        vcy = cy - ny * 0.5f;
        vcz = cz - nz * 0.5f;

        // glTF Y-up (x,y,z) -> Goxel Z-up (x, -z, y)
        pos[0] = (int)floorf(vcx);
        pos[1] = (int)floorf(-vcz);
        pos[2] = (int)floorf(vcy);

        color[0] = all_colors[i0 * 3 + 0];
        color[1] = all_colors[i0 * 3 + 1];
        color[2] = all_colors[i0 * 3 + 2];
        color[3] = 255;
        volume_set_at(layer->volume, &iter, pos, color);
    }

    free(all_vertices);
    free(all_colors);
    free(all_indices);
    free(prim_refs);
    cgltf_free(data);

    return 0;

fail:
    free(all_vertices);
    free(all_colors);
    free(all_indices);
    free(prim_refs);
    if (data) cgltf_free(data);
    return -1;
}

FILE_FORMAT_REGISTER(gltf,
    .name = "gltf",
    .exts = {"*.gltf", "*.glb"},
    .exts_desc = "glTF",
    .export_gui = export_gui,
    .export_func = export_as_gltf,
    .import_func = gltf_import,
    .priority = 100,
)
