#include "../../include/pipeline/texsampler.h"
#include <cstdio>

// --------------------------------------
// -------------- Internal --------------
// --------------------------------------
static rgba bilinear_filter_lookup(float u, float v, const Texture* tex, int level = 0)
{
  int u_ = (int)u, v_ = (int)v;
  float x = u - u_, y = v - v_;

  // lookup the 4 nearest neighbors
  rgba N1 = tex->texel(v_    , u_    , level);
  rgba N2 = tex->texel(v_ + 1, u_    , level);
  rgba N3 = tex->texel(v_    , u_ + 1, level);
  rgba N4 = tex->texel(v_ + 1, u_ + 1, level);

  // interpolate in x
  rgba tx1 = N1+(N3-N1)*x;
  rgba tx2 = N2+(N4-N2)*x;

  // interpolate results in y
  return tx1 + (tx2-tx1)*y;
}

static rgba nn_filter_lookup(float u, float v, const Texture* tex)
{
  // just round coordinates
  int i = (int)(v + 0.5f);
  int j = (int)(u + 0.5f);
  return tex->texel(i,j);
}

// --------------------------------------
// -------- From TextureSampler ---------
// --------------------------------------
rgba TextureSampler::sample2D(float u, float v, float dudx, float dvdx) const
{
  rgba out(1.0f, 0.0f, 0.0f, 1.0f);

  // get filtered texel
  switch(f)
  {
    case NearestNeighbor:
    {
      out = nn_filter_lookup(u*tex_data->l, v*tex_data->l, tex_data);
      break;
    }

    case Bilinear:
    {
      float i = (tex_data->l-1)*v;
      float j = (tex_data->l-1)*u;
      out = bilinear_filter_lookup(j, i, tex_data);
      break;
    }

    case Trilinear:
    {
      // pixel area deformation
      // TODO: REALLY bad approximation
      float scale = std::fmax(dudx * tex_data->l, dvdx * tex_data->l);

      // not a minification operation. use box reconstruction
      if( scale <= 1.0f )
      {
        out = nn_filter_lookup(u*tex_data->l, v*tex_data->l, tex_data);
        break;
      }

      // MIP level estimation and interpolation parameter
      float k_ = log2(scale);
      int k = (int)k_;
      float a = k_ - k;

      int sz1 = tex_data->l/(1 << k);
      int i1 = v*(sz1-1);
      int j1 = u*(sz1-1);
      rgba mip1 = bilinear_filter_lookup(j1, i1, tex_data, k);

      // TODO: in the limit case, k+1 is not a valid
      // mip level. we should take care of this after
      int sz2 = tex_data->l/(1 << (k+1));
      int i2 = v*(sz2-1);
      int j2 = u*(sz2-1);
      rgba mip2 = bilinear_filter_lookup(j2, i2, tex_data, k+1);

      out = mip1 + (mip2-mip1)*a;

      break;
    }
  }

  return out;
}
