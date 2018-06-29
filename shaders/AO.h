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

    // compute a vector tangent to the normal. Our sampling
    // is always isotropic so tangents don't need to be consistent.
    // TODO: care for N(2) == 0
    // NOTE: Lots of copies here; inplace modifications would be better.
    float inv_Nz = is_zero(N(2)) ? 1.0f/N(0) : 1.0f/N(2);
    float Cz = P.dot(N) * inv_Nz;
    vec3 T = (P-vec3(0.0f, 0.0f, Cz)).unit();
    vec3 B = N.cross(T);

    // shoot ONE ray. if occluded, return zero.
    // this should give us a hint on whether AO
    // will work or not
    // NOTE: WE CANT SIMPLY SHOOT A RAY. The voxel representation
    // will hardly ever tightly pack the mesh, which means that
    // self-intersection when traversing the octree will happen:
    //
    // -----------    <- voxel boundary
    // _____     |
    //      \    |
    //      P--> X-->
    //      |    |
    //      |    |
    //   ^
    // actual mesh element
    //
    // When shooting rays here in fragment shader, we're shooting
    // it from P whereas we'd want to shoot from X, the (actual)
    // closest intersection between the ray P--> and the voxel
    // set. This could be naïvely solved by shooting a first ray
    // to retrieve the self-intersection, then shooting again to get
    // the actual intersection we want, but this is quite slow.
    // What's a better way to do it?

    int n_rays = 25;
    float occlusion_f = 0.0f;
    int valid_rays = 0;
    for(int i = 0; i < n_rays; ++i)
    {
      float x = rand()/RAND_MAX, y = rand()/RAND_MAX, z = rand()/RAND_MAX;
      vec3 D = B*(x-0.5f)*2.0f + N + T*(z-0.5f)*2.0f;
      D = D.unit();

      //vec3 D = N;

      float d = tree.closest_leaf(P, N);
      if(d != d) continue;
      if(d > 0.3f) d = 0.0f;

      occlusion_f += d;
      valid_rays++;
    }

    float o = 1.0f - occlusion_f/valid_rays;
    return rgba(o, o, o, 1.0f);
  }
};

//QUESTION:
//PRIMEIRO PASSO RENDERIZANDO A GEOMETRIA NORMAL, ARMAZENA EM G-BUFFER,
//DEPOIS RASTERIZA OS VOXELS COM A COR CORRETA E MISTURA OS DOIS

#endif
