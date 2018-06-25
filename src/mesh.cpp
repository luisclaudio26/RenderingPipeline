#include "../include/mesh.h"
#include "../include/matrix.h"
#include <cstdio>
#include <iostream>
#include <string>

static std::string basedir_from_path(const std::string& path)
{
  std::size_t found = path.find_last_of('/');
  return path.substr(0, found+1);
}

//----------------------------------
//--------- FROM MESHOBJ.H ---------
//----------------------------------
void Mesh::transform_to_center(mat4& M)
{
  //compute bounding box for this mesh
  vec3 min(FLT_MAX,FLT_MAX,FLT_MAX);
  vec3 max(-FLT_MAX,-FLT_MAX,-FLT_MAX);

  for(int t = 0; t < pos.size(); t += 3)
    for(int j = 0; j < 3; ++j)
    {
      min(j) = std::min( min(j), pos[t+j] );
      max(j) = std::max( max(j), pos[t+j] );
    }

  //compute center of the bounding box.
  //this will be used to calculate the
  //translation to the center.
  vec3 center = (min+max)*0.5f;

  //compute translation from the center to
  //the midpoint in the Z axis between the
  //near and far plane. Also, compute scaling
  //so that the objects bounding box has the
  //width as the frutum in the Z plane
  //THIS ASSUMES THE FOLLOWING CAMERA PARAMETERS:
  // - eye = origin,
  // - look_dir = (0,0,-1),
  // - up = (0,1,0),
  // - near = 1
  // - far = 10
  // - FoV = 45°
  mat4 to_origin( vec4(1.0f, 0.0f, 0.0f, 0.0f),
                  vec4(0.0f, 1.0f, 0.0f, 0.0f),
                  vec4(0.0f, 0.0f, 1.0f, 0.0f),
                  vec4(-center(0), -center(1), -center(2), 1.0f));

  float bb_size_x = max(0) - min(0);
  float half_frustrum = 1.0f; //NearPlane times tan(45)
  float s = half_frustrum / bb_size_x;

  mat4 scale( vec4(s, 0.0f, 0.0f, 0.0f),
              vec4(0.0f, s, 0.0f, 0.0f),
              vec4(0.0f, 0.0f, s, 0.0f),
              vec4(0.0f, 0.0f, 0.0f, 1.0f));

  mat4 from_origin( vec4(1.0f, 0.0f, 0.0f, 0.0f),
                    vec4(0.0f, 1.0f, 0.0f, 0.0f),
                    vec4(0.0f, 0.0f, 1.0f, 0.0f),
                    vec4(0.0f, 0.0f, -5.5f, 1.0f));

  //final transformation
  M = from_origin * scale * to_origin;
}

void Mesh::load_file(const std::string& path)
{
  tinyobj::attrib_t attrib; std::string err;
  std::vector<tinyobj::material_t> materials;
  std::vector<tinyobj::shape_t> shapes;

  std::string basedir = basedir_from_path(path).c_str();

  //load data from .obj file. unfortunatelly
  //this will force the materials to be reloaded, but
  //that's not much of a problem
  tinyobj::LoadObj(&attrib, &shapes, &materials, &err,
                    path.c_str(), basedir.c_str());
  if(!err.empty()) printf("ERROR: %s\n", err.c_str() );

  load_geometry_data(shapes, attrib);
}


void Mesh::load_geometry_data(const std::vector<tinyobj::shape_t>& shapes,
                              const tinyobj::attrib_t& attrib)
{
  //this->tris.clear();

  //load triangles for each shape
  for(auto s = shapes.begin(); s != shapes.end(); ++s)
  {
    unsigned int attrib_offset = 0;

    for(int f_id = 0; f_id < s->mesh.num_face_vertices.size(); ++f_id)
    {
      int f = s->mesh.num_face_vertices[f_id];
      Triangle face;

      //load vertices for this triangle
      //TODO: WILL FAIL IF MESH IS NOT MADE OF TRIANGLES!
      for(int v_id = 0; v_id < f; v_id++)
      {
        tinyobj::index_t v = s->mesh.indices[attrib_offset + v_id];

        float vx = attrib.vertices[3*v.vertex_index + 0];
        float vy = attrib.vertices[3*v.vertex_index + 1];
        float vz = attrib.vertices[3*v.vertex_index + 2];
        pos.push_back(vx);
        pos.push_back(vy);
        pos.push_back(vz);

        //face.v[v_id] = Vec3( vx, vy, vz );

        if( !attrib.texcoords.empty() )
        {
          float tx = attrib.texcoords[2*v.texcoord_index + 0];
          float ty = attrib.texcoords[2*v.texcoord_index + 1];
          uv.push_back(tx);
          uv.push_back(ty);

          //face.uv[v_id] = Vec2( tx, ty );
        }
      }

      //load pointer to material. all shapes share the same materials!
      //WARNING: this won't work unless .obj files with no associated .mtl
      //file set all material IDs to zero!!! also, this is why we need
      //to load materials before
      // int m_id = s->mesh.material_ids[f_id];
      // if(m_id < 0 || m_id > this->materials.size()) printf("m_id = %d\n", m_id);

      // face.material = this->materials[s->mesh.material_ids[f_id]];

      // this->tris.push_back( face );
      attrib_offset += f;
    }
  }
}
