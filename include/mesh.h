#ifndef MESH_H
#define MESH_H

#include <string>
#include <vector>
#include <nanogui/glutil.h>
#include "primitives.h"
#include "matrix.h"
#include "../3rdparty/tiny_obj_loader.h"

//the elements of our packed data
struct Elem
{
  Vec3 pos, normal;
  RGB a, d, s;
  float shininess;
};

class Mesh
{
private:
  void load_geometry_data(const std::vector<tinyobj::shape_t>& shapes,
                          const tinyobj::attrib_t& attrib);

public:
  Eigen::MatrixXf mPos, mNormal, mUV, mAmb, mDiff, mSpec, mShininess;
  std::vector<Triangle> tris;

  int n_tris;
  std::vector<float> pos, uv;

  Mesh() : n_tris(0) {}
  Mesh(const std::string& path)
  {
    load_file(path);
  }

  void load_file(const std::string& path);
  void transform_to_center(mat4& M);
};

#endif
