#ifndef RAYMARCHER_H
#define RAYMARCHER_H

#include "../include/pipeline/fragmentshader.h"
#include "octree.h"

class RayMarcherShader : public FragmentShader
{
private:
  const Octree &tree;

  //Theta = 45.0f
  const float THETA = 70.0f;
  const float TAN_THETA_2 = tan( (PI*THETA*0.5f)/180.0f );

public:
  RayMarcherShader(const Octree& tree) : tree(tree) {}

  rgba launch(const float* vertex_in, const float* dVdx, int n) override
  {
    vec2 pos( get_attribute("pos", vertex_in) );
    vec3 eye( get_uniform("eye") );
    vec3 look_dir( get_uniform("look_dir") );
    mat4 inv_view( get_uniform("inv_view") );

    vec3 o = eye;
    vec3 d = (vec3(pos(0)*TAN_THETA_2, pos(1)*TAN_THETA_2, -1.0f) - o).unit();

    float v = 0.0f;
    for(int i = 0; i < 20; ++i)
    {
      vec4 p_ = inv_view * vec4(o + d*(i*0.2f), 1.0f);
      vec3 p(p_(0), p_(1), p_(2));

      if( tree.is_inside(p) ) v = 1.0f;
    }

    return rgba(0.0f, v, 0.0f, 1.0f);
  }
};

#endif
