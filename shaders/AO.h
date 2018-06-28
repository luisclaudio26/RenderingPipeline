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
    vec3 nn( get_attribute("normal", vertex_in) );
    return rgba( (nn(0)+1.0f)*0.5f,
                  (nn(1)+1.0f)*0.5f,
                  (nn(2)+1.0f)*0.5f,
                  1.0f);
  }
};

#endif
