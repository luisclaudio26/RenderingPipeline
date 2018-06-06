#include "../../include/pipeline/vertexshader.h"
#include <cstdlib>
#include <cstring>

void VertexShader::launch(const float* vertex_in, float* vertex_out,
                          int vertex_sz, vec4& position)
{
  // QUESTION: How to make this more user friendly?!
  // output position
  Attribute pos_id = (*attribs)["pos"];
  const float *pos_ = &vertex_in[pos_id.stride];

  vec4 pos(pos_[0], pos_[1], pos_[2], 1.0f);
  pos = (*projection) * (*view) * (*model) * pos;

  vertex_out[0] = 15.0f;
  vertex_out[1] = 20.0f;
  vertex_out[2] = 25.0f;

  for(int i = 0; i < 4; ++i) position(i) = pos(i);

  // forward normals
  Attribute normal_id = (*attribs)["normal"];
  const float *normal = &vertex_in[normal_id.stride];

  memcpy(&vertex_out[normal_id.stride], normal, sizeof(float)*4);

  // forward tex coordinates
  Attribute tex_id = (*attribs)["texcoord"];
  const float *texcoord = &vertex_in[tex_id.stride];

  memcpy(&vertex_out[tex_id.stride], texcoord, sizeof(float)*2);
}
