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

Engine::Engine(const char* path)
  : nanogui::Screen(Eigen::Vector2i(DEFAULT_WIDTH, DEFAULT_HEIGHT), "NanoGUI Test")
{
  // --------------------------------
  // --------- Scene setup ----------
  // --------------------------------
  float angle = 0.0174533f;
  param.cam.eye = glm::vec3(0.0f, 0.0f, 0.0f);
  param.cam.up = glm::vec3(0.0f, 1.0f, 0.0f);
  param.cam.cos_angle = (float)cos(angle);
  param.cam.sin_angle = (float)sin(angle);
  param.cam.look_dir = glm::vec3(0.0f, 0.0f, -1.0f);
  param.cam.right = glm::vec3(1.0f, 0.0f, 0.0f);
  param.cam.step = 0.1f;
  param.cam.near = 1.0f;
  param.cam.far = 10.0f;
  param.cam.FoVy = 45.0f;
  param.cam.FoVx = 45.0f;
  param.cam.lock_view = false;
  param.front_face = GL_CCW;
  param.draw_mode = GL_FILL;
  param.model_color<<0.0f, 0.0f, 1.0f;
  param.light<<0.0f, 2.5f, 0.0f;
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

  //----------------------------------
  //----------- GUI setup ------------
  //----------------------------------
  using namespace nanogui;

  Window* window = new Window(this, "Scene options");
  window->setPosition(Vector2i(0, 0));
  window->setLayout(new GroupLayout());

  Button *reset_view = new Button(window, "Reset view");
  reset_view->setTooltip("Reset view so the object will be centered again");
  reset_view->setCallback( [this] { param.cam.up = glm::vec3(0.0f, 1.0f, 0.0f);
                                    param.cam.look_dir = glm::vec3(0.0f, 0.0f, -1.0f);
                                    param.cam.right = glm::vec3(1.0f, 0.0f, 0.0f);
                                    param.cam.eye = glm::vec3(0.0f, 0.0f, 0.0f);
                                    param.cam.near = 1.0f; param.cam.far = 10.0f;
                                    param.cam.FoVy = 45.0f; param.cam.FoVx = 45.0f; });

  new Label(window, "Near Z plane", "sans-bold");

  Slider *near_plane = new Slider(window);
  near_plane->setFixedWidth(100);
  near_plane->setTooltip("Set near Z plane to any value between 0 and 20");
  near_plane->setCallback( [this](float val) { param.cam.near = val * 20.0f; } );

  new Label(window, "Far Z plane", "sans-bold");

  Slider *far_plane = new Slider(window);
  far_plane->setFixedWidth(100);
  far_plane->setTooltip("Set near Z plane to any value between 10 and 100");
  far_plane->setCallback( [this](float val) { param.cam.far = 10.0f + val * (100.0f - 10.0f); } );

  new Label(window, "Field of view y (deg)", "sans-bold");

  Slider *fovy = new Slider(window);
  fovy->setFixedWidth(100);
  fovy->setTooltip("Set the field of view in Y to any value between 2 and 150");
  fovy->setCallback( [this](float val) { param.cam.FoVy = 2.0f + val * (150.0f - 2.0f); } );

  new Label(window, "Field of view x (deg)", "sans-bold");

  Slider *fovx = new Slider(window);
  fovx->setFixedWidth(100);
  fovx->setTooltip("Set the field of view in x to any value between 2 and 150");
  fovx->setCallback( [this](float val) { param.cam.FoVx = 2.0f + val * (150.0f - 2.0f); } );

  new Label(window, "Model color", "sans-bold");
  ColorPicker *color_picker = new ColorPicker(window, param.model_color);
  color_picker->setFinalCallback([this](const Color& c) { this->param.model_color = c.head<3>(); });

  CheckBox *draw_cw = new CheckBox(window, "Draw triangles in CW order");
  draw_cw->setTooltip("Uncheck this box for drawing triangles in CCW order");
  draw_cw->setCallback([&](bool cw) { param.front_face = cw ? GL_CW : GL_CCW; });

  CheckBox *lock_view = new CheckBox(window, "Lock view on the model");
  lock_view->setTooltip("Lock view point at the point where the model is centered. This will disable camera rotation.");
  lock_view->setCallback([&](bool lock) { param.cam.lock_view = lock;
                                          param.cam.look_dir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - param.cam.eye);
                                          param.cam.right = glm::cross(param.cam.look_dir, param.cam.up);
                                        });

  ComboBox *draw_mode = new ComboBox(window, {"Points", "Wireframe", "Fill"});
  draw_mode->setCallback([&](int opt) {
                          switch(opt)
                          {
                            case 0: param.draw_mode = GL_POINT; break;
                            case 1: param.draw_mode = GL_LINE; break;
                            case 2: param.draw_mode = GL_FILL; break;
                          } });

  ComboBox *shading_model = new ComboBox(window, {"GouraudAD", "GouraudADS", "PhongADS", "No shading"});
  shading_model->setCallback([&](int opt) {
                              switch(opt)
                              {
                                case 0: param.shading = 0; break;
                                case 1: param.shading = 1; break;
                                case 2: param.shading = 2; break;
                                case 3: param.shading = 3; break;
                              } });


  // ------------------------------------
  // -------- OpenGL comparison ---------
  // ------------------------------------
  nanogui::Window *winOpenGL = new nanogui::Window(this, "OpenGL");
  winOpenGL->setSize({480, 270});
  winOpenGL->setPosition(Eigen::Vector2i(50,50));
  winOpenGL->setLayout(new nanogui::GroupLayout());

  ogl = new OGL(param, path, winOpenGL);
  ogl->setSize({480, 270});

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
      param.cam.look_dir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - param.cam.eye);
      param.cam.right = glm::cross(param.cam.look_dir, param.cam.up);
    }
    return true;
  }
  if(key == GLFW_KEY_D && action == GLFW_REPEAT) {
    param.cam.eye += param.cam.right * param.cam.step;
    if(param.cam.lock_view)
    {
      param.cam.look_dir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - param.cam.eye);
      param.cam.right = glm::cross(param.cam.look_dir, param.cam.up);
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
      param.cam.look_dir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - param.cam.eye);
      param.cam.up = glm::cross(param.cam.right, param.cam.look_dir);
    }

    return true;
  }
  if( key == GLFW_KEY_F && action == GLFW_REPEAT ) {
    param.cam.eye += (-param.cam.up) * param.cam.step;
    if(param.cam.lock_view)
    {
      param.cam.look_dir = glm::normalize(glm::vec3(0.0f, 0.0f, -5.5f) - param.cam.eye);
      param.cam.up = glm::cross(param.cam.right, param.cam.look_dir);
    }
    return true;
  }

  //TODO: we can precompute sin and cos values!
  if( key == GLFW_KEY_U && action == GLFW_REPEAT ) {
    if(param.cam.lock_view) return true;

    glm::vec3 u = param.cam.look_dir, v = param.cam.up;
    param.cam.look_dir = u*param.cam.cos_angle + v*param.cam.sin_angle;
    param.cam.up = -u*param.cam.sin_angle + v*param.cam.cos_angle;

    return true;
  }
  if( key == GLFW_KEY_J && action == GLFW_REPEAT ) {
    if(param.cam.lock_view) return true;

    glm::vec3 u = param.cam.look_dir, v = param.cam.up;
    param.cam.look_dir = u*param.cam.cos_angle + -v*param.cam.sin_angle;
    param.cam.up = u*param.cam.sin_angle + v*param.cam.cos_angle;

    return true;
  }
  if( key == GLFW_KEY_K && action == GLFW_REPEAT ) {
    if(param.cam.lock_view) return true;

    glm::vec3 u = param.cam.right, v = param.cam.look_dir;
    param.cam.right = u*param.cam.cos_angle + -v*param.cam.sin_angle;
    param.cam.look_dir = u*param.cam.sin_angle + v*param.cam.cos_angle;

    return true;
  }
  if( key == GLFW_KEY_H && action == GLFW_REPEAT ) {
    if(param.cam.lock_view) return true;

    glm::vec3 u = param.cam.right, v = param.cam.look_dir;
    param.cam.right = u*param.cam.cos_angle + v*param.cam.sin_angle;
    param.cam.look_dir = -u*param.cam.sin_angle + v*param.cam.cos_angle;

    return true;
  }

  //------------
  if( key == GLFW_KEY_M && action == GLFW_REPEAT ) {
    if(param.cam.lock_view) return true;

    glm::vec3 u = param.cam.right, v = param.cam.up;
    param.cam.right = u*param.cam.cos_angle + -v*param.cam.sin_angle;
    param.cam.up = u*param.cam.sin_angle + v*param.cam.cos_angle;

    return true;
  }
  if( key == GLFW_KEY_N && action == GLFW_REPEAT ) {
    if(param.cam.lock_view) return true;

    glm::vec3 u = param.cam.right, v = param.cam.up;
    param.cam.right = u*param.cam.cos_angle + v*param.cam.sin_angle;
    param.cam.up = -u*param.cam.sin_angle + v*param.cam.cos_angle;

    return true;
  }
  //---------------

  return false;
}

void Engine::draw(NVGcontext *ctx)
{
  Screen::draw(ctx);
}

void Engine::drawContents()
{
  //--------------------------------
  //----------- RENDERING ----------
  //--------------------------------
  //proj and viewport could be precomputed!
  mat4 view = mat4::view(param.cam.eye, param.cam.eye + param.cam.look_dir, param.cam.up);
  mat4 proj = mat4::perspective(param.cam.FoVy, param.cam.FoVx,
                                param.cam.near, param.cam.far);
  mat4 viewport = mat4::viewport(fbo.width(), fbo.height());

  fbo.clearDepthBuffer();
  fbo.clearColorBuffer();

  // TEXTURE SAMPLING
  // [X] Bind a loaded texture to a given texture unit
  // [ ] Bind texture unit id to uniform
  gp.bind_tex_unit(checker, 0);

  gp.upload_uniform(model, view, proj, viewport);
  gp.render(fbo, param.front_face == GL_CCW);

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
