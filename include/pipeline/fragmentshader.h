#ifndef FRAGMENT_SHADER_H
#define FRAGMENT_SHADER_H

#include <string>
#include <map>
#include <vector>
#include "attribute.h"
#include "texsampler.h"
#include "../matrix.h"

class FragmentShader
{
private:
  inline const float* get_uniform(const std::string& name)
  {
    int stride = (*uniforms)[name].stride;
    return &uniform_data[stride];
  }

  inline const float* get_attribute(const std::string& name,
                                    const float* vbuffer)
  {
    int stride = (*attribs)[name].stride;
    return &vbuffer[4 + stride];
  }

public:
  virtual rgba launch(const float* vertex_in, const float* dVdx, int n);

  // uniform memory
  const float *uniform_data;
  std::map<std::string, Attribute> *uniforms;

  std::map<std::string, Attribute> *attribs;
  std::vector<TextureSampler> *tex_units;
};

#endif
