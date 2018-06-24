#ifndef OCTREEBUILDER_H

#include "../include/pipeline/fragmentshader.h"

class OctreeBuilderShader : public FragmentShader
{
public:
  rgba launch(const float* vertex_in, const float* dVdx, int n) override
  {
    vec3 pos( get_attribute("pos", vertex_in) );
    return rgba(pos(0), pos(1), pos(2), 1.0f);
  }
};

#endif
