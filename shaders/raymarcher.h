#ifndef RAYMARCHER_H
#define RAYMARCHER_H

#include "../include/pipeline/fragmentshader.h"
#include "octree.h"

class RayMarcherShader : public FragmentShader
{
private:
  const Octree &tree;

  //Theta = 45.0f
  const float THETA = 90.0f;
  const float TAN_THETA_2 = tan( (PI*THETA*0.5f)/180.0f );


public:
  RayMarcherShader(const Octree& tree) : tree(tree) {}

  rgba launch(const float* vertex_in, const float* dVdx, int n) override
  {
    vec2 pos( get_attribute("pos", vertex_in) );
    vec3 eye( get_uniform("eye") );
    mat4 inv_view( get_uniform("inv_view") );

    vec4 o_ = inv_view * vec4(0.0f, 0.0f, 0.0f, 1.0f);
    vec4 d_ = inv_view * vec4(vec3(pos(0)*TAN_THETA_2, pos(1)*TAN_THETA_2, -1.0f).unit(), 0.0f);
    vec3 o_ws(o_(0),o_(1),o_(2)), d_ws(d_(0),d_(1),d_(2));

    // TODO: this should return some attribute, or even the
    // leaf itself so we can do something useful
    // Knowing whether we intersect a leaf on the voxel tree
    // or not should be enough to compute visibilities and thus,
    // ambient occlusion.
    // As for actual shading I don't know
    float v = tree.closest_leaf(o_ws, d_ws);

    if(v != v)
      return rgba(0.1f, 0.1f, 0.1f, 1.0f);
    else
      return rgba(0.0f, 1.0f, 0.0f, 1.0f);
  }
};

#endif
