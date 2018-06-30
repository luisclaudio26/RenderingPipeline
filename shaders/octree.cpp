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
Node::Node() : min_x(0.0f), min_y(0.0f), min_z(0.0f),
               max_x(0.0f), max_y(0.0f), max_z(0.0f)
{
  // we just need to guarantee that the pointers
  // are all null
  for(int i = 0; i < 8; ++i)
    Internal.children[i] = nullptr;

  // AND that the node is not alive as of its creation
  // This part won't overlap the children nodes pointers.
  Leaf.alive = false;
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

bool Node::inside_node(const vec3& p) const
{
  bool inside_x = (min_x <= p(0)) && (p(0) <= max_x);
  bool inside_y = (min_y <= p(1)) && (p(1) <= max_y);
  bool inside_z = (min_z <= p(2)) && (p(2) <= max_z);
  return inside_x && inside_y && inside_z;
}

// ---------------------------
// --------- Octree ----------
// ---------------------------
void Octree::set_aabb(const vec3& min, const vec3& max)
{
  root.min_x = min(0);
  root.min_y = min(1);
  root.min_z = min(2);
  root.max_x = max(0);
  root.max_y = max(1);
  root.max_z = max(2);

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
  vec3 bb_min = vec3(root.min_x, root.min_y, root.min_z);
  vec3 bb_max = vec3(root.max_x, root.max_y, root.max_z);

  // bail out if no intersection with the outter bounding box
  float outter_tmin, outter_tmax;
  if( !intersect_box(o, d, bb_min, bb_max, outter_tmin, outter_tmax) )
    return NAN;

  std::stack<TraversalElem> stack;
  stack.push( TraversalElem(&root, outter_tmin, outter_tmax, 1) );

  while( !stack.empty() )
  {
    TraversalElem e = stack.top(); stack.pop();
    float tmin = e.tmin, tmax = e.tmax;
    const Node* node = e.n;
    int depth = e.depth;

    // skip if node is NULL. there's no leaf down here
    if(!node) continue;

    // TODO: return actual intersection point and normal
    // so we can perform some basic shading.
    // This is always the closest leaf (alive or not) intersected
    // by the ray in this point of the traversal!
    // We need to check whether this leaf is alive not to return
    // intersections with empty leaves that might exist. Also, if its
    // alive, we need to check whether the origin of the ray is inside
    // this leaf to avoid self-intersections: if the origin is inside
    // this leaf, just keep searching for further alive leaves.
    // If we found a leaf which is alive AND our intersection point
    // is outside it, then we succedeed in finding the closest leaf!
    if(depth == MAX_DEPTH)
    {
      if( !node->Leaf.alive || node->inside_node(o)) continue;
      else return tmin;
    }

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
      // if our last intersection point was at t = 0,
      // just stop looking the next intersections (the
      // t's are ordered, thus everyone from now on
      // will be negative).
      if(tlast == 0.0f) break;

      //TODO: segfaulting here because i is too big
      // when the origin of the ray is inside the octree
      // itself, we'll have negative values of t because
      // intersections may occur BEHIND the origin.
      // In these cases we just clamp the value to zero, i.e.
      // we traverse only the octants in front of the origin.
      float cur = t[i];
      if( cur < 0.0f ) cur = 0.0f;

      // if our current intersection is BEHIND
      // tlast, just skip this and get the next
      if( cur >= tlast )
      {
        //TODO: probably because if there's any problem with
        //all t's being infinity or nan, this will loop forever.
        // We need to limit the amount of loops here so that i
        // is never greater than 3! but I'm not sure right now
        // what's the best way to do this.
        i++;
        continue;
      }

      // intersection is correct; compute mid point
      float mid_point = (tlast + cur) * 0.5f;

      // compute in which octant the mid point falls
      // and push to the stack
      int oct = node->which_child(o + d*mid_point);
      const Node* next = node->Internal.children[oct];

      TraversalElem next_e(next, cur, tlast, depth+1);
      stack.push(next_e);

      // advance tlast to the next intersection
      // so we can get the next octant being intersected
      tlast = cur;
      i++;
    }
  }

  // if we reached this point, no actual leaf was found
  return NAN;
}

float Octree::closest_leaf(const vec3& o, const vec3& d, vec3& normal) const
{
  struct TraversalElem
  {
    const Node* n;
    float tmin, tmax;
    vec3 normal;
    int depth;

    TraversalElem(const Node* n, float tmin, float tmax, int depth)
      : n(n), tmin(tmin), tmax(tmax), depth(depth) { }

    TraversalElem(const Node* n, float tmin, float tmax, const vec3& normal, int depth)
        : n(n), tmin(tmin), tmax(tmax), depth(depth), normal(normal) { }
  };

  // --------------------
  vec3 bb_min = vec3(root.min_x,
                      root.min_y,
                      root.min_z);
  vec3 bb_max = vec3(root.max_x,
                      root.max_y,
                      root.max_z);

  // bail out if no intersection with the outter bounding box
  // TODO: not computing first intersection gives problems in the cube.obj
  // scene!
  float outter_tmin, outter_tmax;
  if( !intersect_box(o, d, bb_min, bb_max, outter_tmin, outter_tmax) )
    return NAN;

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
    // TODO: return actual intersection point and normal
    // so we can perform some basic shading.
    if(depth == MAX_DEPTH)
    {
      normal = e.normal;
      return node->Leaf.alive ? tmin : NAN;
    }

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
    //TODO: problem with signs! need to decide whether intersects
    //from the front or from behind
    struct TNormal { float t; vec3 n; };
    TNormal t[] = { TNormal{.t=tx, .n=vec3(1.0f, 0.0f, 0.0f)},
                    TNormal{.t=ty, .n=vec3(0.0f, 1.0f, 0.0f)},
                    TNormal{.t=tz, .n=vec3(0.0f, 0.0f, 1.0f)},
                    TNormal{.t=tmin, .n=e.normal} };

    std::sort(std::begin(t), std::end(t), [](const TNormal& a, const TNormal& b)
                                            {
                                              return a.t > b.t;
                                            });

    // loop intersections in order, discarding
    // points when they're before tmin or after
    // tmax, computing the mid-points
    // break when tlast = tmin
    float mid[4]; float tlast = tmax;
    int i = 0, mid_ind = 0;
    while( tlast != tmin )
    {
      if( tlast == 0.0f ) break;

      //TODO: segfaulting here because i is too big
      TNormal cur = t[i];
      if(cur.t < 0.0f) cur.t = 0.0f;

      // if our current intersection is BEHIND
      // tlast, just skip this and get the next
      if( cur.t >= tlast )
      {
        //TODO: probably because if there's any problem with
        // all t's being infinity or nan, this will loop forever.
        // We need to limit the amount of loops here so that i
        // is never greater than 3! but I'm not sure right now
        // what's the best way to do this.
        i++;
        continue;
      }

      // intersection is correct; compute mid point
      float mid_point = (tlast + cur.t) * 0.5f;

      // compute in which octant the mid point falls
      // and push to the stack
      int oct = node->which_child(o + d*mid_point);
      const Node* next = node->Internal.children[oct];

      TraversalElem next_e(next, cur.t, tlast, cur.n, depth+1);
      stack.push(next_e);

      // advance tlast to the next intersection
      // so we can get the next octant being intersected
      tlast = cur.t;
      i++;
    }
  }

  // if we reached this point, no actual leaf was found
  return NAN;
}

bool Octree::is_inside(const vec3& p) const
{
  //TODO: there are lots of repeated code here and in
  //add_point(). FUsion both!
  const Node *n = &root;
  float l = root.max_x - root.min_x;

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

      // compute bounding box for the new node.
      (*next)->min_x = address & 0b100 ? n->Internal.x : n->min_x;
      (*next)->min_y = address & 0b010 ? n->Internal.y : n->min_y;
      (*next)->min_z = address & 0b001 ? n->Internal.z : n->min_z;

      (*next)->max_x = ~address & 0b100 ? n->Internal.x : n->max_x;
      (*next)->max_y = ~address & 0b010 ? n->Internal.y : n->max_y;
      (*next)->max_z = ~address & 0b001 ? n->Internal.z : n->max_z;

      // compute splitting point (midpoint)
      (*next)->Internal.x = ( (*next)->min_x + (*next)->max_x ) * 0.5f;
      (*next)->Internal.y = ( (*next)->min_y + (*next)->max_y ) * 0.5f;
      (*next)->Internal.z = ( (*next)->min_z + (*next)->max_z ) * 0.5f;
    }

    // descend tree
    n = *next;
  }

  // at this point, all nodes (including the leaf itself)
  // we created and n now points to the leaf. Mark it as
  // ALIVE.
  n->Leaf.alive = true;
}
