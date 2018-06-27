#include "octree.h"
#include <cstdio>
#include <algorithm>
#include <stack>

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

// -------------------------
// --------- Node ----------
// -------------------------
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

unsigned char Node::which_child(const vec3& p) const
{
  unsigned char address = 0x0;
  if( p(0) >= this->Internal.x ) address |= 0b100;
  if( p(1) >= this->Internal.y ) address |= 0b010;
  if( p(2) >= this->Internal.z ) address |= 0b001;
  return address;
}

// ---------------------------
// --------- Octree ----------
// ---------------------------
void Octree::set_aabb(const vec3& min, const vec3& max)
{
  root.Internal.min_x = min(0);
  root.Internal.min_y = min(1);
  root.Internal.min_z = min(2);
  root.Internal.max_x = max(0);
  root.Internal.max_y = max(1);
  root.Internal.max_z = max(2);

  vec3 center = (min + max) * 0.5f;
  root.Internal.x = center(0);
  root.Internal.y = center(1);
  root.Internal.z = center(2);
}

float Octree::closest_leaf(const vec3& o, const vec3& d) const
{
  struct TraversalElem
  {
    const Node* n;
    float tmin, tmax;
    int depth;

    TraversalElem(const Node* n, float tmin, float tmax, int depth)
      : n(n), tmin(tmin), tmax(tmax), depth(depth) { }
  };

  // --------------------
  vec3 bb_min = vec3(root.Internal.min_x,
                      root.Internal.min_y,
                      root.Internal.min_z);
  vec3 bb_max = vec3(root.Internal.max_x,
                      root.Internal.max_y,
                      root.Internal.max_z);

  // bail out if no intersection with the outter bounding box
  float outter_tmin, outter_tmax;
  if( !intersect_box(o, d, bb_min, bb_max, outter_tmin, outter_tmax) ) return NAN;

  std::stack<TraversalElem> stack;
  stack.push( TraversalElem(&root, outter_tmin, outter_tmax, 1) );

  while( !stack.empty() )
  {
    TraversalElem e = stack.top(); stack.pop();
    float tmin = e.tmin, tmax = e.tmax;
    const Node* node = e.n;
    int depth = e.depth;

    // bail out if node is NAN. there's no leaf down here
    if(!node) continue;

    // this is the first leaf intersected by the ray!
    if(depth == MAX_DEPTH) return tmin;

    // if we intersect the box, compute
    // the (possibly) inner intersections
    // TODO: Take care of INFs!
    const float sx = node->Internal.x, sy = node->Internal.y, sz = node->Internal.z;
    float tx = (sx - o(0)) / d(0);
    float ty = (sy - o(1)) / d(1);
    float tz = (sz - o(2)) / d(2);

    // order intersections in decreasing order because
    // we start from the furthest box to the closest one,
    // which is the order we want to push them to the stack.
    // we don't include tmax here because this
    // is the intersection we'll be starting with.
    float t[] = {tx, ty, tz, tmin};
    std::sort(std::begin(t), std::end(t), [](float a, float b) {
                                              return a > b;
                                            });

    // loop intersections in order, discarding
    // points when they're before tmin or after
    // tmax, computing the mid-points
    // break when tlast = tmin
    float mid[4]; float tlast = tmax;
    int i = 0, mid_ind = 0;
    while( tlast != tmin )
    {
      float cur = t[i];

      // if our current intersection is BEHIND
      // tlast, just skip this and get the next
      if( cur >= tlast )
      {
        i++;
        continue;
      }

      // intersection is correct; compute mid point
      float mid_point = (tlast + cur) * 0.5f;
      //mid[mid_ind] = (tlast + cur) * 0.5f;
      //mid_ind++;

      // compute in which octant the mid point falls
      // and push to the stack
      int oct = node->which_child(o + d*mid_point);
      const Node* next = node->Internal.children[oct];

      TraversalElem next_e(next, cur, tlast, depth+1);
      /*
      next_e.depth = depth + 1;
      next_e.node = next;
      next_e.tmin = cur;
      next_e.tmax = tlast; */
      stack.push(next_e);

      // advance tlast to the next intersection
      // so we can get the next octant being intersected
      tlast = cur;
      i++;
    }

    // for each mid-point, compute the octant if falls into

    // descend to the closest octant
    /*
    int oct = node->which_child(o + d*mid[0]);
    node = node->Internal.children[oct];
    depth = depth + 1;
    */
  }

  // if we reached this point, no actual leaf was found
  return NAN;
}

bool Octree::is_inside(const vec3& p) const
{
  //TODO: there are lots of repeated code here and in
  //add_point(). FUsion both!
  const Node *n = &root;
  float l = root.Internal.max_x - root.Internal.min_x;

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
    // and try to descend
    unsigned char oct = n->which_child(p);
    n = n->Internal.children[oct];
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
  float l = root.Internal.max_x - root.Internal.min_x;

  // go down until the last but one level,
  // which are internal nodes only
  for(int i = 0; i < MAX_DEPTH-1; ++i)
  {
    // decide in which octant this point falls
    unsigned char address = n->which_child(p);
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
      /*
      float shift = l/4;
      (*next)->Internal.x = n->Internal.x + (address & 0b100 ? shift : -shift);
      (*next)->Internal.y = n->Internal.y + (address & 0b010 ? shift : -shift);
      (*next)->Internal.z = n->Internal.z + (address & 0b001 ? shift : -shift);
      */

      // compute bounding box for the new node.
      (*next)->Internal.min_x = address & 0b100 ? n->Internal.x : n->Internal.min_x;
      (*next)->Internal.min_y = address & 0b010 ? n->Internal.y : n->Internal.min_y;
      (*next)->Internal.min_z = address & 0b001 ? n->Internal.z : n->Internal.min_z;

      (*next)->Internal.max_x = ~address & 0b100 ? n->Internal.x : n->Internal.max_x;
      (*next)->Internal.max_y = ~address & 0b010 ? n->Internal.y : n->Internal.max_y;
      (*next)->Internal.max_z = ~address & 0b001 ? n->Internal.z : n->Internal.max_z;

      // compute splitting point (midpoint)
      (*next)->Internal.x = ( (*next)->Internal.min_x + (*next)->Internal.max_x ) * 0.5f;
      (*next)->Internal.y = ( (*next)->Internal.min_y + (*next)->Internal.max_y ) * 0.5f;
      (*next)->Internal.z = ( (*next)->Internal.min_z + (*next)->Internal.max_z ) * 0.5f;
    }

    // update offsets so they will be correct in the
    // next iteration if we need to build a node
    l = l/2;

    // descend tree
    n = *next;
  }

  // the last level is a leaf
  /*
  n->Leaf.x = p(0);
  n->Leaf.y = p(1);
  n->Leaf.z = p(2);
  */
}
