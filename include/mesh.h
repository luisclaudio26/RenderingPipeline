#ifndef MESH_H
#define MESH_H

#include <string>
#include <vector>
#include <nanogui/glutil.h>
#include "primitives.h"
#include "matrix.h"

//the elements of our packed data
struct Elem
{
  Vec3 pos, normal;
  RGB a, d, s;
  float shininess;
};

class Mesh
{
public:
  Eigen::MatrixXf mPos, mNormal, mAmb, mDiff, mSpec, mShininess;
  std::vector<Triangle> tris;

  Mesh() {}
  Mesh(const std::string& path)
  {
    load_file(path);
  }

  void load_file(const std::string& path);
  void transform_to_center(mat4& M);
};

#endif
