#include "primitive_common.h"

void _set_vertex_data_from_array(Evas_Canvas3D_Mesh *mesh,
                                 int frame,
                                 const float *data,
                                 Evas_Canvas3D_Vertex_Attrib attr,
                                 int start,
                                 int attr_count,
                                 int line,
                                 int vcount)
{
   float *address, *out;
   int stride, i, j;
   evas_canvas3d_mesh_frame_vertex_data_copy_set(mesh, frame, attr, 0, NULL);
   address = (float *)evas_canvas3d_mesh_frame_vertex_data_map(mesh, frame, attr);
   stride = evas_canvas3d_mesh_frame_vertex_stride_get(mesh, frame, attr);
   if (stride == 0) stride = sizeof(float) * attr_count;
   for (i = 0; i < vcount; i++)
     {
        out = (float *)((char *)address + stride * i);
        for (j = 0; j < attr_count; j++)
           out[j] = data[start + (line * i) + j];
     }
   evas_canvas3d_mesh_frame_vertex_data_unmap(mesh, frame, attr);
}

void
_set_vec3_vertex_data(Evas_Canvas3D_Mesh *mesh,
                      int frame,
                      int vcount,
                      Eina_Vector3 *data,
                      Evas_Canvas3D_Vertex_Attrib attr)
{
   float *address, *out;
   int stride, i;
   evas_canvas3d_mesh_frame_vertex_data_copy_set(mesh, frame, attr, 0, NULL);
   address = (float *)evas_canvas3d_mesh_frame_vertex_data_map(mesh, frame, attr);
   stride = evas_canvas3d_mesh_frame_vertex_stride_get(mesh, frame, attr);
   if (stride == 0) stride = sizeof(float) * 3;
   for (i = 0; i < vcount; i++)
     {
        out = (float *)((char *)address + stride * i);
        out[0] = data[i].x;
        out[1] = data[i].y;
        out[2] = data[i].z;
     }
   free(data);
   evas_canvas3d_mesh_frame_vertex_data_unmap(mesh, frame, attr);
}

void
_set_vec2_vertex_data(Evas_Canvas3D_Mesh *mesh,
                      int frame,
                      int vcount,
                      Eina_Vector2 *data,
                      Evas_Canvas3D_Vertex_Attrib attr)
{
   float *address, *out;
   int stride, i;
   evas_canvas3d_mesh_frame_vertex_data_copy_set(mesh, frame, attr, 0, NULL);
   address = (float *)evas_canvas3d_mesh_frame_vertex_data_map(mesh, frame, attr);
   stride = evas_canvas3d_mesh_frame_vertex_stride_get(mesh, frame, attr);
   if (stride == 0) stride = sizeof(float) * 2;
   for (i = 0; i < vcount; i++)
     {
        out = (float *)((char *)address + stride * i);
        out[0] = data[i].x;
        out[1] = data[i].y;
     }
   free(data);
   evas_canvas3d_mesh_frame_vertex_data_unmap(mesh, frame, attr);
}

void
_generate_indices(unsigned short *indices, int a, int b)
{
   int i, j, a1 = a + 1;
   unsigned short *index = &indices[0];

   for (i = 0; i < b; i++)
     {
        for (j = 0; j < a; j++)
          {
             *index++ = j + a1 * i;
             *index++ = j + a1 * (i + 1);
             *index++ = j + 1 + a1 * (i + 1);

             *index++ = j + a1 * i;
             *index++ = j + 1 +  a1 * i;
             *index++ = j + a1 * (i + 1) + 1;
          }
     }
}

void
_primitives_vec3_copy(Eina_Vector3 *dst, const Eina_Vector3 *src)
{
   dst->x = src->x;
   dst->y = src->y;
   dst->z = src->z;
}

void
_primitives_vec3_subtract(Eina_Vector3 *out, const Eina_Vector3 *a, const Eina_Vector3 *b)
{
   out->x = a->x - b->x;
   out->y = a->y - b->y;
   out->z = a->z - b->z;
}

void
_primitives_vec3_cross_product(Eina_Vector3 *out, const Eina_Vector3 *a, const Eina_Vector3 *b)
{
   Eina_Vector3 tmp;

   tmp.x = a->y * b->z - a->z * b->y;
   tmp.y = a->z * b->x - a->x * b->z;
   tmp.z = a->x * b->y - a->y * b->x;

   _primitives_vec3_copy(out, &tmp);
}

void
_primitives_vec3_normalize(Eina_Vector3 *out)
{
   Evas_Real size = out->x * out->x + out->y *out->y + out->z * out->z;
   size = sqrt(size);
   out->x /= size;
   out->y /= size;
   out->z /= size;
}

void
evas_common_set_model_from_primitive(Evas_Canvas3D_Mesh *model,
                                     int frame,
                                     Evas_Canvas3D_Primitive_Data *primitive)
{
   Evas_Real ratio = primitive->ratio;
   int precision = primitive->precision;
   Evas_Canvas3D_Surface_Func *surface = primitive->surface;
   Eina_Vector2 tex_scale = primitive->tex_scale;
   Evas_Canvas3D_Primitive_Mode mode = primitive->mode;

   switch (primitive->form)
     {
      case EVAS_CANVAS3D_MESH_PRIMITIVE_NONE:
      case EVAS_CANVAS3D_MESH_PRIMITIVE_COUNT:
        {
           ERR("Primitive with this type cannot be set to mesh.");
           break;
        }
      case EVAS_CANVAS3D_MESH_PRIMITIVE_SQUARE:
        {
           evas_model_set_from_square_primitive(model, frame);
           break;
        }
      case EVAS_CANVAS3D_MESH_PRIMITIVE_CUBE:
        {
           evas_model_set_from_cube_primitive(model, frame);
           break;
        }
      case EVAS_CANVAS3D_MESH_PRIMITIVE_CYLINDER:
        {
           evas_model_set_from_cylinder_primitive(model, frame, mode,
                                                  precision, tex_scale);
           break;
        }
      case EVAS_CANVAS3D_MESH_PRIMITIVE_CONE:
        {
           evas_model_set_from_cone_primitive(model, frame, mode,
                                              precision, tex_scale);
           break;
        }
      case EVAS_CANVAS3D_MESH_PRIMITIVE_SPHERE:
        {
           evas_model_set_from_sphere_primitive(model, frame, mode,
                                                precision, tex_scale);
           break;
        }
      case EVAS_CANVAS3D_MESH_PRIMITIVE_TORUS:
        {
           evas_model_set_from_torus_primitive(model, frame,
                                               ratio, precision, tex_scale);
           break;
        }
      case EVAS_CANVAS3D_MESH_PRIMITIVE_SURFACE:
        {
           evas_model_set_from_surface_primitive(model, frame,
                                                 surface, precision, tex_scale);
           break;
        }
      case EVAS_CANVAS3D_MESH_PRIMITIVE_TERRAIN:
        {
           evas_model_set_from_terrain_primitive(model, frame,
                                                 precision, tex_scale);
           break;
        }
      default:
        {
           ERR("Unknown type of primitive");
        }
     }
}
