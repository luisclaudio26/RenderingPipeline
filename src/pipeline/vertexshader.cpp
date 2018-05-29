#include "../../include/pipeline/vertexshader.h"
#include <cstdlib>

void VertexShader::launch(const float* vertex_in, float* vertex_out,
                          int vertex_sz, vec4& position)
{
  Attribute pos_id = (*attribs)["pos"];
  const float *pos_ = &vertex_in[pos_id.stride];

  vec4 pos(pos_[0], pos_[1], pos_[2], 1.0f);
  pos = (*projection) * (*view) * (*model) * pos;

  vertex_out[0] = 15.0f;
  vertex_out[1] = 20.0f;
  vertex_out[2] = 25.0f;

  // set output vertex
  for(int i = 0; i < 4; ++i) position(i) = pos(i);
}
