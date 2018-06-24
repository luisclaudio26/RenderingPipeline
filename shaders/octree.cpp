#include "octree.h"
#include <cstdio>

Node::Node()
{
  // we just need to guarantee that the pointers
  // are all null
  for(int i = 0; i < 8; ++i)
    Internal.children[i] = nullptr;
}

Node::~Node()
{
  //TODO: we should traverse the tree deleting stuff
}

void Octree::add_point(const vec3& p)
{
  Node *n = &root;

  // the bounding box is built to be cubic, so we
  // know all sides are equal. we use this to keep
  // track of the splitting point of each box/octant.
  float l = max(0) - min(0);

  // go down until the last but one level,
  // which are internal nodes only
  for(int i = 0; i < MAX_DEPTH-1; ++i)
  {
    // decide in which octant this point falls
    unsigned char address = 0x0;
    if( p(0) >= n->Internal.x ) address |= 0b100;
    if( p(1) >= n->Internal.y ) address |= 0b010;
    if( p(2) >= n->Internal.z ) address |= 0b001;
    Node **next = &n->Internal.children[address];

    // if this node hasn't been created yet, do it
    if( !(*next) )
    {
      *next = new Node;

      // offset to the center of the next octant.
      // all the displacements are either SHIFT of
      // -SHIFT from the center, depending on the
      // address of this octant.
      // The way we defined the address, when we
      // have a 1 bit in a given position (positive
      // in a given axis), we ADD the shift; otherwise
      // we subtract.
      float shift = l/4;
      (*next)->Internal.x = n->Internal.x + (address && 0b100 ? shift : -shift);
      (*next)->Internal.y = n->Internal.y + (address && 0b010 ? shift : -shift);
      (*next)->Internal.z = n->Internal.z + (address && 0b001 ? shift : -shift);
    }

    // update offsets so they will be correct in the
    // next iteration if we need to build a node
    l = l/2;

    // descend tree
    n = *next;
  }

  // the last level is a leaf
  n->Leaf.x = p(0);
  n->Leaf.y = p(1);
  n->Leaf.z = p(2);
}
