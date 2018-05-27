#ifndef PIPELINE_H
#define PIPELINE_H

#include <vector>
#include <string>
#include <map>
#include "framebuffer.h"
#include "../matrix.h"

class GraphicPipeline
{
private:
  // Array of attributes. In a first moment we'll work
  // with FLOAT attributes only because it is easier to
  // manage pointer and interpolation if all attributes
  // have the same type. For the future we should support
  // different types.
  float *vertex_buffer;

  // Vertex size. The DATA array is comprised of vertex
  // attributes all aligned; vertex_size defines how many
  // attributes make up a vertex
  int vertex_size;

  // Attributes can be accessed within the vertex_buffer
  // by knowing its size and stride. We store this info
  // so it can be accessed later by the shaders
  struct Attribute { int size; int stride; };
  std::map<std::string, Attribute> attribs;

  // Uniform matrices. Viewport matrix is computed
  // based on the Framebuffer data received on render()
  mat4 model, view, projection;

public:
  GraphicPipeline();
  ~GraphicPipeline();

  // upload a set of floats containing all the attributes
  // contiguously defined. This can be slow because we make
  // a copy of this data, as our vertex buffer will be
  // modified many times during the pipeline.
  // Besides the actual data, vertex_size informs how many
  // floats makes up a vertex (for example, if a vertex has
  // position and normal information as vec3, vertex_size is 6).
  void upload_data(const std::vector<float>& data, int vertex_size);

  // Define an attribute inside the data uploaded. This
  // attribute is defined by telling how many floats it
  // is comprised of and the stride within the vertex.
  void define_attribute(const std::string& name, int n_floats, int stride);

  // After setting the attributes and uniforms,
  // render sends them through the pipeline and
  // stores the final result in the target Framebuffer
  void render(Framebuffer& target);
};

#endif
