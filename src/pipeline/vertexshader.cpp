#include "../../include/pipeline/vertexshader.h"
#include <cstdlib>
#include <cstring>

void VertexShader::launch(const float* vertex_in, float* vertex_out,
                          int vertex_sz, vec4& position)
{
  mat4 view( get_uniform("view") );
  mat4 proj( get_uniform("proj") );
  mat4 model( get_uniform("model") );

  vec3 pos_( get_attribute("pos", vertex_in) );

  vec4 pos = model * vec4(pos_, 1.0f);

  //forward coordinates in world space
  vertex_out[0] = pos(0);
  vertex_out[1] = pos(1);
  vertex_out[2] = pos(2);

  //forward untransformed normals
  //TODO: THIS IS WRONG!!!
  int n_s = (*attribs)["normal"].stride;
  memcpy(&vertex_out[n_s], &vertex_in[n_s], 3*sizeof(float));

  //return projected vertex
  pos = proj * view * pos;
  for(int i = 0; i < 4; ++i)
    position(i) = pos(i);
}
