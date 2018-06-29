#ifndef RAYMARCHER_H
#define RAYMARCHER_H

#include "../include/pipeline/fragmentshader.h"
#include "octree.h"

class RayMarcherShader : public FragmentShader
{
private:
  const Octree &tree;

  //Theta = 45.0f
  const float THETA = 20.0f;
  const float TAN_THETA_2 = tan( (PI*THETA*0.5f)/180.0f );


public:
  RayMarcherShader(const Octree& tree) : tree(tree) {}

  rgba launch(const float* vertex_in, const float* dVdx, int n) override
  {
    vec2 pos( get_attribute("pos", vertex_in) );
    vec3 eye( get_uniform("eye") );
    mat4 inv_view( get_uniform("inv_view") );

    vec4 o_ = inv_view * vec4(0.0f, 0.0f, 0.0f, 1.0f);
    vec4 d_ = inv_view * vec4(vec3(pos(0)*TAN_THETA_2, pos(1)*TAN_THETA_2, -0.1f).unit(), 0.0f);
    vec3 o_ws(o_(0),o_(1),o_(2)), d_ws(d_(0),d_(1),d_(2));

    // TODO: this should return some attribute, or even the
    // leaf itself so we can do something useful
    // Knowing whether we intersect a leaf on the voxel tree
    // or not should be enough to compute visibilities and thus,
    // ambient occlusion.
    // As for actual shading I don't know
    vec3 nr;
    float v = tree.closest_leaf(o_ws, d_ws, nr);

    if( v != v ) return rgba(1.0f, 0.0f, 0.0f, 1.0f);
    else if (v < 0.0f) return rgba(1.0f, 1.0f, 0.0f, 1.0f);
    else
    {
      printf("%f ", v);
      return rgba((nr(0)+1.0f)*0.5f,
                  (nr(1)+1.0f)*0.5f,
                  (nr(2)+1.0f)*0.5f,
                  1.0f);
    }
  }
};

/* NOTE: this is how we correctly invert the view matrix

mat4 inv_view( vec4(u(0), u(1), u(2), 0.0f),
                vec4(v(0), v(1), v(2), 0.0f),
                vec4(w(0), w(1), w(2), 0.0f),
                vec4(param.cam.eye, 1.0f) );

*/

#endif
