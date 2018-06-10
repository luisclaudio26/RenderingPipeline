#ifndef CAMERA_H
#define CAMERA_H

#include <nanogui/opengl.h>
#include <nanogui/glutil.h>
#include <glm/glm.hpp>

struct Camera
{
  //Camera parameters
  glm::vec3 eye, look_dir, up, right;
  float near, far, step, cos_angle, sin_angle, FoVy, FoVx;
  bool lock_view;
};

struct SceneParameters
{
  //scene parameters
  Camera cam;
  Eigen::Vector3f light;

  //model parameters
  Eigen::Vector3f model_color;

  //general parameters
  GLenum front_face;
  GLenum draw_mode;
  int shading;
};

#endif
