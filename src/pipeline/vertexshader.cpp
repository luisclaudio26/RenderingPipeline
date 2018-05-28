#include "../../include/pipeline/vertexshader.h"
#include <cstdlib>

void VertexShader::launch(const float* vertex_in,
                            int vertex_sz, float* vertex_out)
{
  static int i = 0;

  for(int i = 0; i < vertex_sz; ++i)
    vertex_out[i] = (float)(rand()%10);

  vertex_out[3] = 5.0f;
  vertex_out[vertex_sz] = 1.0f;
}
