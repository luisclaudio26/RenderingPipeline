#ifndef PASSTHROUGH_SHADER_H
#define PASSTHROUGH_SHADER_H

#include "../include/pipeline/vertexshader.h"

class PassthroughShader : public VertexShader
{
public:
  void launch(const float* vertex_in, float* vertex_out, int vertex_sz, vec4& position) override
  {
      vec2 pos( get_attribute("pos", vertex_in) );
      position(0) = pos(0);
      position(1) = pos(1);
      position(2) = 1.0f;
      position(3) = 1.0f;
  }
};

#endif
