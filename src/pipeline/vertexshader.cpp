#include "../../include/pipeline/vertexshader.h"

void VertexShader::launch(const float* vertex_in,
                            int vertex_sz, float* vertex_out)
{
  for(int i = 0; i < vertex_sz; ++i)
    vertex_out[i] = 15.0f;
  vertex_out[vertex_sz] = 1.0f;
}
