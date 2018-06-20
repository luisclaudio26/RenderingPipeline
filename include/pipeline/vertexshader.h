#ifndef VERTEX_SHADER_H
#define VERTEX_SHADER_H

#include <string>
#include <map>
#include <vector>
#include "attribute.h"
#include "../matrix.h"

class VertexShader
{
private:
  // QUESTION would index-based access be substantially
  // faster then name-based?
  inline const float* get_uniform(const std::string& name)
  {
    int stride = (*uniforms)[name].stride;
    return &uniform_data[stride];
  }

  inline const float* get_attribute(const std::string& name,
                                    const float* vbuffer)
  {
    int stride = (*attribs)[name].stride;
    return &vbuffer[stride];
  }

public:
  virtual void launch(const float* vertex_in, float* vertex_out,
                      int vertex_sz, vec4& position);

  // vertex attributes positions/strides
  std::map<std::string, Attribute> *attribs;

  // uniform memory
  const float *uniform_data;
  std::map<std::string, Attribute> *uniforms;
};

#endif
