#ifndef APP_H
#define APP_H

#include <nanogui/screen.h>
#include "pipeline/pipeline.h"
#include "pipeline/texture.h"
#include "mesh.h"
#include "param.h"

#include "../shaders/octreebuilder.h"
#include "../shaders/passthrough.h"
#include "../shaders/raymarcher.h"
#include "../shaders/AO.h"

const int DEFAULT_WIDTH = 960;
const int DEFAULT_HEIGHT = 540;

class Engine : public nanogui::Screen
{
private:
  // octree build up
  VertexShader standard;
  OctreeBuilderShader voxelizer;
  GraphicPipeline gp;
  Framebuffer octreeTarget;

  // voxel rendering
  GraphicPipeline renderer;

  VertexShader standard_renderer;
  AmbientOcclusionShader amb_occ;

  PassthroughShader passthrough;
  RayMarcherShader raymarch;

  Framebuffer renderTarget;

  void compute_octree();

  // scene info
  Mesh mesh; mat4 model;

  //display stuff
  nanogui::GLShader shader;
  GLuint color_gpu;
  int buffer_width, buffer_height;

  //comparison window
  SceneParameters param;

public:
  Engine(const char* path);
  bool keyboardEvent(int key, int scancode, int action, int modifiers) override;
  void draw(NVGcontext *ctx) override;
  void drawContents() override;
  bool resizeEvent(const Eigen::Vector2i &size) override;
};

#endif
