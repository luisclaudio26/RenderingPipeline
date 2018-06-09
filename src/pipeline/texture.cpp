//#define STB_IMAGE_IMPLEMENTATION
#include "../../3rdparty/stb_image.h"
#include "../../include/pipeline/texture.h"
#include <cstring>
#include <cstdio>

// -----------------------------
// --------- internal ----------
// -----------------------------
static void average_4(const unsigned char* P1, const unsigned char* P2,
                      const unsigned char* P3, const unsigned char* P4,
                      unsigned char* target, int n_channels)
{
  for(int i = 0; i < n_channels; ++i)
  {
    // compute the average and round to nearest integer
    int acc = (int)P1[i] + (int)P2[i] + (int)P3[i] + (int)P4[i];
    target[i] = (unsigned char)(acc*0.25+0.5f);
  }
}

// -----------------------------------
// --------- From texture.h ----------
// -----------------------------------
Texture::Texture() : data(NULL), l(0), n(0) { }

Texture::~Texture()
{
  if(data) delete[] data;
}

void Texture::load_from_file(const char* path)
{
  int dummy;
  unsigned char* img = stbi_load(path, &l, &dummy, &n, 0);

  //TODO: assert that img is square and dimensions are powers
  //of 2. nothing will work otherwise!

  // compute how much memory we need to store this texture
  // AND all of its MIP levels (this assumes textures are
  // power of 2!!!)
  int n_levels = (int)log2(l);

  // Memory for MIP levels is computed as follows:
  //
  // Starting with an image of dimension LxL, we need to store
  // memory for images of size L/2 x L/2, L/4 x L/4, L/8 x L/8,
  // ..., 1x1. The k-th level stores 2^k x 2^k = 4^k texels,
  // so we need a total of 4⁰+4¹+4²+...+4^(n-1) = (4^n-1)/3
  // texels. We compute 4^n as a bitshift because it is a bit
  // faster.
  int MIP_size = (2<<(2*n_levels)-1)/3;
  data = new unsigned char[n*(l*l+MIP_size)];

  // compute original image to our allocated memory area, then
  // delete the original one used by STB
  memcpy(data, img, n*l*l*sizeof(unsigned char));
  stbi_image_free(img);
}

void Texture::compute_mips()
{
  if(!data) return;

  #define AT(r,c,s) ((n)*((r)*(s)+(c)))

  // sz stores the size of the MIP level we're reducing now
  unsigned char* cur_level = &data[0];
  unsigned char* next_level = &data[n*l*l];
  int sz = l, half_sz = l/2;

  // keep reducing image until we have a single pixel
  while( sz > 1 )
  {
    // reduce cur_level and write to next_level
    for(int row = 0, r_out = 0; row < sz; row += 2, ++r_out)
      for(int col = 0, c_out = 0; col < sz; col += 2, ++c_out)
      {
        // fetch the four pixels we'll average to filter the image
        unsigned char *P1, *P2, *P3, *P4;
        P1 = &cur_level[AT(row,col,sz)];
        P2 = &cur_level[AT(row,col+1,sz)];
        P3 = &cur_level[AT(row+1,col,sz)];
        P4 = &cur_level[AT(row+1,col+1,sz)];

        // average out and write to the next level
        average_4(P1, P2, P3, P4, &next_level[AT(r_out,c_out,half_sz)], n);
      }

    // dimensions and offsets for next reduction
    cur_level = next_level;
    next_level += n*half_sz*half_sz;
    sz = half_sz; half_sz = sz/2;
  }
}

rgba Texture::texel(int i, int j, int level) const
{
  int l_ = l; //size of the k-th level

  // the mip levels are stored sequencially, so we need
  // to retrieve the starting address of the k-th image.
  // a bit of arithmetic leads us to the following formula:
  //
  //  add = l² n 4 (1 - 1/(4^k)) / 3
  //
  // which is not all intuitive. we'll just do a loop
  // summing sizes up to the level we want, because it is
  // easier to understand.
  int offset = 0;
  for(int k = 0; k < level; ++k, l_ /= 2)
    offset += n*l_*l_;

  int add = offset + n*(i*l_+j);
  unsigned char* t = &data[add];

  rgba out(t[0]/255.0f, t[1]/255.0f, t[2]/255.0f, 1.0f);
  if(n == 4) out(4) = t[3]/255.0f;

  return out;
}
