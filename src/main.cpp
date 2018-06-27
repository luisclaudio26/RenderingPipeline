#include <ctime>
#include <iostream>
#include "../include/pipeline/pipeline.h"
#include "../shaders/octreebuilder.h"
#include "../shaders/raymarcher.h"
#include "../shaders/passthrough.h"
#include "../include/mesh.h"
#include <cstdio>

#include "../include/app.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../3rdparty/stb_image_write.h"

int main(int argc, char** args)
{
  /*
  // load geometry
  Mesh mesh; mesh.load_file(args[1]);

  std::vector<float> mesh_data;
  for( auto p : mesh.pos )
    mesh_data.push_back(p);

  // model transformation
  mat4 model;
  mesh.transform_to_center(model);

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

  printf("Cubic bounding box: \n");
  printf("\t(%f, %f, %f) - (%f, %f, %f)\n", bb_min(0), bb_min(1), bb_min(2),
                                            cubic_bb_max(0),
                                            cubic_bb_max(1),
                                            cubic_bb_max(2));

  // -------------------------------
  // -------- BUILD OCTREE ---------
  // -------------------------------
  // setup rendering pipeline
  GraphicPipeline gp;
  VertexShader standard;
  gp.set_vertex_shader(standard);

  OctreeBuilderShader voxelizer;
  OctreeBuilderShader::tree.set_aabb(cubic_bb_min, cubic_bb_max);
  gp.set_fragment_shader(voxelizer);

  // upload data
  gp.upload_data(mesh_data, 3);
  gp.define_attribute("pos", 3, 0);

  // framebuffer object. this must be coherent
  // with our voxel grid resolution; if we want
  // a voxel grid with resolution 128x128x128,
  // our framebuffer must be 128x128 (because
  // each fragment will become a potential voxel
  // element).
  Framebuffer octreeTarget(GRID_RES, GRID_RES);

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

  // ---------------------------------
  // -------- RENDER PREVIEW ---------
  // ---------------------------------
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

  // define viewport and uniforms
  Framebuffer renderTarget(640, 480);
  viewport = mat4::viewport(renderTarget.width(), renderTarget.height());

  eye = vec3(0.5f, 0.8f, +1.5f);
  vec3 look_at(0.0f, 0.0f, 0.0f);
  vec3 up(0.0f, 1.0f, 0.0f);
  view = mat4::view(eye, look_at, up);

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

  // ---------------------------------
  // -------- OUTPUT TO FILE ---------
  // ---------------------------------
  stbi_write_png("../octree.png", octreeTarget.width(), octreeTarget.height(),
                  4, (const void*)octreeTarget.colorBuffer(),
                  sizeof(RGBA8)*octreeTarget.width());


  stbi_write_png("../preview.png", renderTarget.width(), renderTarget.height(),
                  4, (const void*)renderTarget.colorBuffer(),
                  sizeof(RGBA8)*renderTarget.width());
  */

  nanogui::init();

        /* scoped variables */ {
            nanogui::ref<Engine> app = new Engine(args[1]);
            app->drawAll();
            app->setVisible(true);
            nanogui::mainloop();
        }

  nanogui::shutdown();
}
