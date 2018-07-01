#ifndef OCTREE_H
#define OCTREE_H

#include "../include/matrix.h"

// in order to build a compact octree, nodes should
// allocated on demand once we request a fragment to be
// inserted into the tree. nodes should be allocated
// sequentially if possible so that memory jumps are not
// huge. as described in the original article, leaves store
// a pointer to attributes in texture memory, but we won't
// make any distinction here. in the end, this is just a
// basic octree, far from the sparse voxel tree devised in
// the article.
const int MAX_DEPTH = 7;

struct Node
{
  // bounding box
  float min_x, min_y, min_z;
  float max_x, max_y, max_z;

  union
  {
    struct
    {
      // splitting point of this node.
      // we can't store a vec3, so we do
      // this explicitly as 3 floats.
      float x, y, z;

      //Nodes are ordered as follows:
      //
      // l b f
      // l b b
      // l u f
      // l u b
      // r b f
      // r b b
      // r u f
      // r u b
      //
      // So that address 7 = 0b111 is on the
      // upper-, right-, back- octant.
      Node* children[8];
    } Internal;

    struct
    {
      // not all leaves within the representation are actually
      // alive, so we must mark those that are set or not
      bool alive;
    } Leaf;
  };

  //QUESTION: Inline?
  unsigned char which_child(const vec3& p) const;
  bool inside_node(const vec3& p) const;

  Node();
  ~Node();
};

struct Octree
{
  Node root;

  void set_aabb(const vec3& min, const vec3& max);

  // assumes MIN and MAX are consistently defined
  void add_point(const vec3& p);
  bool is_inside(const vec3& p) const;
  float closest_leaf(const vec3& o, const vec3& d) const;
  float closest_leaf(const vec3& o, const vec3& d, vec3& normal) const;
};

#endif
