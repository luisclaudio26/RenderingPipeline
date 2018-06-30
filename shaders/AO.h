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

  rgba launch(const float* vertex_in, const float* dVdx, int n) override
  {
    vec3 N( get_attribute("normal", vertex_in) );
    vec3 P( get_attribute("pos", vertex_in) );

    const int n_rays = 4;
    float occlusion_f = 0.0f;

    // trace at least one ray in the Normal direction
    float d = tree.closest_leaf(P, N);
    if( !(d != d || d > 0.1f) ) occlusion_f += 1.0f;

    for(int i = 1; i < n_rays; ++i)
    {
      const float inv_max_rand = 1.0f / RAND_MAX;
      float x = 2.0f*(rand()*inv_max_rand-0.5f);
      float y = 2.0f*(rand()*inv_max_rand-0.5f);
      float z = 2.0f*(rand()*inv_max_rand-0.5f);
      vec3 D = vec3(x,y,z).unit();
      if( D.dot(N) < 0 ) D = D*(-1.0f);

      float d = tree.closest_leaf(P, D);
      if(d != d || d > 0.1f) continue;

      occlusion_f += 1.0f;
    }

    // final occlusion level
    float occlusion = occlusion_f/n_rays;

    // diffuse direct lighting
    const vec3 light_dir(-1.0f, -1.0f, 0.0f);

    float kdiff = std::max(0.0f, N.dot(-light_dir));
    float kamb = 0.5f;
    vec3 out = vec3(0.1f, 0.4f, 0.3f)*kamb + vec3(1.0f, 1.0f, 1.0f)*kdiff;
    out = out * (1.0f-occlusion);

    return rgba(out, 1.0f);
  }
};

#endif
