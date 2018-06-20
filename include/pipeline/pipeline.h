#ifndef PIPELINE_H
#define PIPELINE_H

#include <vector>
#include <string>
#include <map>
#include "../matrix.h"
#include "framebuffer.h"
#include "vertexshader.h"
#include "fragmentshader.h"
#include "attribute.h"
#include "texsampler.h"

class GraphicPipeline
{
private:
  // Array of attributes. In a first moment we'll work
  // with FLOAT attributes only because it is easier to
  // manage pointer and interpolation if all attributes
  // have the same type. For the future we should support
  // different types.
  // Notice that this array stores only the attributes
  // and not the 1.0 value we need to later perform
  // perspective interpolation. Also, vbuffer_in won't
  // change, as we might need it to perform subsequent
  // calls to the graphic pipeline to render the same
  // object without changing its data.
  const float *vbuffer_in;
  int vbuffer_in_sz; //number of floats

  // Vertex size. The DATA array is comprised of vertex
  // attributes all aligned; vertex_size defines how many
  // attributes make up a vertex WITHOUT including the
  // attribute 1.0 which each vertex should have in order
  // to properly perform interpolation!
  int vertex_size;
  int n_vertices;

  // this is the actual vertex buffer we'll be working in.
  // vbuffer_elem_sz defines the size of a single element
  // inside vbuffer (i.e., it includes the W parameter used
  // for interpolation and vertex position is stored as 4 floats).
  // All sizes here are expressed in NUMBER OF FLOATS, so
  // if each vertex has XYZW position only and we have 10 vertices,
  // vbuffer_elem_sz = 4, tri_sz = 3*4 = 12 and vbuffer_sz = 10*12 = 120
  float *vbuffer;
  int vbuffer_sz;
  int vbuffer_elem_sz;
  int tri_sz;

  // Attributes can be accessed within the vbuffer_in
  // by knowing its size and stride. We store this info
  // so it can be accessed later by the shaders
  std::map<std::string, Attribute> attribs;

  // each texture unit is a texture sampler which can
  // receive a TEXTURE and some texture-related configuration
  // (filtering method and wrap function). Shaders access
  // this via a texture unit ID which is usually passed as
  // uniform.
  std::vector<TextureSampler> tex_units;

  // Uniform variables are stored inside a contiguous memory
  // area. OpenGL doesn't require one to set how many memory
  // will be needed to store uniforms, so we use a vector here
  // to reproduce this behaviour.
  // If we know the total size of uniforms we'll need we
  // can .reserve() this vector.
  float *uniform_data; int uniform_data_index;
  std::map<std::string, Attribute> uniforms;

  /*
  mat4 model, view, projection, viewport;
  vec3 eye, light;
  */

  // viewport transformation
  mat4 viewport;

  // shaders. Pipeline is not responsible for shader deletion! Client code
  // must delete it somehow.
  // Shaders are actually CLOSURES (i.e. functions with the needed context
  // to execute them), because we need pointers to the loaded uniforms,
  // attributes and texture units. For the moment, the easiest way to implement
  // this is using abstract classes with virtual methods, which is extremely
  // inefficient (and no implementable in GPUs). How to better implement this?
  // Maybe defining uniforms/attributes/texture units as global and thus shaders
  // would have access to it, or simply pass everything as argument (which would
  // make the function more costly).
  VertexShader *vshader;
  FragmentShader *fshader;

  // fixed stages
  void vertex_processing();
  int primitive_clipping();
  void perspective_division();
  int primitive_culling(bool cull_back);
  void rasterization(Framebuffer& render_target, bool fill);

public:
  GraphicPipeline();
  ~GraphicPipeline();

  // set shaders. References cannot be const because we'll
  // modify the shader objects by setting their Uniform/Attribute
  // pointers to the ones allocated by Pipeline
  void set_fragment_shader(FragmentShader& fshader);
  void set_vertex_shader(VertexShader& vshader);

  // binds a given texture object to the target unit, so
  // when using the sampler2D method of this texture unit
  // we'll be sampling the binded texture.
  // this is simple binding, no copies involved, so deleting
  // the texture and then trying to use it will probably crash!
  void bind_tex_unit(const Texture& tex, int unit);

  // this should be as flexible as the attribute system,
  // but for simplicity we're gonna store only the main
  // uniform matrices.
  // TODO: This will disappear once we have the uniform
  // variable management system working and will be replaced
  // by something that receives a variable and a string ID,
  // just like the attributes
  void upload_uniform(const std::string& name, const float* data, int n_floats);
  void upload_uniform(const std::string& name, float d);

  // define viewport. this is not a uniform variable because
  // we need to access it outside the programmable shaders.
  void set_viewport(const mat4& viewport);

  // upload a set of floats containing all the attributes
  // contiguously defined. This can be slow because we make
  // a copy of this data, as our vertex buffer will be
  // modified many times during the pipeline.
  // Besides the actual data, vertex_size informs how many
  // floats makes up a vertex (for example, if a vertex has
  // position and normal information as vec3, vertex_size is 6).
  void upload_data(const std::vector<float>& data, int vertex_size);

  // Define an attribute inside the data uploaded. This
  // attribute is defined by telling how many floats it
  // is comprised of and the stride within the vertex.
  void define_attribute(const std::string& name, int n_floats, int stride);

  // After setting the attributes and uniforms,
  // render sends them through the pipeline and
  // stores the final result in the target Framebuffer
  void render(Framebuffer& target, bool cull_back = true, bool fill = true);
};

#endif
