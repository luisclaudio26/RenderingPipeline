#include <ctime>
#include <iostream>
#include "../include/pipeline/pipeline.h"
#include "../shaders/octreebuilder.h"
#include "../include/mesh.h"
#include <cstdio>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../3rdparty/stb_image_write.h"

const int GRID_RES = 128;

int main(int argc, char** args)
{
  // load geometry
  Mesh mesh; mesh.load_file(args[1]);

  std::vector<float> mesh_data;
  for( auto p : mesh.pos )
    mesh_data.push_back(p);

  // model transformation
  mat4 model;
  mesh.transform_to_center(model);

  // setup rendering pipeline
  GraphicPipeline gp;
  VertexShader vshader;
  OctreeBuilderShader fshader;
  gp.set_fragment_shader(fshader);
  gp.set_vertex_shader(vshader);

  // framebuffer object. this must be coherent
  // with our voxel grid resolution; if we want
  // a voxel grid with resolution 128x128x128,
  // our framebuffer must be 128x128 (because
  // each fragment will become a potential voxel
  // element).
  Framebuffer fbo(GRID_RES, GRID_RES);

  // upload data
  gp.upload_data(mesh_data, 3);
  gp.define_attribute("pos", 3, 0);

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
  float half_l = l * 0.5f;
  mat4 viewport = mat4::viewport(fbo.width(), fbo.height());
  mat4 proj = mat4::orthogonal(-half_l, half_l, -half_l, half_l, 0.5f, l + 1.5f);

  //XY view
  vec3 eye = cubic_bb_min;
  eye(0) = cubic_bb_min(0) + half_l;
  eye(1) = cubic_bb_min(1) + half_l;
  eye(2) = cubic_bb_min(2) - 1.0f;
  mat4 view = mat4::view(eye, eye + vec3(0.0f, 0.0f, +1.0f), vec3(0.0f, 1.0f, 0.0f));

  fbo.clearDepthBuffer();
  fbo.clearColorBuffer();
  gp.set_viewport(viewport);

  gp.upload_uniform("view", view.data(), 16);
  gp.upload_uniform("model", model.data(), 16);
  gp.upload_uniform("proj", proj.data(), 16);

  gp.render(fbo);

  // ---------------------------------
  // -------- OUTPUT TO FILE ---------
  // ---------------------------------
  stbi_write_png("../out.png", fbo.width(), fbo.height(),
                  4, (const void*)fbo.colorBuffer(), sizeof(RGBA8)*fbo.width());
}
