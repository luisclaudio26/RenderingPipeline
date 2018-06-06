#include "../../include/pipeline/fragmentshader.h"

rgba FragmentShader::launch(const float* vertex_in)
{
  Attribute normal_id = (*attribs)["normal"];
  const float *normal = &vertex_in[4 + normal_id.stride];

  return rgba((normal[0]+1.0f)*0.5f,
              (normal[1]+1.0f)*0.5f,
              (normal[2]+1.0f)*0.5f,
              1.0f);
}
