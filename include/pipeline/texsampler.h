#ifndef TEX_SAMPLER_H
#define TEX_SAMPLER_H

#include "../matrix.h"
#include "texture.h"
#include <cmath>

class TextureSampler
{
public:
  const Texture* tex_data;

  rgba sample2D(float u, float v) const
  {
    /*
    float s = cos(2.0f*3.14159265f*(4.0f*u +4.0f*v));
    s = 0.5f*(s + 1.0f);
    */
    rgba out(1.0f, 0.0f, 0.0f, 1.0f);

    int i = (int)((tex_data->h-1) * v);
    int j = (int)((tex_data->w-1) * u);

    unsigned char* texel = &tex_data->data[i*(tex_data->w*tex_data->n)+j*tex_data->n];
    out(0) = texel[0] / 255.0f;
    out(1) = texel[1] / 255.0f;
    out(2) = texel[2] / 255.0f;

    return out;
  }
};

#endif
