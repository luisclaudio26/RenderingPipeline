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
