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
public:
  rgba launch(const float* vertex_in, const float* dVdx, int n);

  std::vector<TextureSampler> *tex_units;
  std::map<std::string, Attribute> *attribs;
};

#endif
