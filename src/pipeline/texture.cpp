#define STB_IMAGE_IMPLEMENTATION
#include "../../3rdparty/stb_image.h"
#include "../../include/pipeline/texture.h"

Texture::Texture() : data(NULL), w(0), h(0), n(0) { }

Texture::~Texture()
{
  if(data) delete[] data;
}

void Texture::load_from_file(const char* path)
{
  unsigned char* img = stbi_load(path, &w, &h, &n, 0);

  // compute how much memory we need to store this texture
  // AND all of its MIP levels (this assumes textures are
  // power of 2!!!)
  int n_levels = (int)log2(w);

  // Memory for MIP levels is computed as follows:
  //
  // Starting with an image of dimension LxL, we need to store
  // memory for images of size L/2 x L/2, L/4 x L/4, L/8 x L/8,
  // ..., 1x1. Each of these levels
  //
  int MIP_memory = (2 << (2*n_levels) - 1)/3;

}

void Texture::compute_mips()
{

}

rgba Texture::texel(int i, int j) const
{
  int add = n*(i*w+j);
  unsigned char* t = &data[add];

  rgba out(t[0]/255.0f, t[1]/255.0f, t[2]/255.0f, 1.0f);
  if(n == 4) out(4) = t[3]/255.0f;

  return out;
}
