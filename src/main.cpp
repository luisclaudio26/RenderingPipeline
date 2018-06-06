#include <ctime>
#include <iostream>
#include "../include/app.h"

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
