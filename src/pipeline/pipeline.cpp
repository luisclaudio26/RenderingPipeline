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
  // 1. Vertex shader execution
  // 2. Triangle clipping
  // 3. Perspective division
  // 4. Triangle culling
  // 5. Rasterization (including Fragment shader)

  //vertex processing stage. v stores the starting address of a new
  //vertex and v_id the actual "id" of this very same vertex
  for(int v = 0, v_id = 0; v < vbuffer_in_sz; v += vertex_size, ++v_id)
  {
    //extract vertex data
    const float* vertex_data = &vbuffer_in[v];
    float* target = &vbuffer[v_id*(vertex_size+1)];

    vshader.launch(vertex_data, vertex_size, target);
  }

  RGBA8 c = (RGBA8){255, 255, 255, 255};
  target.setColorBuffer(100, 100, c);
}
