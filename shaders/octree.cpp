#include "octree.h"
#include <cstdio>
#include <algorithm>

// -----------------------------
// --------- INTERNAL ----------
// -----------------------------
static bool intersect_box(const vec3& o, const vec3& d,
                          const vec3& min, const vec3& max,
                          float& tmin, float& tmax)
{
  #define EPS 0.00001f

  //use tavianator's slab method: sucessively clip ray in each axis
  tmin = -FLT_MAX; tmax = FLT_MAX;

  for(int i = 0; i < 3; ++i)
  {
    //if r.d(i) == 0, ray is parallel to the current axis
    if(d(i) == 0.0f ) continue;

    float t1, t2, ro = o(i);

    //this should avoid the cases where the ray intersects infinitely
    //many points on one of the planes
    if( min(i) == o(i) || max(i) == o(i)) ro += EPS;

    t1 = (min(i) - ro) / d(i);
    t2 = (max(i) - ro) / d(i);

    tmin = std::fmax(tmin, std::fmin(t1, t2));
    tmax = std::fmin(tmax, std::fmax(t1, t2));
  }

  //tmax = tmin is a hit right in the corner of the box,
  //which we assume to not to be a hit! TODO: is this a problem?
  return tmax >= tmin && tmax > 0.0f;
}

// ----------------------------------
// --------- FROM OCTREE.H ----------
// ----------------------------------
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

void Octree::set_aabb(const vec3& min, const vec3& max)
{
  this->min = min;
  this->max = max;

  vec3 center = (min + max) * 0.5f;
  root.Internal.x = center(0);
  root.Internal.y = center(1);
  root.Internal.z = center(2);
}

bool Octree::closest_leaf(const vec3& o, const vec3& d) const
{
  vec3 bb_min = this->min, bb_max = this->max;
  float tmin, tmax;
  const Node* node = &root;

  // bail out if no intersection
  if( !intersect_box(o, d, bb_min, bb_max, tmin, tmax) )
    return false;

  // if we intersect the box, compute
  // the (possibly) inner intersections
  const float sx = node->Internal.x, sy = node->Internal.y, sz = node->Internal.z;
  float tx = (sx - o(0)) / d(0);
  float ty = (sy - o(1)) / d(1);
  float tz = (sz - o(2)) / d(2);

  // order the inner intersections so we know
  // in which order we should traverse
  float t[] = {tx, ty, tz};
  std::sort(std::begin(t), std::end(t));

  // loop intersections in order, discarding
  // points when they're before tmin or after
  // tmax, computing the mid-points

  // for each mid-point, compute the octant
  // if falls into

  // descend to the closest octant

  // repeat until we reach a leaf (or the node
  // we're trying to descend doesn't exist)
}

bool Octree::is_inside(const vec3& p) const
{
  //TODO: there are lots of repeated code here and in
  //add_point(). FUsion both!
  const Node *n = &root;
  float l = max(0) - min(0);

  // assert that P is inside the outter bounding box.
  // TODO: do the same for add_point()
  float half_l = l*0.5f;
  if( fabs(p(0) - root.Internal.x) > half_l ||
      fabs(p(1) - root.Internal.y) > half_l ||
      fabs(p(2) - root.Internal.z) > half_l ) return false;

  // go down until we reach MAX_DEPTH (a leaf) or
  // a null node
  for(int i = 0; i < MAX_DEPTH; ++i)
  {
    // no node to descent to, thus this point is not
    // on the voxel structure. bail out!
    if( !n ) return false;

    // decide in which octant this point falls
    unsigned char address = 0x0;
    if( p(0) >= n->Internal.x ) address |= 0b100;
    if( p(1) >= n->Internal.y ) address |= 0b010;
    if( p(2) >= n->Internal.z ) address |= 0b001;

    // yes, a pointer to a const pointer...
    //Node* const *next = &n->Internal.children[address];
    n = n->Internal.children[address];
  }

  // if we reached this point, we reached a leaf
  // in the previous loop, thus the point is inside
  // the voxel structure
  return true;
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
