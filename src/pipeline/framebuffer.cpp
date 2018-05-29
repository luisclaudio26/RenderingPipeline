#include "../../include/pipeline/framebuffer.h"

Framebuffer::Framebuffer()
{
  color = NULL;
  depth = NULL;
  w = h = 0;
}

Framebuffer::Framebuffer(int w, int h) : w(w), h(h)
{
  int n_pixels = w*h;
  color = new RGBA8[n_pixels];
  depth = new float[n_pixels];
}

Framebuffer::~Framebuffer()
{
  if(color) delete[] color;
  if(depth) delete[] depth;
}

void Framebuffer::resizeBuffer(int w, int h)
{
  this->w = w; this->h = h;
  delete[] color; color = new RGBA8[w*h];
  delete[] depth; depth = new float[w*h];
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

void Framebuffer::clearColorBuffer() { memset((void*)color, 0, sizeof(RGBA8)*w*h); }
void Framebuffer::clearDepthBuffer()
{
  for(int i = 0; i < w*h; ++i)
    depth[i] = 100.0f;
}
