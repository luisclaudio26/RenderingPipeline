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


  bool intersect_box(const vec3& o, const vec3& d,
                      const vec3& min, const vec3& max,
                      float& tmin, float& tmax) const
  {
    #define EPS 0.00001f

    //use tavianator's slab method: sucessively clip ray in each axis
    tmin = -FLT_MAX; tmax = FLT_MAX;

    for(int i = 0; i < 3; ++i)
    {
      //if r.d(i) == 0, ray is parallel to the current axis
      if(d(i) == 0.0f ) continue;

      float t1, t2, ro = o(i);

      //this should avoid the cases where the ray intersects infinitely
      //many points on one of the planes
      if( min(i) == o(i) || max(i) == o(i)) ro += EPS;

      t1 = (min(i) - ro) / d(i);
      t2 = (max(i) - ro) / d(i);

      tmin = std::fmax(tmin, std::fmin(t1, t2));
      tmax = std::fmin(tmax, std::fmax(t1, t2));
    }

    //tmax = tmin is a hit right in the corner of the box,
    //which we assume to not to be a hit! TODO: is this a problem?
    return tmax >= tmin && tmax > 0.0f;
  }


public:
  RayMarcherShader(const Octree& tree) : tree(tree) {}

  rgba launch(const float* vertex_in, const float* dVdx, int n) override
  {
    vec2 pos( get_attribute("pos", vertex_in) );
    vec3 eye( get_uniform("eye") );
    vec3 look_dir( get_uniform("look_dir") );
    mat4 inv_view( get_uniform("inv_view") );

    vec4 o_ = inv_view * vec4(0.0f, 0.0f, 0.0f, 1.0f);
    vec4 d_ = inv_view * vec4(vec3(pos(0)*TAN_THETA_2, pos(1)*TAN_THETA_2, -1.0f).unit(), 0.0f);
    vec3 o_ws(o_(0),o_(1),o_(2)), d_ws(d_(0),d_(1),d_(2));

    float v = 0.0f;

    float tmin, tmax;
    if( intersect_box(o_ws, d_ws, tree.min, tree.max, tmin, tmax) )
      v = 1.0f;

    return rgba(0.0f, v, 0.0f, 1.0f);
  }
};

#endif
