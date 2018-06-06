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

#include "../include/pipeline/pipeline.h"
#include "../include/mesh.h"

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

  vec3 eye, right, up, look_dir;
  float step, angle_step, cos_angle_step, sin_angle_step;

  bool lock_view, cull_back;

  //display stuff
  nanogui::GLShader shader;
  GLuint color_gpu;

public:
  Engine(const char* path);
  bool keyboardEvent(int key, int scancode, int action, int modifiers) override;
  void draw(NVGcontext *ctx) override;
  void drawContents() override;
};
