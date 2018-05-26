#include "../../include/pipeline/framebuffer.h"

Framebuffer::Framebuffer(int w, int h) : w(w), h(h)
{
  int n_pixels = w*h;
  color = new RGBA8[n_pixels];
  depth = new float[n_pixels];
}

Framebuffer::~Framebuffer()
{
  delete[] color;
  delete[] depth;
}

int Framebuffer::width() const  { return w; }
int Framebuffer::height() const { return h; }

void Framebuffer::setColorBuffer(int i, int j, RGBA8 c)
{
  //TODO: check range of i and j
  color[i*w+j] = c;
}

void Framebuffer::setDepthBuffer(int i, int j, float d)
{
  //TODO: check range of i and j
  depth[i*w+j] = d;
}

float Framebuffer::getDepthBuffer(int i, int j) const
{
  //TODO: check range of i and j
  return depth[i*w+j];
}

void clearColorBuffer() { memset((void*)color, 0, sizeof(RGBA8)*w*h); }
void clearDepthBuffer() { memset((void*)depth, 0, sizeof(float)*w*h); }
