#include "../../include/pipeline/vertexshader.h"
#include <cstdlib>
#include <cstring>

void VertexShader::launch(const float* vertex_in, float* vertex_out,
                          int vertex_sz, vec4& position)
{
  mat4 view( get_uniform("view") );
  mat4 proj( get_uniform("proj") );
  mat4 model( get_uniform("model") );

  // QUESTION: How to make this more user friendly?!
  // output position
  Attribute pos_id = (*attribs)["pos"];
  const float *pos_ = &vertex_in[pos_id.stride];

  vec4 pos(pos_[0], pos_[1], pos_[2], 1.0f);
  pos = model * pos;

  vertex_out[0] = pos(0);
  vertex_out[1] = pos(1);
  vertex_out[2] = pos(2);

  pos = proj * view * pos;
  for(int i = 0; i < 4; ++i)
    position(i) = pos(i);
}
