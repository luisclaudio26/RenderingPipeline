#ifndef AMBIENT_OCCLUSION_H
#define AMBIENT_OCCLUSION_H

#include "../include/pipeline/fragmentshader.h"
#include "octree.h"

class AmbientOcclusionShader : public FragmentShader
{
private:
  const Octree &tree;

public:
  AmbientOcclusionShader(const Octree& tree) : tree(tree) {}

  rgba launch(const float* vertex_in, const float* dVdx, int n) override
  {
    return rgba(0.0f, 1.0f, 0.0f, 1.0f);
  }
};

#endif
