#ifndef AMBIENT_OCCLUSION_H
#define AMBIENT_OCCLUSION_H

#include "../include/pipeline/fragmentshader.h"
#include "octree.h"

class AmbientOcclusionShader : public FragmentShader
{
private:
  const Octree &tree;

  const float EPS = 0.000001f;
  inline bool is_zero(float a) { return std::fabs(a) < EPS; }

public:
  AmbientOcclusionShader(const Octree& tree) : tree(tree) {}
  rgba launch(const float* vertex_in, const float* dVdx, int n) override;
};

#endif
