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

const int DEFAULT_WIDTH = 960;
const int DEFAULT_HEIGHT = 540;

class Engine : public nanogui::Screen
{
private:
  GraphicPipeline gp;
  Framebuffer fbo;
  Mesh mesh;

  //display stuff
  nanogui::GLShader shader;
  GLuint color_gpu;

public:
  Engine(const char* path) : nanogui::Screen(Eigen::Vector2i(DEFAULT_WIDTH, DEFAULT_HEIGHT), "NanoGUI Test")
  {
    //Load model
    mesh.load_file( std::string(path) );

    // Allocate the texture we'll use to render
    // our final color buffer. Once the screen
    // is resized, we must reallocate this.
    glGenTextures(1, &color_gpu);
    glBindTexture(GL_TEXTURE_2D, color_gpu);
    glTexStorage2D(GL_TEXTURE_2D,
                    1,
                    GL_RGBA8,
                    DEFAULT_WIDTH,
                    DEFAULT_HEIGHT);

    fbo.resizeBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT);

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

    // upload triangles which we'll use to render the final
    // image and the corresponding texture coordinates
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
    fbo.clearColorBuffer();

    //draw stuff
  
    GLubyte *color_buffer = fbo.colorBuffer();

    //-------------------------------------------------------
    //---------------------- DISPLAY ------------------------
    //-------------------------------------------------------
    // send to GPU in texture unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, color_gpu);

    //WARNING: be careful with RGB pixel data
    //as OpenGL expects 4-byte aligned data
    //https://www.khronos.org/opengl/wiki/Common_Mistakes#Texture_upload_and_pixel_reads
    glPixelStorei(GL_UNPACK_LSB_FIRST, 0);
    glTexSubImage2D(GL_TEXTURE_2D,
                    0, 0, 0,
                    DEFAULT_WIDTH,
                    DEFAULT_HEIGHT,
                    GL_RGBA,
                    GL_UNSIGNED_BYTE,
                    color_buffer);

    //WARNING: IF WE DON'T SET THIS IT WON'T WORK!
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    shader.bind();
    shader.setUniform("frame", 0);

    //draw stuff
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    shader.drawArray(GL_TRIANGLES, 0, 6);
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
