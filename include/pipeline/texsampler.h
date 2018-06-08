#ifndef TEX_SAMPLER_H
#define TEX_SAMPLER_H

#include "../matrix.h"
#include "texture.h"

class TextureSampler
{
public:
  enum Filter {NearestNeighbor, Bilinear, Trilinear};

  const Texture* tex_data;
  Filter f;

  void compute_mipmap();
  rgba sample2D(float u, float v) const;
  TextureSampler() : f(Trilinear) {}
};

#endif
