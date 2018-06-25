#ifndef OCTREEBUILDER_H
#define OCTREEBUILDER_H

#include "../include/pipeline/fragmentshader.h"
#include "octree.h"

// TODO: create Octree object globally, so every call
// to OctreeBuilder.launch() we update the very same
// object. Ideally we should store those things as uniforms.

//---------------------
class OctreeBuilderShader : public FragmentShader
{
public:
  rgba launch(const float* vertex_in, const float* dVdx, int n) override
  {
    vec3 pos( get_attribute("pos", vertex_in) );
    OctreeBuilderShader::tree.add_point(pos);
    return rgba(0.0f, 0.0f, 1.0f, 1.0f);
  }

  static Octree tree;
};


#endif
