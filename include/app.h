#ifndef APP_H
#define APP_H

#include <nanogui/screen.h>
#include "pipeline/pipeline.h"
#include "pipeline/texture.h"
#include "mesh.h"
#include "ogl.h"

const int DEFAULT_WIDTH = 960;
const int DEFAULT_HEIGHT = 540;

class Engine : public nanogui::Screen
{
private:
  // rendering pipeline
  GraphicPipeline gp;
  Framebuffer fbo;

  // scene info
  Mesh mesh; mat4 model;
  Texture checker;

  //display stuff
  nanogui::GLShader shader;
  GLuint color_gpu;
  int buffer_width, buffer_height;

  //comparison window
  SceneParameters param;
  OGL *ogl;

public:
  Engine(const char* path);
  bool keyboardEvent(int key, int scancode, int action, int modifiers) override;
  void draw(NVGcontext *ctx) override;
  void drawContents() override;
  bool resizeEvent(const Eigen::Vector2i &size) override;
};

#endif
