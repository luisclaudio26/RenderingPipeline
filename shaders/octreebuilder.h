#ifndef OCTREEBUILDER_H

#include "../include/pipeline/fragmentshader.h"

class OctreeBuilderShader : public FragmentShader
{
public:
  rgba launch(const float* vertex_in, const float* dVdx, int n) override
  {
    return rgba(0.0f, 0.0f, 1.0f, 1.0f);
  }
};

#endif
