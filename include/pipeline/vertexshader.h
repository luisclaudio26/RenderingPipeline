#ifndef VERTEX_SHADER_H
#define VERTEX_SHADER_H

#include <string>
#include <map>
#include <vector>
#include "attribute.h"
#include "../matrix.h"

class VertexShader
{
protected:
  // QUESTION would index-based access be substantially
  // faster then name-based?
  // Update: YES. Callgrind profiles show that most of the
  // time spent in VertexShader (and FragmentShader as well)
  // is performing comparisons, which is due to map's binary
  // search.
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

  //TODO: implement these functions to ease shader writing
  //forward(string& name, vbuffer)
  //forward(string& name, const float* val, vbuffer)

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
