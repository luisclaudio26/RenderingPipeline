#include "../../include/pipeline/pipeline.h"

GraphicPipeline::GraphicPipeline() : vertex_buffer(NULL), vertex_size(0) { }

GraphicPipeline::~GraphicPipeline()
{
  if(vertex_buffer) delete[] vertex_buffer;
}

void GraphicPipeline::upload_data(const std::vector<float>& data, int vertex_size)
{
  // delete any previous allocated data
  if(vertex_buffer) delete[] vertex_buffer;

  // copy data into internal vertex buffer
  int n_floats = data.size();
  vertex_buffer = new float[n_floats];
  memcpy((void*)vertex_buffer, (const void*)data.data(), n_floats * sizeof(float));

  this->vertex_size = vertex_size;
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
  RGBA8 c = (RGBA8){255, 255, 255, 255};
  target.setColorBuffer(100, 100, c);
}
