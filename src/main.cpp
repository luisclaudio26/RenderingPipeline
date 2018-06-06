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

// angular step used in camera motion
#define THETA 0.0174533f
#define COSTHETA float(cos(THETA))
#define SINTHETA float(sin(THETA))

class Engine : public nanogui::Screen
{
private:
  // rendering pipeline
  GraphicPipeline gp;
  Framebuffer fbo;

  // scene info
  Mesh mesh; mat4 model;

  vec3 eye, right, up, look_dir; float step;
  bool lock_view;

  bool cull_back;

  //display stuff
  nanogui::GLShader shader;
  GLuint color_gpu;

public:
  Engine(const char* path) : nanogui::Screen(Eigen::Vector2i(DEFAULT_WIDTH, DEFAULT_HEIGHT), "NanoGUI Test")
  {
    eye = vec3(0.0f, 0.0f, 0.0f);
    up = vec3(0.0f, 1.0f, 0.0f);
    look_dir = vec3(0.0f, 0.0f, -1.0f);
    right = vec3(1.0f, 0.0f, 0.0f);
    step = 0.1f;
    lock_view = false;
    cull_back = false;

    // Load model and unpack.
    // The first version packs mesh data into
    // Eigen matrices. Although this indeed
    // makes thing easier, we prefer to make
    // the code self-contained, so we unpack
    // things into a plain std::vector.
    // TODO: Load .obj files
    mesh.load_file( std::string(path) );
    std::vector<float> mesh_data;
    int n_tris = mesh.mPos.cols(); //calling mPOs.cols() works?! LULZ?!?!?!?

    for(int i = 0; i < n_tris; ++i)
    {
      Eigen::Vector3f p = mesh.mPos.col(i);
      mesh_data.push_back(p(0));
      mesh_data.push_back(p(1));
      mesh_data.push_back(p(2));


      Eigen::Vector3f n = mesh.mNormal.col(i);
      mesh_data.push_back(n(0));
      mesh_data.push_back(n(1));
      mesh_data.push_back(n(2));
    }

    mesh.transform_to_center(model);

    // ---------------------------------------------
    // ---------- Upload data to pipeline ----------
    // ---------------------------------------------
    gp.upload_data(mesh_data, 6);
    gp.define_attribute("pos", 3, 0);
    gp.define_attribute("normal", 3, 3);

    // ----------------------------------
    // ---------- Framebuffers ----------
    // ----------------------------------
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

  virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) {
      if (Screen::keyboardEvent(key, scancode, action, modifiers)) return true;

      //camera movement
      if(key == GLFW_KEY_A && action == GLFW_REPEAT) {
        eye += (-right) * step;
        if(lock_view)
        {
          look_dir = (vec3(0.0f, 0.0f, -5.5f) - eye).unit();
          right = look_dir.cross(up);
        }
        return true;
      }
      if(key == GLFW_KEY_D && action == GLFW_REPEAT) {
        eye += right * step;
        if(lock_view)
        {
          look_dir = (vec3(0.0f, 0.0f, -5.5f) - eye).unit();
          right = look_dir.cross(up);
        }
        return true;
      }
      if( key == GLFW_KEY_W && action == GLFW_REPEAT ) {
        eye += look_dir * step;
        //if(lock_view) look_dir = (vec3(0.0f, 0.0f, -5.5f) - eye).unit();
        return true;
      }
      if( key == GLFW_KEY_S && action == GLFW_REPEAT ) {
        eye += look_dir * (-step);
        //if(lock_view) look_dir = (vec3(0.0f, 0.0f, -5.5f) - eye).unit();
        return true;
      }
      if( key == GLFW_KEY_R && action == GLFW_REPEAT ) {
        eye += up * step;
        if(lock_view)
        {
          look_dir = (vec3(0.0f, 0.0f, -5.5f) - eye).unit();
          up = right.cross(look_dir);
        }

        return true;
      }
      if( key == GLFW_KEY_F && action == GLFW_REPEAT ) {
        eye += (-up) * step;
        if(lock_view)
        {
          look_dir = (vec3(0.0f, 0.0f, -5.5f) - eye).unit();
          up = right.cross(look_dir);
        }
        return true;
      }

      //TODO: we can precompute sin and cos values!
      if( key == GLFW_KEY_U && action == GLFW_REPEAT ) {
        if(lock_view) return true;

        vec3 u = look_dir, v = up;
        look_dir = u*COSTHETA + v*SINTHETA;
        up = -u*SINTHETA + v*COSTHETA;

        return true;
      }
      if( key == GLFW_KEY_J && action == GLFW_REPEAT ) {
        if(lock_view) return true;

        vec3 u = look_dir, v = up;
        look_dir = u*COSTHETA + -v*SINTHETA;
        up = u*SINTHETA + v*COSTHETA;

        return true;
      }
      if( key == GLFW_KEY_K && action == GLFW_REPEAT ) {
        if(lock_view) return true;

        vec3 u = right, v = look_dir;
        right = u*COSTHETA + -v*SINTHETA;
        look_dir = u*SINTHETA + v*COSTHETA;

        return true;
      }
      if( key == GLFW_KEY_H && action == GLFW_REPEAT ) {
        if(lock_view) return true;

        vec3 u = right, v = look_dir;
        right = u*COSTHETA + v*SINTHETA;
        look_dir = -u*SINTHETA + v*COSTHETA;

        return true;
      }

      //------------
      if( key == GLFW_KEY_M && action == GLFW_REPEAT ) {
        if(lock_view) return true;

        vec3 u = right, v = up;
        right = u*COSTHETA + -v*SINTHETA;
        up = u*SINTHETA + v*COSTHETA;

        return true;
      }
      if( key == GLFW_KEY_N && action == GLFW_REPEAT ) {
        if(lock_view) return true;

        vec3 u = right, v = up;
        right = u*COSTHETA + v*SINTHETA;
        up = -u*SINTHETA + v*COSTHETA;

        return true;
      }
      //---------------

      return false;
    }

  virtual void draw(NVGcontext *ctx)
  {
    Screen::draw(ctx);
  }

  virtual void drawContents()
  {
    //--------------------------------
    //----------- RENDERING ----------
    //--------------------------------
    //proj and viewport could be precomputed!
    mat4 view = mat4::view(eye, eye + look_dir, up);

    mat4 proj = mat4::perspective(45.0f, 45.0f, 1.0f, 10.0f);
    mat4 viewport = mat4::viewport(fbo.width(), fbo.height());

    fbo.clearDepthBuffer();
    fbo.clearColorBuffer();

    gp.upload_uniform(model, view, proj, viewport);
    gp.render(fbo, cull_back);

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
