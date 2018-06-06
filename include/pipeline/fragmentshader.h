#ifndef FRAGMENT_SHADER_H
#define FRAGMENT_SHADER_H

#include <string>
#include <map>
#include "attribute.h"
#include "../matrix.h"

class FragmentShader
{
private:
public:
  rgba launch(const float* vertex_in);

  std::map<std::string, Attribute> *attribs;
};

#endif
