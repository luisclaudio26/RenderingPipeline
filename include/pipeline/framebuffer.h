#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <nanogui/opengl.h>
#include <nanogui/glutil.h>

struct RGBA8
{
  GLubyte r, g, b, a;
};

class Framebuffer
{
private:
  int w, h;
  RGBA8 *color;
  float *depth;

public:
  Framebuffer(int w, int h);
  ~Framebuffer();

  int width() const;
  int height() const;

  void setColorBuffer(int i, int j, RGBA8 color);
  void setDepthBuffer(int i, int j, float depth);
  float getDepthBuffer(int i, int j) const;

  void clearColorBuffer();
  void clearDepthBuffer();
};

#endif
