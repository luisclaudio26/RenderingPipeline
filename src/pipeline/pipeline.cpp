#include "../../include/pipeline/pipeline.h"

GraphicPipeline::GraphicPipeline()
  : vbuffer_in(NULL), vbuffer(NULL), vertex_size(0)
{
  vshader.attribs = &attribs;
}

GraphicPipeline::~GraphicPipeline()
{
  if(vbuffer_in) delete[] vbuffer_in;
  if(vbuffer) delete[] vbuffer;
}

void GraphicPipeline::upload_data(const std::vector<float>& data, int vertex_size)
{
  int n_floats = data.size();
  this->vertex_size = vertex_size;
  this->n_vertices = (int)(n_floats / vertex_size);
  this->vbuffer_in_sz = n_floats;

  // delete any previous allocated data
  if(vbuffer_in) delete[] vbuffer_in;

  // copy data into internal vertex buffer
  // TODO: as vbuffer_in doesn't change, we could simply store the pointer
  // instead of copying it (until we need to actually implement this in GPU)
  vbuffer_in = new float[n_floats];
  memcpy((void*)vbuffer_in, (const void*)data.data(), n_floats * sizeof(float));

  // allocate memory for the working vertex buffer.
  // We need extra space because of the extra w parameter
  // we'll need for perpective interpolation.
  vbuffer = new float[n_floats + n_vertices];
}

void GraphicPipeline::define_attribute(const std::string& name, int n_floats, int stride)
{
  Attribute a;
  a.size = n_floats;
  a.stride = stride;

  attribs[name] = a;
}

void GraphicPipeline::upload_uniform(const mat4& model,
                                      const mat4& view,
                                      const mat4& projection,
                                      const mat4& viewport)
{
  this->model = model;
  this->view = view;
  this->projection = projection;
  this->viewport = viewport;

  vshader.model = &this->model;
  vshader.view = &this->view;
  vshader.projection = &this->projection;
  vshader.viewport = &this->viewport;
}

void GraphicPipeline::render(Framebuffer& render_target)
{
  // 1. [X] Vertex shader execution
  // 2. [X] Triangle clipping
  // 3. [X] Perspective division
  // 4. [X] Triangle culling
  // 5. [X] Rasterization (including Fragment shader)

  // vertex processing stage. v stores the starting address of a new
  // vertex and v_id the actual "id" of this very same vertex
  for(int v = 0, v_id = 0; v < vbuffer_in_sz; v += vertex_size, ++v_id)
  {
    //extract vertex data and send to vertex shader
    const float* vertex_data = &vbuffer_in[v];
    float* target = &vbuffer[v_id*(vertex_size+1)];

    // TODO: THE FIRST 4 POSITIONS SHOULD CORRESPOND TO
    // GL_POSITION'S X Y Z W!!! The vertex shader must
    // guarantee that
    vshader.launch(vertex_data, vertex_size, target);
  }

  // triangle clipping
  this->vbuffer_sz = vbuffer_in_sz + n_vertices;
  this->tri_sz = 3*(vertex_size+1);
  int non_clipped = 0;
  for(int t = 0; t < vbuffer_sz; t += tri_sz)
  {
    bool discard = false;

    // a primitice is discarded if any of
    // its vertices is not visible
    for(int v_id = 0; v_id < 3; ++v_id)
    {
      float* v = &vbuffer[t + v_id*(vertex_size+1)];
      float w = v[3];

      // discard primitives behind the camera
      if( w <= 0 )
      {
        discard = true;
        break;
      }

      // clip primitives outside frustum
      if(std::fabs(v[0]) > w || std::fabs(v[1]) > w || std::fabs(v[2]) > w)
      {
        discard = true;
        break;
      }
    }

    // if this primitive has survived clipping, copy it to
    // the vertex buffer...
    // ...but well, things are a bit more complicated. We could
    // have different vertex buffers and make copying simpler,
    // but for this first, single-threaded version, we just send
    // surviving primitives to the beginning of vbuffer, e.g.,
    // suppose we have:
    //
    // vbuffer: [ V0 V1 V2 V3 V4 ... Vn ]
    // vbuffer_sz: n
    //
    // and we found that V0, V1 and V4 survived the clipping while
    // V2, V3, V5, V6, ... Vn did not; so we copy V4 to the beginning
    // of the vector and change its size:
    //
    // vbuffer: [ V0 V1 V4 V3 V4 ... Vn ]
    // vbuffer_sz: 3
    //
    // A GPU implementation of this would need a new buffer (or
    // intermediate per-block cache) to handle this without needing
    // lots of sync barriers.
    if(!discard)
    {
      float *target = &vbuffer[non_clipped];
      memmove(target, &vbuffer[t], tri_sz*sizeof(float));
      non_clipped += tri_sz;
    }
  }

  // update vertex buffer size (there will be garbage by the
  // end of the vector, but this isn't a problem).
  vbuffer_sz = non_clipped;

  // perspective division
  for(int v_id = 0; v_id < vbuffer_sz; v_id += (vertex_size+1))
  {
    float *v = &vbuffer[v_id];
    float w = v[3];

    for(int i = 0; i < vertex_size+1; ++i) v[i] /= w;
  }

  // triangle culling
  //TODO: In OpenGL architecture, culling happens in the primitive
  //assembly stage, which is the first part of rasterization
  int non_culled = 0;
  for(int t = 0; t < vbuffer_sz; t += tri_sz)
  {
    float *v0 = &vbuffer[t + 0*(vertex_size+1)];
    float *v1 = &vbuffer[t + 1*(vertex_size+1)];
    float *v2 = &vbuffer[t + 2*(vertex_size+1)];

    vec3 v0_(v0[0], v0[1], 1.0f);
    vec3 v1_(v1[0], v1[1], 1.0f);
    vec3 v2_(v2[0], v2[1], 1.0f);

    //compute cross product p = v0v1 X v0v2;
    //if p is pointing outside the screen, v0v1v2 are defined
    //in counter-clockwise order. then, reject or accept this
    //triangle based on the param.front_face flag.
    vec3 c = (v1_-v0_).cross(v2_-v0_);

    // cull clockwise triangles (z component of vector product
    // c is facing -z). We do the same in place update of the
    // vertex buffer as we did in the primitive clipping step
    if(c(2) >= 0)
    {
      float* target = &vbuffer[non_culled];
      memmove(target, &vbuffer[t], tri_sz*sizeof(float));
      non_culled += tri_sz;
    }
  }

  // update vbuffer size
  vbuffer_sz = non_culled;

  this->rasterization(render_target);
}

// --------------------------------
// --------------------------------
static inline void sub_vertex(const float* lhs, const float* rhs,
                              float* target, int vertex_sz)
{
  for(int i = 0; i < vertex_sz; ++i)
    target[i] = lhs[i] - rhs[i];
}

static inline void scalar_vertex(const float* lhs, float k,
                                  float* target, int vertex_sz)
{
  for(int i = 0; i < vertex_sz; ++i)
    target[i] = lhs[i] * k;
}

static inline void inc_vertex(float* target, float* inc, int vertex_sz)
{
  for(int i = 0; i < vertex_sz; ++i)
    target[i] += inc[i];
}

void GraphicPipeline::rasterization(Framebuffer& render_target)
{
  #define ROUND(x) ((int)(x + 0.5f))
  #define SWAP(a,b) { float* aux = b; b = a; a = aux; }
  #define X(x) (x[0])
  #define Y(x) (x[1])
  #define Z(x) (x[2])
  #define W(x) (x[3])

  // preallocate buffers
  float *v0 = new float[vertex_size+1];
  float *v1 = new float[vertex_size+1];
  float *v2 = new float[vertex_size+1];

  float* dV0_dy = new float[vertex_size+1];
  float* dV1_dy = new float[vertex_size+1];
  float* dV2_dy = new float[vertex_size+1];
  
  float *dV_dx = new float[vertex_size+1];  //horizontal increment
  float *f = new float[vertex_size+1];      //bilinearly interpolated fragment
  float *frag = new float[vertex_size+1];   //persective interpolated fragment

  for(int t = 0; t < vbuffer_sz; t += tri_sz)
  {
    float *v0_ = &vbuffer[t + 0*(vertex_size+1)];
    float *v1_ = &vbuffer[t + 1*(vertex_size+1)];
    float *v2_ = &vbuffer[t + 2*(vertex_size+1)];

    //we need x and y positions mapped to the viewport and
    //with integer coordinates, otherwise we'll have displacements
    //for start and end which are huge when 0 < dy < 1;
    //these cases must be treated as straight, horizontal lines.
    vec4 pos0 = viewport*vec4(v0_[0], v0_[1], 1.0f, 1.0f);
    X(v0) = ROUND(pos0(0)); Y(v0) = ROUND(pos0(1));
    Z(v0) = v0_[2]; W(v0) = v0_[vertex_size];
    memcpy(&v0[4], &v0_[4], (vertex_size+1-4)*sizeof(float));

    vec4 pos1 = viewport*vec4(v1_[0], v1_[1], 1.0f, 1.0f);
    X(v1) = ROUND(pos1(0)); Y(v1) = ROUND(pos1(1));
    Z(v1) = v1_[2]; W(v1) = v1_[vertex_size];
    memcpy(&v1[4], &v1_[4], (vertex_size+1-4)*sizeof(float));

    vec4 pos2 = viewport*vec4(v2_[0], v2_[1], 1.0f, 1.0f);
    X(v2) = ROUND(pos2(0)); Y(v2) = ROUND(pos2(1));
    Z(v2) = v2_[2]; W(v2) = v2_[vertex_size];
    memcpy(&v2[4], &v2_[4], (vertex_size+1-4)*sizeof(float));

    //order vertices by y coordinate
    if( Y(v0) > Y(v1) ) SWAP(v0, v1);
    if( Y(v0) > Y(v2) ) SWAP(v0, v2);
    if( Y(v1) > Y(v2) ) SWAP(v1, v2);

    //these dVdy_ variables define how much we must
    //increment v when increasing one unit in y, so
    //we can use this to compute the start and end
    //boundaries for rasterization. Notice that not
    //only this defines the actual x coordinate of the
    //fragment in the scanline, but all the other
    //attributes. Also, notice that y is integer and
    //thus if we make dy0 = (v1.y-v0.y) steps in y, for intance,
    //incrementing v0 with dVdy0 at each step, by the end of
    //the dy steps we'll have:
    //
    // v0 + dy0 * dVdy0 = v0 + dy0*(v1-v0)/dy0 = v0 + v1 - v0 = v1
    //
    //which is exactly what we want, a linear interpolation
    //between v0 and v1 with dy0 steps
    //TODO: Preallocate dVx_dy in the upload_data() routine!
    sub_vertex(v1, v0, dV0_dy, vertex_size+1);
    scalar_vertex(dV0_dy, 1.0f/(Y(v1)-Y(v0)), dV0_dy, vertex_size+1);

    sub_vertex(v2, v0, dV1_dy, vertex_size+1);
    scalar_vertex(dV1_dy, 1.0f/(Y(v2)-Y(v0)), dV1_dy, vertex_size+1);

    sub_vertex(v2, v1, dV2_dy, vertex_size+1);
    scalar_vertex(dV2_dy, 1.0f/(Y(v2)-Y(v1)), dV2_dy, vertex_size+1);

    float *start, *end;
    float *dStart_dy, *dEnd_dy;

    //this will tell us whether we should change dStart_dy
    //or dEnd_dy to the next active edge (dV2_dy) when we
    //reach halfway the triangle
    float* next_active_edge;

    //decide start/end edges. If v1 is to the left
    //side of the edge connecting v0 and v2, then v0v1
    //is the starting edge and v0v2 is the ending edge;
    //if v1 is to the right, it is the contrary.
    //the v0v1 edge will be substituted by the v1v2 edge
    //when we reach the v1 vertex while scanlining, so we
    //store which of the start/end edges we should replace
    //with v1v2.
    vec3 right_side = vec3(X(v1)-X(v0), Y(v1)-Y(v0), 0.0f).cross(vec3(X(v2)-X(v0), Y(v2)-Y(v0), 0.0f));
    if( right_side(2) > 0.0f )
    {
      dEnd_dy = dV0_dy;
      dStart_dy = dV1_dy;
      next_active_edge = dEnd_dy;
    }
    else
    {
      dEnd_dy = dV1_dy;
      dStart_dy = dV0_dy;
      next_active_edge = dStart_dy;
    }

    //handle flat top triangles
    if( Y(v0) == Y(v1) )
    {
      //switch active edge and update
      //starting and ending points
      if( X(v0) < X(v1) )
      {
        dEnd_dy = dV2_dy;
        start = v0; end = v1;
      }
      else
      {
        dStart_dy = dV2_dy;
        start = v1; end = v0;
      }
    }
    else start = end = v0;

    //loop over scanlines
    for(int y = Y(v0); y <= Y(v2); ++y)
    {
      //starting and ending points for scanline rasterization
      int s = ROUND(X(start)), e = ROUND(X(end));

      // compute horizontal increment dV_dx
      sub_vertex(end, start, dV_dx, vertex_size+1);
      scalar_vertex(dV_dx, 1.0f/(e-s), dV_dx, vertex_size+1);

      // initialize the actual fragment
      memcpy(f, start, (vertex_size+1)*sizeof(float));

      for(int x = s; x <= e; ++x)
      {
        //in order to draw only the edges, we skip this
        //the scanline rasterization in all points but
        //the extremities. TODO
        //if(param.draw_mode == GL_LINE && (x != s && x != e)) continue;

        // To better represent what the pipeline does, we should, in the
        // following order:
        //
        // 1) perform early fragment tests at this point (which include
        // depth buffering, scissor testing and stencil buffering, for
        // for example), which decide whether this fragment will live
        // or not. Notice that at this point, a fragment is the set of
        // attributes interpolated by the rasterizer;
        //
        // 2) evaluate fragment shader to compute a pixel sample from the
        // fragment's attributes;
        //
        // 3) perform per-sample operations like alpha
        // blending using the sample computed in the previous stage;
        //
        // It is interesting to notice that, as of version 4.6, the OpenGL
        // specification calls "per-fragment
        // operations" both early fragment tests (which MAY be performed
        // before or after fragment shader evaluation, but executing before
        // allows us to discard fragments without evaluating them) and
        // per-sample operations (like like alpha blending, dithering and
        // sRGB conversions), which MUST be performed after fragment shader
        // evaluation because we need a pixel sample.

        // Here we mixed things in the same code for simplicity
        if( Z(f) < render_target.getDepthBuffer(y,x) )  // early fragment tests
        {
          render_target.setDepthBuffer(y, x, Z(f));     // early fragment tests

          // perspectively-correct interpolation of attributes
          scalar_vertex(f, 1.0f/W(f), frag, vertex_size+1);

          float color[4];
          // ... fragment shader comes here, writing to COLOR[4] ...
          color[0] = 1.0f;
          color[1] = 0.0f;
          color[2] = 0.0f;
          color[3] = 1.0f;

          // write to framebuffer
          RGBA8 color_ubyte;
          color_ubyte.r = std::min(255, (int)(color[0]*255.0f));
          color_ubyte.g = std::min(255, (int)(color[1]*255.0f));
          color_ubyte.b = std::min(255, (int)(color[2]*255.0f));
          color_ubyte.a = std::min(255, (int)(color[3]*255.0f));

          render_target.setColorBuffer(y, x, color_ubyte);
        }

        inc_vertex(f, dV_dx, vertex_size+1);
      }

      //switch active edges if halfway through the triangle
      //This MUST be done before incrementing, otherwise
      //once we reached v1 we would pass through it and
      //start coming back only in the next step, causing
      //the big "leaking" triangles!
      if( y == (int)Y(v1) ) next_active_edge = dV2_dy;

      //increment bounds
      inc_vertex(start, dStart_dy, vertex_size+1);
      inc_vertex(end, dEnd_dy, vertex_size+1);
    }
  }

  // clean buffers
  delete[] v0;
  delete[] v1;
  delete[] v2;
  delete[] dV0_dy;
  delete[] dV1_dy;
  delete[] dV2_dy;
  delete[] f;
  delete[] frag;
  delete[] dV_dx;
}
