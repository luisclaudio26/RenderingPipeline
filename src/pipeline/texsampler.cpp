#include "../../include/pipeline/texsampler.h"

rgba TextureSampler::sample2D(float u, float v) const
{
  rgba out(1.0f, 0.0f, 0.0f, 1.0f);
  float i_ = (tex_data->h-1) * v;
  float j_ = (tex_data->w-1) * u;

  //unsigned char* texel = &tex_data->data[i*(tex_data->w*tex_data->n)+j*tex_data->n];



  switch(f)
  {
    case NearestNeighbor:
    {
      int i = (int)(i_ + 0.5f);
      int j = (int)(j_ + 0.5f);

      const unsigned char* texel = tex_data->texel(i,j);
      out(0) = texel[0] / 255.0f;
      out(1) = texel[1] / 255.0f;
      out(2) = texel[2] / 255.0f;

      break;
    }
    case Bilinear: break;
    case Trilinear: break;
  }

  return out;
}
