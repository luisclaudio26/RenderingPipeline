#include "../../include/pipeline/vertexshader.h"
#include <cstdlib>

void VertexShader::launch(const float* vertex_in,
                            int vertex_sz, float* vertex_out)
{
  Attribute pos_id = (*attribs)["pos"];
  const float *pos_ = &vertex_in[pos_id.stride];

  vec4 pos(pos_[0], pos_[1], pos_[2], 1.0f);
  pos = (*projection) * (*view) * (*model) * pos;

  for(int i = 0; i < 4; ++i)
    vertex_out[i] = pos(i);

    /*
  for(int i = 0; i < vertex_sz; ++i)
    printf("(%f %f)", vertex_in[i], vertex_out[i]);
  printf("\n");
  */
}
