#include <ctime>
#include <iostream>
#include "../include/pipeline/pipeline.h"
#include "../shaders/octreebuilder.h"
#include "../include/mesh.h"

const int GRID_RES = 128;

int main(int argc, char** args)
{
  // load geometry
  Mesh mesh; mesh.load_file("../data/cube_text.in");
  std::vector<float> mesh_data;
  int n_vert = mesh.mPos.cols();

  for(int i = 0; i < n_vert; ++i)
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
  // our framebuffer must be 128x128x128 (because
  // each fragment will become a potential voxel
  // element).
  Framebuffer fbo; fbo.resizeBuffer(GRID_RES, GRID_RES);

  // upload data
  gp.upload_data(mesh_data, 8);
  gp.define_attribute("pos", 3, 0);
  gp.define_attribute("normal", 3, 3);
  gp.define_attribute("texcoord", 2, 6);

  // compute scene bounding box
  vec3 bb_min(FLT_MAX,FLT_MAX,FLT_MAX), bb_max(-FLT_MAX,-FLT_MAX,-FLT_MAX);
  for(int i = 0; i < n_vert; ++i)
  {
    Eigen::Vector3f p_ = mesh.mPos.col(i);
    vec4 p = model * vec4(p_(0), p_(1), p_(2), 1.0f);

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

  // -------------------------------
  // -------- BUILD OCTREE ---------
  // -------------------------------
  mat4 viewport = mat4::viewport(fbo.width(), fbo.height());
  mat4 view = mat4::view(param.cam.eye, param.cam.eye + param.cam.look_dir, param.cam.up);
  mat4 proj = mat4::orthogonal(-3.0f, 3.0f, -3.0f, 3.0f, param.cam.near, param.cam.far);


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


}
