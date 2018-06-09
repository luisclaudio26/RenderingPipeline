#include "../../include/pipeline/texsampler.h"
#include <cstdio>

rgba TextureSampler::sample2D(float u, float v) const
{
  rgba out(1.0f, 0.0f, 0.0f, 1.0f);
  float i_ = (tex_data->l-1) * v;
  float j_ = (tex_data->l-1) * u;

  // get filtered texel
  switch(f)
  {
    case NearestNeighbor:
    {
      int i = (int)(i_ + 0.5f);
      int j = (int)(j_ + 0.5f);

      out = tex_data->texel(i,j);
      break;
    }

    case Bilinear:
    {
      int i = (int)i_, j = (int)j_;
      float y = i_ - i, x = j_ - j;

      // lookup the 4 nearest neighbors
      rgba N1 = tex_data->texel(i,j);
      rgba N2 = tex_data->texel(i+1,j);
      rgba N3 = tex_data->texel(i,j+1);
      rgba N4 = tex_data->texel(i+1,j+1);

      // interpolate in x
      rgba tx1 = N1+(N3-N1)*x;
      rgba tx2 = N2+(N4-N2)*x;
      out = tx1 + (tx2-tx1)*y;

      break;
    }

    case Trilinear:
    {

      out = rgba(1.0f, 0.0f, 0.0f, 1.0f);
      break;
    }
  }

  return out;
}
