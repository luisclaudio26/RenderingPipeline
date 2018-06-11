#include "../../include/pipeline/fragmentshader.h"
#include <cmath>

rgba FragmentShader::launch(const float* vertex_in, const float* dVdx, int n)
{
  // TODO: make this more user friendly! We should not
  // have to type 4 + stride everytime (the user should
  // not know that the first 4 floats are the XYZW positions
  // of the vertex).
  Attribute normal_id = (*attribs)["normal"];
  const float *normal = &vertex_in[4 + normal_id.stride];

  Attribute tex_id = (*attribs)["texcoord"];
  const float *texcoord = &vertex_in[4 + tex_id.stride];

  // TEXTURE SAMPLING
  // [ ] receive a texture unit ID as uniform
  // [X] retrieve texture sampler using this ID and the texture manager
  // [X] sample using texcoord
  //return (*tex_units)[0].sampleTrilinear(texcoord[0], texcoord[1], dVdx[10], dVdx[11]);
  return (*tex_units)[0].sampleNearestNeighbor(texcoord[0], texcoord[1]);
}
