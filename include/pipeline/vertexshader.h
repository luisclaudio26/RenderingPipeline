#ifndef VERTEX_SHADER_H
#define VERTEX_SHADER_H

class VertexShader
{
private:
public:
  void launch(const float* vertex_in, int vertex_sz, float* vertex_out);
};

#endif
