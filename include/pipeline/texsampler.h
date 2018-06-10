#ifndef TEX_SAMPLER_H
#define TEX_SAMPLER_H

#include "texture.h"

class TextureSampler
{
public:
  const Texture* tex_data;

  TextureSampler() : tex_data(NULL) {}

  rgba sampleNearestNeighbor(float u, float v) const;
  rgba sampleBilinear(float u, float v) const;
  rgba sampleTrilinear(float u, float v, float dudx, float dvdx) const;
};

#endif
