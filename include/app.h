#include <nanogui/opengl.h>
#include <nanogui/glutil.h>
#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <nanogui/button.h>
#include <nanogui/slider.h>
#include <nanogui/label.h>
#include <nanogui/checkbox.h>
#include <nanogui/colorpicker.h>
#include <nanogui/combobox.h>

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

  //comparison window
  SceneParameters param;
  OGL *ogl;

public:
  Engine(const char* path);
  bool keyboardEvent(int key, int scancode, int action, int modifiers) override;
  void draw(NVGcontext *ctx) override;
  void drawContents() override;
};
