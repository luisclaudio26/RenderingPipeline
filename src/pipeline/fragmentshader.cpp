#include "../../include/pipeline/fragmentshader.h"

rgba FragmentShader::launch(const float* vertex_in)
{
  Attribute normal_id = (*attribs)["normal"];
  const float *normal = &vertex_in[4 + normal_id.stride];

  printf("(%f %f %f) ", normal[0], normal[1], normal[2]);

  return rgba(normal[0], normal[1], normal[2], 1.0f);
}
