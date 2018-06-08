#ifndef TEXTURE_H
#define TEXTURE_H

#include "../matrix.h"

class Texture
{
public:
  int w, h, n;
  unsigned char* data;

  Texture();
  ~Texture();
  void load_from_file(const char* path);
  rgba texel(int i, int j) const;
};

#endif
