//#define STB_IMAGE_IMPLEMENTATION
#include "../../3rdparty/stb_image.h"
#include "../../include/pipeline/texture.h"
#include <cstring>

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

  // a buffer to hold averaged columns before
  // we average its rows
  unsigned char* aux = new unsigned char[n*l*l];

  // data_ind stores the location in data
  // buffer where we should store the next MIP level.
  // data_last stores the last MIP level.
  int data_ind = n*l*l, data_last = 0;

  for(int i = 0, sz = l; i < n; ++i, sz /= 2)
  {
    // loop over columns computing their average
    for(int col = 0, aux_i = 0;
        col < n*sz;
        col += 2*n, aux_i += n)
    {
      // average of neighboring texels
      for(int k = 0; k < 4; ++k)
        aux[aux_i+k] = (data[data_last+col+k] + data[data_last+col+n+k])*0.5f;
    }

    // loop over rows, computing its average
    // TODO: loopar nas colunas de AUX e salvar a média
    // em DATA a partir de DATA_IND

    memcpy(&data[ind], aux, sizeof(unsigned char)*n*sz*sz);
  }


  delete[] aux;
}

rgba Texture::texel(int i, int j) const
{
  int add = n*(i*l+j);
  unsigned char* t = &data[add];

  rgba out(t[0]/255.0f, t[1]/255.0f, t[2]/255.0f, 1.0f);
  if(n == 4) out(4) = t[3]/255.0f;

  return out;
}
