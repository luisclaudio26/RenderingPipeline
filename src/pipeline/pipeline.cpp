#include "../../include/pipeline/pipeline.h"

GraphicPipeline::GraphicPipeline()
  : vbuffer_in(NULL), vbuffer(NULL), vertex_size(0)
{ }

GraphicPipeline::~GraphicPipeline()
{
  if(vbuffer_in) delete[] vbuffer_in;
  if(vbuffer) delete[] vbuffer;
}

void GraphicPipeline::upload_data(const std::vector<float>& data, int vertex_size)
{
  int n_floats = data.size();
  this->vertex_size = vertex_size;
  this->n_vertices = (int)(n_floats / vertex_size);
  this->vbuffer_in_sz = n_floats;

  // delete any previous allocated data
  if(vbuffer_in) delete[] vbuffer_in;

  // copy data into internal vertex buffer
  // TODO: as vbuffer_in doesn't change, we could simply store the pointer
  // instead of copying it (until we need to actually implement this in GPU)
  vbuffer_in = new float[n_floats];
  memcpy((void*)vbuffer_in, (const void*)data.data(), n_floats * sizeof(float));

  // allocate memory for the working vertex buffer.
  // We need extra space because of the extra w parameter
  // we'll need for perpective interpolation.
  vbuffer = new float[n_floats + n_vertices];
}

void GraphicPipeline::define_attribute(const std::string& name, int n_floats, int stride)
{
  Attribute a;
  a.size = n_floats;
  a.stride = stride;

  attribs[name] = a;
}

void GraphicPipeline::upload_uniform(const mat4& model,
                                      const mat4& view,
                                      const mat4& projection,
                                      const mat4& viewport)
{
  this->model = model;
  this->view = view;
  this->projection = projection;
  this->viewport = viewport;
}

void GraphicPipeline::render(Framebuffer& target)
{
  // 1. [X] Vertex shader execution
  // 2. [X] Triangle clipping
  // 3. [X] Perspective division
  // 4. [ ] Triangle culling
  // 5. [ ] Rasterization (including Fragment shader)

  // vertex processing stage. v stores the starting address of a new
  // vertex and v_id the actual "id" of this very same vertex
  for(int v = 0, v_id = 0; v < vbuffer_in_sz; v += vertex_size, ++v_id)
  {
    //extract vertex data and send to vertex shader
    const float* vertex_data = &vbuffer_in[v];
    float* target = &vbuffer[v_id*(vertex_size+1)];

    // TODO: THE FIRST 4 POSITIONS SHOULD CORRESPOND TO
    // GL_POSITION'S X Y Z W!!! The vertex shader must
    // guarantee that
    vshader.launch(vertex_data, vertex_size, target);
  }

  // triangle clipping
  int vbuffer_sz = vbuffer_in_sz + n_vertices;
  int tri_sz = 3*(vertex_size+1);
  int non_clipped = 0;
  for(int t = 0; t < vbuffer_sz; t += tri_sz)
  {
    bool discard = false;

    // a primitice is discarded if any of
    // its vertices is not visible
    for(int v_id = 0; v_id < 3; ++v_id)
    {
      float* v = &vbuffer[t + v_id*(vertex_size+1)];
      float w = v[3];

      // discard primitives behind the camera
      if( w <= 0 )
      {
        discard = true;
        break;
      }

      // clip primitives outside frustum
      if(std::fabs(v[0]) > w || std::fabs(v[1]) > w || std::fabs(v[2]) > w)
      {
        discard = true;
        break;
      }
    }

    // if this primitive has survived clipping, copy it to
    // the vertex buffer...
    // ...but well, things are a bit more complicated. We could
    // have different vertex buffers and make copying simpler,
    // but for this first, single-threaded version, we just send
    // surviving primitives to the beginning of vbuffer, e.g.,
    // suppose we have:
    //
    // vbuffer: [ V0 V1 V2 V3 V4 ... Vn ]
    // vbuffer_sz: n
    //
    // and we found that V0, V1 and V4 survived the clipping while
    // V2, V3, V5, V6, ... Vn did not; so we copy V4 to the beginning
    // of the vector and change its size:
    //
    // vbuffer: [ V0 V1 V4 V3 V4 ... Vn ]
    // vbuffer_sz: 3
    //
    // A GPU implementation of this would need a new buffer (or
    // intermediate per-block cache) to handle this without needing
    // lots of sync barriers.
    if(!discard)
    {
      float *target = &vbuffer[non_clipped];
      memmove(target, &vbuffer[t], tri_sz*sizeof(float));
      non_clipped += tri_sz;
    }
  }

  // update vertex buffer size (there will be garbage by the
  // end of the vector, but this isn't a problem).
  vbuffer_sz = non_clipped;

  // perspective division
  for(int v_id = 0; v_id < vbuffer_sz; v_id += (vertex_size+1))
  {
    float *v = &vbuffer[v_id];
    float w = v[3];

    for(int i = 0; i < vertex_size+1; ++i) v[i] /= w;
  }

  // triangle culling
  //TODO: In OpenGL architecture, culling happens in the primitive
  //assembly stage, which is the first part of rasterization
  int non_culled = 0;
  for(int t = 0; t < vbuffer_sz; t += tri_sz)
  {
    float *v0 = &vbuffer[t + 0*(vertex_size+1)];
    float *v1 = &vbuffer[t + 1*(vertex_size+1)];
    float *v2 = &vbuffer[t + 2*(vertex_size+1)];

    vec3 v0_(v0[0], v0[1], 1.0f);
    vec3 v1_(v1[0], v1[1], 1.0f);
    vec3 v2_(v2[0], v2[1], 1.0f);

    //compute cross product p = v0v1 X v0v2;
    //if p is pointing outside the screen, v0v1v2 are defined
    //in counter-clockwise order. then, reject or accept this
    //triangle based on the param.front_face flag.
    vec3 c = (v1_-v0_).cross(v2_-v0_);

    // cull clockwise triangles (z component of vector product
    // c is facing -z). We do the same in place update of the
    // vertex buffer as we did in the primitive clipping step
    if(c(2) >= 0)
    {
      float* target = &vbuffer[non_culled];
      memmove(target, &vbuffer[t], tri_sz*sizeof(float));
      non_culled += tri_sz;
    }
  }

  // update vbuffer size
  vbuffer_sz = non_culled;

  printf("%d ---------------\n", vbuffer_sz);
  for(int i = 0; i < vbuffer_sz; i += vertex_size+1)
  {
    float *v = &vbuffer[i];
    printf("%d - %f %f %f %f %f %f %f \n", i, v[0], v[1], v[2],
                                          v[3], v[4], v[5], v[6]);
  }
  printf("---------------\n");

}
