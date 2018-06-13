#include "../../include/pipeline/fragmentshader.h"
#include <cmath>

const vec3 LIGHT_POS(2.0f, 1.0f, -1.0f);

rgba FragmentShader::launch(const float* vertex_in, const float* dVdx, int n)
{
  // TODO: make this more user friendly! We should not
  // have to type 4 + stride everytime (the user should
  // not know that the first 4 floats are the XYZW positions
  // of the vertex).
  Attribute normal_id = (*attribs)["normal"];
  const float *normal_ = &vertex_in[4 + normal_id.stride];
  vec3 normal = vec3(normal_[0], normal_[1], normal_[2]).unit();

  Attribute pos_id = (*attribs)["pos"];
  const float *pos = &vertex_in[4 + pos_id.stride];
  vec3 p(pos[0], pos[1], pos[2]);

  Attribute tex_id = (*attribs)["texcoord"];
  const float *texcoord = &vertex_in[4 + tex_id.stride];

  // TEXTURE SAMPLING
  // [ ] receive a texture unit ID as uniform
  // [X] retrieve texture sampler using this ID and the texture manager
  // [X] sample using texcoord
  //rgba tex_sample = (*tex_units)[0].sampleTrilinear(texcoord[0], texcoord[1], dVdx[10], dVdx[11]);
  //rgba tex_sample = (*tex_units)[0].sampleBilinear(texcoord[0], texcoord[1]);

  rgba color;
  if(textures)
    //color = (*tex_units)[0].sampleNearestNeighbor(texcoord[0], texcoord[1]);
    color = (*tex_units)[0].sampleBilinear(texcoord[0], texcoord[1]);
    //color = (*tex_units)[0].sampleTrilinear(texcoord[0], texcoord[1], dVdx[10], dVdx[11]);
  else
    color = model_color;

  // phong shading
  vec3 v2l = (LIGHT_POS - p).unit();
  vec3 v2e = (*eye - p).unit();
  vec3 h = (v2l+v2e).unit();

  float diff_k = std::fmax(0.0f, normal.dot(v2l));
  float spec_k = std::fmax(0.0f, pow(normal.dot(h), 10.0f));

  rgba out = color*(0.1f + diff_k) + rgba(1.0f,1.0f,1.0f,0.0f)*spec_k;
  out(3) = 1.0f;

  return out;
}
