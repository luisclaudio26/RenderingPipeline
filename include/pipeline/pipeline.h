#ifndef PIPELINE_H
#define PIPELINE_H

#include <vector>
#include "framebuffer.h"
#include "../matrix.h"

class GraphicPipeline
{
private:
  // Vertex attributes
  std::vector<vec3> pos;
  std::vector<vec3> normal;
  std::vector<vec2> uv;

  // Uniform matrices
  mat4 model, view, projection, viewport;

public:
  // After setting the attributes and uniforms,
  // render sends them through the pipeline and
  // stores the final result in the target Framebuffer
  void render(Framebuffer& target);
};

#endif
