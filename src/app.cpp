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

static void print_node(const Node* n)
{
  if(!n) return;
  printf("[%f %f %f | %f %f %f]\n", n->Internal.min_x, n->Internal.min_y, n->Internal.min_z,
                                    n->Internal.max_x, n->Internal.max_y, n->Internal.max_z);
  for(int i = 0; i < 8; ++i)
    print_node(n->Internal.children[i]);
}

// ------------------------------
const int GRID_RES = 32;

void Engine::draw(NVGcontext *ctx)
{
  Screen::draw(ctx);
}

void Engine::compute_octree()
{
  // compute scene bounding box
  vec3 bb_min(FLT_MAX,FLT_MAX,FLT_MAX), bb_max(-FLT_MAX,-FLT_MAX,-FLT_MAX);
  for(int t = 0; t < mesh.pos.size(); t += 3)
  {
    vec4 p = model * vec4(mesh.pos[t+0],
                          mesh.pos[t+1],
                          mesh.pos[t+2],
                          1.0f);

    for(int j = 0; j < 3; ++j)
    {
      bb_min(j) = std::fmin(bb_min(j), p(j));
      bb_max(j) = std::fmax(bb_max(j), p(j));
    }
  }

  printf("Bounding box: \n");
  printf("\t(%f, %f, %f) - (%f, %f, %f)\n", bb_min(0), bb_min(1), bb_min(2),
                                            bb_max(0), bb_max(1), bb_max(2));

  // compute minimal CUBIC bounding box which will be used
  // to define the rasterization limits
  vec3 diff = bb_max - bb_min;
  int greatest_axis = 0;

  for(int i = 0; i < 3; ++i)
    if(diff(i) > diff(greatest_axis))
      greatest_axis = i;

  // side of the cubic bounding box
  float l = diff(greatest_axis);

  vec3 cubic_bb_min = bb_min;
  vec3 cubic_bb_max = bb_min + vec3(l,l,l);

  OctreeBuilderShader::tree.set_aabb(cubic_bb_min, cubic_bb_max);

  printf("Cubic bounding box: \n");
  printf("\t(%f, %f, %f) - (%f, %f, %f)\n", bb_min(0), bb_min(1), bb_min(2),
                                            cubic_bb_max(0),
                                            cubic_bb_max(1),
                                            cubic_bb_max(2));

  // setup common matrices
  float half_l = l * 0.5f;
  mat4 viewport = mat4::viewport(octreeTarget.width(), octreeTarget.height());
  mat4 proj = mat4::orthogonal(-half_l, half_l, -half_l, half_l, 0.0f, l + 0.5f);

  //XY view
  vec3 eye = cubic_bb_min;
  eye(0) = cubic_bb_min(0) + half_l;
  eye(1) = cubic_bb_min(1) + half_l;
  eye(2) = cubic_bb_min(2);
  mat4 view = mat4::view(eye, eye + vec3(0.0f, 0.0f, +1.0f), vec3(0.0f, 1.0f, 0.0f));

  octreeTarget.clearDepthBuffer();
  octreeTarget.clearColorBuffer();
  gp.set_viewport(viewport);

  gp.upload_uniform("view", view.data(), 16);
  gp.upload_uniform("model", model.data(), 16);
  gp.upload_uniform("proj", proj.data(), 16);

  gp.render(octreeTarget, false);

  //XZ view
  eye = cubic_bb_min;
  eye(0) = cubic_bb_min(0) + half_l;
  eye(1) = cubic_bb_min(1);
  eye(2) = cubic_bb_min(2) + half_l;
  view = mat4::view(eye, eye + vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, +1.0f));

  octreeTarget.clearDepthBuffer();
  octreeTarget.clearColorBuffer();
  gp.set_viewport(viewport);

  gp.upload_uniform("view", view.data(), 16);
  gp.upload_uniform("model", model.data(), 16);
  gp.upload_uniform("proj", proj.data(), 16);

  gp.render(octreeTarget, false);

  //YZ view
  eye = cubic_bb_min;
  eye(0) = cubic_bb_min(0);
  eye(1) = cubic_bb_min(1) + half_l;
  eye(2) = cubic_bb_min(2) + half_l;
  view = mat4::view(eye, eye + vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));

  octreeTarget.clearDepthBuffer();
  octreeTarget.clearColorBuffer();
  gp.set_viewport(viewport);

  gp.upload_uniform("view", view.data(), 16);
  gp.upload_uniform("model", model.data(), 16);
  gp.upload_uniform("proj", proj.data(), 16);

  gp.render(octreeTarget, false);

  // -------
  print_node(&OctreeBuilderShader::tree.root);
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

  //vec3 eye(0.5f, 0.8f, +1.5f);
  //vec3 look_at(0.0f, 0.0f, 0.0f);
  //vec3 up(0.0f, 1.0f, 0.0f);
  mat4 view = mat4::view(param.cam.eye,
                          param.cam.eye + param.cam.look_dir,
                          param.cam.up);

  // TODO: this can be computed from view but I'm too lazy
  vec3 w = param.cam.eye.unit();
  vec3 u = (param.cam.up.cross(w)).unit();
  vec3 v = w.cross(u);
  mat4 inv_view( vec4(u(0), u(1), u(2), 0.0f),
                  vec4(v(0), v(1), v(2), 0.0f),
                  vec4(w(0), w(1), w(2), 0.0f),
                  vec4(param.cam.eye.dot(u), param.cam.eye.dot(v), param.cam.eye.dot(w), 1.0f));

  renderer.set_viewport(viewport);
  renderer.upload_uniform("eye", param.cam.eye.data(), 3);
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
  fflush(stdout);
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
    raymarch(OctreeBuilderShader::tree)
{
  // --------------------------------
  // --------- Scene setup ----------
  // --------------------------------
  float angle = 0.0174533f;
  param.cam.eye = vec3(0.0f, 0.0f, 2.0f);
  param.cam.up = vec3(0.0f, 1.0f, 0.0f);
  param.cam.cos_angle = (float)cos(angle);
  param.cam.sin_angle = (float)sin(angle);
  param.cam.look_dir = vec3(0.0f, 0.0f, -1.0f);
  param.cam.right = vec3(1.0f, 0.0f, 0.0f);
  param.cam.step = 0.1f;
  param.cam.near = 0.5f;
  param.cam.far = 5.0f;
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

  // ------------------------------------
  // ---------- Compute octree ----------
  // ------------------------------------
  compute_octree();

  performLayout();
}

bool Engine::keyboardEvent(int key, int scancode, int action, int modifiers)
{
  if (Screen::keyboardEvent(key, scancode, action, modifiers)) return true;

  //camera movement
  if(key == GLFW_KEY_Q && action == GLFW_REPEAT)
  {
    param.cam.eye = vec3(0.0f, 0.0f, 3.0f);
    param.cam.look_dir = vec3(0.0f, 0.0f, -1.0f);
    param.cam.up = vec3(0.0f, 1.0f, 0.0f);
    printf("XY view\n");
    return true;
  }

  if(key == GLFW_KEY_W && action == GLFW_REPEAT)
  {
    param.cam.eye = vec3(0.0f, 3.0f, 0.0f);
    param.cam.look_dir = vec3(0.0f, -1.0f, 0.0f);
    param.cam.up = vec3(0.0f, 0.0f, 1.0f);
    printf("XZ view\n");
    return true;
  }

  if( key == GLFW_KEY_E && action == GLFW_REPEAT )
  {
    param.cam.eye = vec3(3.0f, 0.0f, 0.0f);
    param.cam.look_dir = vec3(-1.0f, 0.0f, 0.0f);
    param.cam.up = vec3(0.0f, 1.0f, 0.0f);
    printf("YZ view\n");
    return true;
  }

  return false;
}
