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

struct Leaf
{
  // TODO: this probably has some severe alignment problems
  vec3 normal, color;
};

struct Node
{
  Node* children[8];
};

#endif
