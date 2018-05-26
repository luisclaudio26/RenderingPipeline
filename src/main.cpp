#include <ctime>
#include <iostream>

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

class Engine : public nanogui::Screen
{
private:
  GraphicPipeline gp;

public:
  Engine(const char* path) : nanogui::Screen(Eigen::Vector2i(960, 540), "NanoGUI Test")
  {
  }

  virtual void draw(NVGcontext *ctx)
  {
    Screen::draw(ctx);
  }

  virtual void drawContents()
  {
  }
};

int main(int argc, char** args)
{
  nanogui::init();

  /* scoped variables. why this? */ {
    nanogui::ref<Engine> app = new Engine(args[1]);
    app->drawAll();
    app->setVisible(true);
    nanogui::mainloop();
  }

  nanogui::shutdown();
}
