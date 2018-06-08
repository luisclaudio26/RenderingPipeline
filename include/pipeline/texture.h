#ifndef TEXTURE_H
#define TEXTURE_H

#include "../matrix.h"

class Texture
{
public:
  int w, h, n;

  // data holds not only the original image
  // but also the MIP levels. This means that
  // we'll allocate more memory than we actually
  // need initially (but once compute_mips() is
  // invoked, no memory is wasted).
  unsigned char* data;

  Texture();
  ~Texture();
  void load_from_file(const char* path);
  void compute_mips();
  rgba texel(int i, int j) const;
};

#endif
