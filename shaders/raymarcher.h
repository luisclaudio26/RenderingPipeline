#ifndef RAYMARCHER_H
#define RAYMARCHER_H

#include "../include/pipeline/fragmentshader.h"

class RayMarcherShader : public FragmentShader
{
public:
  rgba launch(const float* vertex_in, const float* dVdx, int n) override
  {
    return rgba(1.0f, 0.0f, 0.0f, 1.0f);
  }
};

#endif
