#define STB_IMAGE_IMPLEMENTATION
#include "../../3rdparty/stb_image.h"
#include "../../include/pipeline/texture.h"

Texture::Texture() : data(NULL), w(0), h(0), n(0) { }

Texture::~Texture()
{
  if(data) stbi_image_free(data);
}

void Texture::load_from_file(const char* path)
{
  data = stbi_load(path, &w, &h, &n, 0);
}

rgba Texture::texel(int i, int j) const
{
  int add = n*(i*w+j);
  unsigned char* t = &data[add];

  rgba out(t[0]/255.0f, t[1]/255.0f, t[2]/255.0f, 1.0f);
  if(n == 4) out(4) = t[3]/255.0f;

  return out;
}
