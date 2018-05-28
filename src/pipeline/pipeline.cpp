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
  // 2. [ ] Triangle clipping
  // 3. [ ] Perspective division
  // 4. [ ] Triangle culling
  // 5. [ ] Rasterization (including Fragment shader)

  // vertex processing stage. v stores the starting address of a new
  // vertex and v_id the actual "id" of this very same vertex
  for(int v = 0, v_id = 0; v < vbuffer_in_sz; v += vertex_size, ++v_id)
  {
    //extract vertex data and send to vertex shader
    const float* vertex_data = &vbuffer_in[v];
    float* target = &vbuffer[v_id*(vertex_size+1)];

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


  printf("%d ---------------\n", non_clipped);
  for(int i = 0; i < non_clipped; i += tri_sz)
  {
    float *v = &vbuffer[i];
    printf("%d - %f %f %f %f %f %f %f \n", i, v[0], v[1], v[2],
                                          v[3], v[4], v[5], v[6]);
  }
  printf("---------------\n");

}
