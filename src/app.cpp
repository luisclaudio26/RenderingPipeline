#include "../include/app.h"

#include <nanogui/opengl.h>
#include <nanogui/glutil.h>
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <nanogui/button.h>
#include <nanogui/slider.h>
#include <nanogui/label.h>
#include <nanogui/checkbox.h>
#include <nanogui/colorpicker.h>
#include <nanogui/combobox.h>

void Engine::draw(NVGcontext *ctx)
{
  Screen::draw(ctx);
}

void Engine::drawContents()
{
  //--------------------------------
  //----------- RENDERING ----------
  //--------------------------------
  clock_t start = clock();

  //proj and viewport could be precomputed!
  mat4 view = mat4::view(param.cam.eye, param.cam.eye + param.cam.look_dir, param.cam.up);
  mat4 proj = mat4::perspective(param.cam.FoVy, param.cam.FoVx,
                                param.cam.near, param.cam.far);
  mat4 viewport = mat4::viewport(fbo.width(), fbo.height());

  fbo.clearDepthBuffer();
  fbo.clearColorBuffer();

  // TEXTURE SAMPLING
  // [X] Bind a loaded texture to a given texture unit
  // [X] Bind texture unit id to uniform
  gp.bind_tex_unit(checker, 0);

  gp.set_viewport(viewport);
  gp.upload_uniform("view", view.data(), 16);
  gp.upload_uniform("model", model.data(), 16);
  gp.upload_uniform("proj", proj.data(), 16);  
  gp.upload_uniform("tex", 0.0);
  //gp.upload_uniform("eye", param.cam.eye.data(), 3);

  gp.render(fbo, param.front_face == GL_CCW, param.draw_mode != GL_LINE);

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
                  buffer_width,
                  buffer_height,
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

  //count time
   clock_t elapsed = clock() - start;
   printf("\rTime per frame: %fs", ((double)elapsed)/CLOCKS_PER_SEC);
}

bool Engine::resizeEvent(const Eigen::Vector2i &size)
{
  buffer_height = this->height(), buffer_width = this->width();

  //delete previous texture and allocate a new one with the new size
  glDeleteTextures(1, &color_gpu);
  glGenTextures(1, &color_gpu);
  glBindTexture(GL_TEXTURE_2D, color_gpu);
  glTexStorage2D(GL_TEXTURE_2D,
                  1,
                  GL_RGBA8,
                  buffer_width,
                  buffer_height);

  //resize buffers
  fbo.resizeBuffer(buffer_width, buffer_height);
}

Engine::Engine(const char* path)
  : nanogui::Screen(Eigen::Vector2i(DEFAULT_WIDTH, DEFAULT_HEIGHT), "NanoGUI Test"),
    buffer_width(DEFAULT_WIDTH), buffer_height(DEFAULT_HEIGHT)
{
  // --------------------------------
  // --------- Scene setup ----------
  // --------------------------------
  float angle = 0.0174533f;
  param.cam.eye = vec3(0.0f, 0.0f, 0.0f);
  param.cam.up = vec3(0.0f, 1.0f, 0.0f);
  param.cam.cos_angle = (float)cos(angle);
  param.cam.sin_angle = (float)sin(angle);
  param.cam.look_dir = vec3(0.0f, 0.0f, -1.0f);
  param.cam.right = vec3(1.0f, 0.0f, 0.0f);
  param.cam.step = 0.1f;
  param.cam.near = 1.0f;
  param.cam.far = 10.0f;
  param.cam.FoVy = 45.0f;
  param.cam.FoVx = 45.0f;
  param.cam.lock_view = false;
  param.front_face = GL_CCW;
  param.draw_mode = GL_FILL;
  param.model_color = rgba(0.0f, 0.0f, 1.0f, 1.0f);
  param.light = vec3(2.0f, 1.0f, -1.0f);
  param.shading = 0;

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

    Eigen::Vector2f t = mesh.mUV.col(i);
    mesh_data.push_back(t(0));
    mesh_data.push_back(t(1));
  }

  mesh.transform_to_center(model);

  // load textures
  checker.load_from_file("../data/mandrill_256.jpg");
  checker.compute_mips();

  // ---------------------------------------------
  // ---------- Upload data to pipeline ----------
  // ---------------------------------------------
  gp.upload_data(mesh_data, 8);
  gp.define_attribute("pos", 3, 0);
  gp.define_attribute("normal", 3, 3);
  gp.define_attribute("texcoord", 2, 6);

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
                  buffer_width,
                  buffer_height);

  fbo.resizeBuffer(buffer_width, buffer_height);

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

  performLayout();
}

bool Engine::keyboardEvent(int key, int scancode, int action, int modifiers)
{
  if (Screen::keyboardEvent(key, scancode, action, modifiers)) return true;

  //camera movement
  if(key == GLFW_KEY_A && action == GLFW_REPEAT) {
    param.cam.eye += (-param.cam.right) * param.cam.step;
    if(param.cam.lock_view)
    {
      param.cam.look_dir = (vec3(0.0f, 0.0f, -5.5f) - param.cam.eye).unit();
      param.cam.right = param.cam.look_dir.cross(param.cam.up);
    }
    return true;
  }
  if(key == GLFW_KEY_D && action == GLFW_REPEAT) {
    param.cam.eye += param.cam.right * param.cam.step;
    if(param.cam.lock_view)
    {
      param.cam.look_dir = (vec3(0.0f, 0.0f, -5.5f) - param.cam.eye).unit();
      param.cam.right = param.cam.look_dir.cross(param.cam.up);
    }
    return true;
  }
  if( key == GLFW_KEY_W && action == GLFW_REPEAT ) {
    param.cam.eye += param.cam.look_dir * param.cam.step;
    //if(param.cam.param.cam.lock_view) param.cam.look_dir = (vec3(0.0f, 0.0f, -5.5f) - param.cam.eye).unit();
    return true;
  }
  if( key == GLFW_KEY_S && action == GLFW_REPEAT ) {
    param.cam.eye += param.cam.look_dir * (-param.cam.step);
    //if(param.cam.param.cam.lock_view) param.cam.look_dir = (vec3(0.0f, 0.0f, -5.5f) - param.cam.eye).unit();
    return true;
  }
  if( key == GLFW_KEY_R && action == GLFW_REPEAT ) {
    param.cam.eye += param.cam.up * param.cam.step;
    if(param.cam.lock_view)
    {
      param.cam.look_dir = (vec3(0.0f, 0.0f, -5.5f) - param.cam.eye).unit();
      param.cam.up = param.cam.right.cross(param.cam.look_dir);
    }

    return true;
  }
  if( key == GLFW_KEY_F && action == GLFW_REPEAT ) {
    param.cam.eye += (-param.cam.up) * param.cam.step;
    if(param.cam.lock_view)
    {
      param.cam.look_dir = (vec3(0.0f, 0.0f, -5.5f) - param.cam.eye).unit();
      param.cam.up = param.cam.right.cross(param.cam.look_dir);
    }
    return true;
  }

  //TODO: we can precompute sin and cos values!
  if( key == GLFW_KEY_U && action == GLFW_REPEAT ) {
    if(param.cam.lock_view) return true;

    vec3 u = param.cam.look_dir, v = param.cam.up;
    param.cam.look_dir = u*param.cam.cos_angle + v*param.cam.sin_angle;
    param.cam.up = -u*param.cam.sin_angle + v*param.cam.cos_angle;

    return true;
  }
  if( key == GLFW_KEY_J && action == GLFW_REPEAT ) {
    if(param.cam.lock_view) return true;

    vec3 u = param.cam.look_dir, v = param.cam.up;
    param.cam.look_dir = u*param.cam.cos_angle + -v*param.cam.sin_angle;
    param.cam.up = u*param.cam.sin_angle + v*param.cam.cos_angle;

    return true;
  }
  if( key == GLFW_KEY_K && action == GLFW_REPEAT ) {
    if(param.cam.lock_view) return true;

    vec3 u = param.cam.right, v = param.cam.look_dir;
    param.cam.right = u*param.cam.cos_angle + -v*param.cam.sin_angle;
    param.cam.look_dir = u*param.cam.sin_angle + v*param.cam.cos_angle;

    return true;
  }
  if( key == GLFW_KEY_H && action == GLFW_REPEAT ) {
    if(param.cam.lock_view) return true;

    vec3 u = param.cam.right, v = param.cam.look_dir;
    param.cam.right = u*param.cam.cos_angle + v*param.cam.sin_angle;
    param.cam.look_dir = -u*param.cam.sin_angle + v*param.cam.cos_angle;

    return true;
  }

  //------------
  if( key == GLFW_KEY_M && action == GLFW_REPEAT ) {
    if(param.cam.lock_view) return true;

    vec3 u = param.cam.right, v = param.cam.up;
    param.cam.right = u*param.cam.cos_angle + -v*param.cam.sin_angle;
    param.cam.up = u*param.cam.sin_angle + v*param.cam.cos_angle;

    return true;
  }
  if( key == GLFW_KEY_N && action == GLFW_REPEAT ) {
    if(param.cam.lock_view) return true;

    vec3 u = param.cam.right, v = param.cam.up;
    param.cam.right = u*param.cam.cos_angle + v*param.cam.sin_angle;
    param.cam.up = -u*param.cam.sin_angle + v*param.cam.cos_angle;

    return true;
  }
  //---------------

  return false;
}
