#ifndef VERTEX_SHADER_H
#define VERTEX_SHADER_H

#include <string>
#include <map>
#include "attribute.h"
#include "../matrix.h"

class VertexShader
{
private:
public:
  void launch(const float* vertex_in, float* vertex_out,
              int vertex_sz, vec4& position);

  std::map<std::string, Attribute> *attribs;
  mat4 *model, *view, *projection, *viewport;
};

#endif
