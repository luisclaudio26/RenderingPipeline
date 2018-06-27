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

const int GRID_RES = 128;

void Engine::draw(NVGcontext *ctx)
{
  Screen::draw(ctx);
}

void Engine::drawContents()
{
  //-------------------------------------
  //----------- OCTREE BUILDUP ----------
  //-------------------------------------
  clock_t start = clock();

  /*
  //proj and viewport could be precomputed!
  mat4 view = mat4::view(param.cam.eye, param.cam.eye + param.cam.look_dir, param.cam.up);
  mat4 proj = mat4::orthogonal(-3.0f, 3.0f, -3.0f, 3.0f, param.cam.near, param.cam.far);
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
  */

  // ---------------------------------
  // -------- RENDER PREVIEW ---------
  // ---------------------------------
  /*
  GraphicPipeline renderer;

  // a simple quad for simply being able to invoke a fragment
  // shader for each pixel
  std::vector<float> quad;
  quad.push_back(-1.0f); quad.push_back(-1.0f);
  quad.push_back(+1.0f); quad.push_back(-1.0f);
  quad.push_back(+1.0f); quad.push_back(+1.0f);
  quad.push_back(-1.0f); quad.push_back(-1.0f);
  quad.push_back(+1.0f); quad.push_back(+1.0f);
  quad.push_back(-1.0f); quad.push_back(+1.0f);
  renderer.upload_data(quad, 2);
  renderer.define_attribute("pos", 2, 0);

  // shader setting
  PassthroughShader passthrough;
  RayMarcherShader raymarch(OctreeBuilderShader::tree);
  renderer.set_vertex_shader(passthrough);
  renderer.set_fragment_shader(raymarch);
  */

  // define viewport and uniforms
  //Framebuffer renderTarget(buffer_width, buffer_height);
  mat4 viewport = mat4::viewport(renderTarget.width(), renderTarget.height());

  vec3 eye(0.5f, 0.8f, +1.5f);
  vec3 look_at(0.0f, 0.0f, 0.0f);
  vec3 up(0.0f, 1.0f, 0.0f);
  mat4 view = mat4::view(eye, look_at, up);

  // TODO: this can be computed from view but I'm too lazy
  vec3 w = (eye-look_at).unit();
  vec3 u = (up.cross(w)).unit();
  vec3 v = w.cross(u);
  mat4 inv_view( vec4(u(0), u(1), u(2), 0.0f),
                  vec4(v(0), v(1), v(2), 0.0f),
                  vec4(w(0), w(1), w(2), 0.0f),
                  vec4(eye.dot(u), eye.dot(v), eye.dot(w), 1.0f));

  renderer.set_viewport(viewport);
  renderer.upload_uniform("eye", eye.data(), 3);
  renderer.upload_uniform("view", view.data(), 16);
  renderer.upload_uniform("inv_view", inv_view.data(), 16);

  // clear and render
  renderTarget.clearDepthBuffer();
  renderTarget.clearColorBuffer();
  renderer.render(renderTarget);

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
                  renderTarget.colorBuffer());

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
  renderTarget.resizeBuffer(buffer_width, buffer_height);
}

Engine::Engine(const char* path)
  : nanogui::Screen(Eigen::Vector2i(DEFAULT_WIDTH, DEFAULT_HEIGHT), "NanoGUI Test"),
    buffer_width(DEFAULT_WIDTH), buffer_height(DEFAULT_HEIGHT),
    //raymarch(RayMarcherShader(OctreeBuilderShader::tree))
    raymarch(OctreeBuilderShader::tree)
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
  // It's a bit dumb to copy each element in a for loop
  // using push_back(), but this is just because our meshes
  // have only position information (in the general case,
  // at least texture coordinates would be present and we
  // would need to store them inside mesh_data also)
  mesh.load_file( std::string(path) );
  std::vector<float> mesh_data;
  for( auto p : mesh.pos ) mesh_data.push_back(p);
  mesh.transform_to_center(model);

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

  renderTarget.resizeBuffer(buffer_width, buffer_height);
  octreeTarget.resizeBuffer(GRID_RES, GRID_RES);

  //--------------------------------------
  //----------- Shader options -----------
  //--------------------------------------
  gp.set_fragment_shader(voxelizer);
  gp.set_vertex_shader(standard);
  renderer.set_fragment_shader(raymarch);
  renderer.set_vertex_shader(passthrough);

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

  // ---------------------------------
  // ---------- Upload data ----------
  // ---------------------------------
  gp.upload_data(mesh_data, 3);
  gp.define_attribute("pos", 3, 0);

  // a simple quad so we can invoke the fragment shader for each pixel
  std::vector<float> quad;
  quad.push_back(-1.0f); quad.push_back(-1.0f);
  quad.push_back(+1.0f); quad.push_back(-1.0f);
  quad.push_back(+1.0f); quad.push_back(+1.0f);
  quad.push_back(-1.0f); quad.push_back(-1.0f);
  quad.push_back(+1.0f); quad.push_back(+1.0f);
  quad.push_back(-1.0f); quad.push_back(+1.0f);
  renderer.upload_data(quad, 2);
  renderer.define_attribute("pos", 2, 0);

  // upload triangles which we'll use to render the final
  // image and the corresponding texture coordinates
  Eigen::MatrixXf quad_gl(2, 6);
  quad_gl.col(0)<<-1.0, -1.0;
  quad_gl.col(1)<<+1.0, -1.0;
  quad_gl.col(2)<<+1.0, +1.0;
  quad_gl.col(3)<<-1.0, -1.0;
  quad_gl.col(4)<<+1.0, +1.0;
  quad_gl.col(5)<<-1.0, +1.0;

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
  shader.uploadAttrib<Eigen::MatrixXf>("quad_pos", quad_gl);
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
