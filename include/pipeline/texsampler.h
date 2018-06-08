#ifndef TEX_SAMPLER_H
#define TEX_SAMPLER_H

#include "../matrix.h"
#include <cmath>

class TextureSampler
{
private:
public:
  rgba sample2D(float u, float v) const
  {
    float s = cos(2.0f*3.14159265f*(4.0f*u +4.0f*v));
    s = 0.5f*(s + 1.0f);
    return rgba(s, s, s, 1.0f);
  }
};

#endif
