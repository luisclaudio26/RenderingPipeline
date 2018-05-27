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
#include "../include/mesh.h"

class Engine : public nanogui::Screen
{
private:
  nanogui::GLShader shader;
  GraphicPipeline gp;
  Mesh mesh;

public:
  Engine(const char* path) : nanogui::Screen(Eigen::Vector2i(960, 540), "NanoGUI Test")
  {
    //Load model
    mesh.load_file( std::string(path) );

    //--------------------------------------
    //----------- Shader options -----------
    //--------------------------------------
    shader.init("passthrough",

                //Vertex shader
                "#version 450\n"
                "// from host\n"
                "in vec2 quad_pos;\n"
                "in vec2 quad_uv;\n"

                "// to fragment shader\n"
                "out vec2 uv_frag;\n"

                "void main()\n"
                "{\n"
                  "gl_Position.xy = quad_pos;\n"
                  "gl_Position.zw = vec2(0.0f, 1.0f);\n"
                  "uv_frag = quad_uv;\n"
                "}\n",

                //Fragment shader
                "#version 450\n"
                "// from vertex shader\n"
                "in vec2 uv_frag;\n"

                "// fragment final color\n"
                "out vec4 color;\n"

                "// the actual color buffer\n"
                "uniform sampler2D frame;\n"

                "void main()\n"
                "{\n"
                  "color = texture(frame, uv_frag);\n"
                "}");


    //this will serve us both as screen coordinates for the quad
    //AND texture coordinates
    Eigen::MatrixXf quad(2, 6);
    quad.col(0)<<-1.0, -1.0;
    quad.col(1)<<+1.0, -1.0;
    quad.col(2)<<+1.0, +1.0;
    quad.col(3)<<-1.0, -1.0;
    quad.col(4)<<+1.0, +1.0;
    quad.col(5)<<-1.0, +1.0;

    //for some reason, OpenGL inverts the v axis,
    //so we undo this here
    Eigen::MatrixXf texcoord(2, 6);
    texcoord.col(0)<<0.0f, 1.0f;
    texcoord.col(1)<<1.0f, 1.0f;
    texcoord.col(2)<<1.0f, 0.0f;
    texcoord.col(3)<<0.0f, 1.0f;
    texcoord.col(4)<<1.0f, 0.0f;
    texcoord.col(5)<<0.0f, 0.0f;

    shader.bind();
    shader.uploadAttrib<Eigen::MatrixXf>("quad_pos", quad);
    shader.uploadAttrib<Eigen::MatrixXf>("quad_uv", texcoord);
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
