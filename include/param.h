#ifndef CAMERA_H
#define CAMERA_H

#include "matrix.h"

struct Camera
{
  //Camera parameters
  vec3 eye, look_dir, up, right;
  float near, far, step, cos_angle, sin_angle, FoVy, FoVx;
  bool lock_view;
};

struct SceneParameters
{
  //scene parameters
  Camera cam;
  vec3 light;

  //model parameters
  rgba model_color;

  //general parameters
  GLenum front_face;
  GLenum draw_mode;
  int shading;
};

#endif
