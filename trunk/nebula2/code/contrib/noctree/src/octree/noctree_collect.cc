#define N_IMPLEMENTS nOctree
//-------------------------------------------------------------------
//  noctree_collect.cc
//  (C) 1999 A.Weissflog
//-------------------------------------------------------------------
#include "kernel/nkernelserver.h"
#include "kernel/nenv.h"
#include "gfx2/ncamera2.h"
#include "octree/noctree.h"
#include "gfx2/ngfxserver2.h"

//-------------------------------------------------------------------
/**
    Determines the view volume clip status for the bounding box:

    Return:
      - <0  -> entirely out
      - ==0 -> partly in/out
      - >0  -> entirely in
  
    this->vp_matrix must be up-to-date...
  
    - 02-Jun-99   floh    created
*/
int nOctree::box_clip_viewvol(vector3& p0, vector3& p1)
{
    int i, and_flags, or_flags;
    vector4 v0,v1;

    // for each of the 8 bounding box corners...
    and_flags = 0xffff;
    or_flags  = 0;
    for (i=0; i<8; i++) {
        int clip = 0;
        v0.w = 1.0;
        if (i & 1) v0.x = p0.x;
        else       v0.x = p1.x;
        if (i & 2) v0.y = p0.y;
        else       v0.y = p1.y;
        if (i & 4) v0.z = p0.z;
        else       v0.z = p1.z;

        // transform current corner to clip coordinate
        v1 = this->vp_matrix * v0;

        // Get clip flags...
        if      (v1.x < -v1.w)  clip |= N_OCT_CLIPX0;
        else if (v1.x > v1.w)   clip |= N_OCT_CLIPX1;
        if      (v1.y < -v1.w)  clip |= N_OCT_CLIPY0;
        else if (v1.y > v1.w)   clip |= N_OCT_CLIPY1;
        if      (v1.z < -v1.w)  clip |= N_OCT_CLIPZ0;
        else if (v1.z > v1.w)   clip |= N_OCT_CLIPZ1;
        and_flags &= clip;
        or_flags  |= clip;
    }
    if (0 == or_flags)          return +1;  // ENTIRELY IN
    else if (0 != and_flags)    return -1;  // ENTIRELY OUT
    else                        return 0;   // PARTLY IN/OUT
}

//-------------------------------------------------------------------
/**
    Get clipping status of node bounding box against global
    collection bounding box.

    Return:
      - +1 - node box completely containd in collection box
      - -1 - node box completely outside of collection box
      -  0 - node box partially inside collection box, or collection
             box is contained in node box

    - 02-Jun-00   floh    created
*/
int nOctree::box_clip_box(vector3& p0, vector3& p1)
{
    bbox3 b(p0,p1);
    int res = b.intersect(this->collect_bbox);
    if (bbox3::OUTSIDE == res) {
        return -1;
    } else if (bbox3::ISCONTAINED == res) {
        return +1;
    } else {
        return 0;
    }
}

//-------------------------------------------------------------------
/**
    Collects an element and ensures that collect_array doesn't
    overflow.

    - 02-Jun-99   floh    created
*/
void nOctree::collect(nOctElement *oe)
{
    if (this->num_collected < this->ext_array_size) {
        this->ext_collect_array[this->num_collected++] = oe;
    } else {
        n_printf("nOctree::collect(): Overflow in collect array!\n");
    }
}

//-------------------------------------------------------------------
/**
    Trivially collects all elements of the specified node and
    recursively searches its child nodes.

    - 02-Jun-99   floh    created
*/
void nOctree::recurse_collect_nodes_with_flags(nOctNode *on, int c_flags)
{
    nOctElement *oe;
    for (oe = (nOctElement *) on->elm_list.GetHead();
         oe;
         oe = (nOctElement *) oe->GetSucc())
    {
        oe->SetCollectFlags(c_flags);
        this->collect(oe);
    }
    if (on->c[0]) {
        int i;
        for (i=0; i<8; i++) this->recurse_collect_nodes_with_flags(on->c[i],c_flags);
    }
}

//-------------------------------------------------------------------
/**
    - 30-May-00   floh    created
*/
void nOctree::collect_nodes_in_viewvol_or_bbox(nOctNode *on)
{
    nOctElement *oe;
    for (oe = (nOctElement *) on->elm_list.GetHead();
         oe;
         oe = (nOctElement *) oe->GetSucc())
    {
        int vvol_c = this->box_clip_viewvol(oe->p0,oe->p1);
        int bbox_c = this->box_clip_box(oe->p0,oe->p1);
        int c_flags = 0;
        if (vvol_c >= 0) c_flags |= nOctElement::N_COLLECT_VIEWVOL;
        if (bbox_c >= 0) c_flags |= nOctElement::N_COLLECT_BBOX;
        if (c_flags != 0) {
            oe->SetCollectFlags(c_flags);
            this->collect(oe);
        }
    }
}

//-------------------------------------------------------------------
/**
    - 30-May-00   floh    created
*/
void nOctree::recurse_collect_by_viewvol_or_bbox(nOctNode *on)
{
    // clip status of current node
    int vvol_c = this->box_clip_viewvol(on->p0,on->p1);
    int bbox_c = this->box_clip_box(on->p0,on->p1);
    if ((vvol_c > 0) && (bbox_c > 0)) {
        // node is contained both in view volume and in bounding
        // box, trivially accept all sub-nodes
        this->recurse_collect_nodes_with_flags(on,(nOctElement::N_COLLECT_VIEWVOL|nOctElement::N_COLLECT_BBOX));
    } else if ((vvol_c > 0) && (bbox_c < 0)) {
        // node is contained in view volume and completely outside
        // bounding box, trivially accept all view volume sub nodes
        this->recurse_collect_nodes_with_flags(on,nOctElement::N_COLLECT_VIEWVOL);
    } else if ((vvol_c < 0) && (bbox_c > 0)) {
        // node is contained in bounding box and completely outside
        // view volume, trivially accept all bounding box sub nodes
        this->recurse_collect_nodes_with_flags(on,nOctElement::N_COLLECT_BBOX);
    } else if ((vvol_c < 0) && (bbox_c < 0)) {
        // node is outside of both volumes
    } else {
        // look at each element and recurse down further...
        this->collect_nodes_in_viewvol_or_bbox(on);
        if (on->c[0]) {
            int i;
            for (i=0; i<8; i++) this->recurse_collect_by_viewvol_or_bbox(on->c[i]);
        }
    }
}

//-------------------------------------------------------------------
/**
    - 17-Aug-00   floh    created
*/
void nOctree::collect_nodes_in_bbox(nOctNode *on)
{
    nOctElement *oe;
    for (oe = (nOctElement *) on->elm_list.GetHead();
         oe;
         oe = (nOctElement *) oe->GetSucc())
    {
        int c = this->box_clip_box(oe->p0,oe->p1);
        if (c >= 0) {
            oe->SetCollectFlags(nOctElement::N_COLLECT_BBOX);
            this->collect(oe);
        }
    }
}

//-------------------------------------------------------------------
/**
    - 17-Aug-00   floh    created
*/
void nOctree::recurse_collect_by_bbox(nOctNode *on)
{
    // clip status of current node
    int bbox_c = this->box_clip_box(on->p0,on->p1);
    if (bbox_c > 0) {
        // node is contained completely inside the
        // collection box, trivially accept all child nodes
        this->recurse_collect_nodes_with_flags(on,nOctElement::N_COLLECT_BBOX);
    } else if (bbox_c == 0) {
        // node is partly contained in box, all elements in
        // the node need to be checked further, and recurse
        // to children
        this->collect_nodes_in_bbox(on);
        if (on->c[0]) {
            int i;
            for (i=0; i<8; i++) this->recurse_collect_by_bbox(on->c[i]);
        }
    }
}

//-------------------------------------------------------------------
/**
    @brief Collect all nodes that are either in the view volume 
    and/or a bounding bbox.

    - 30-May-00   floh    created
*/
int nOctree::CollectByViewvolOrBBox(matrix44& mv, 
                                    matrix44& p, 
                                    bbox3& bbox,
                                    nOctElement **ext_array,
                                    int size)
{
    n_assert(ext_array);
    n_assert(NULL == ext_collect_array);

    this->vp_matrix         = mv * p;
    this->collect_bbox      = bbox;
    this->num_collected     = 0;
    this->ext_collect_array = ext_array;
    this->ext_array_size    = size;
    
    this->recurse_collect_by_viewvol_or_bbox(this->tree_root);
    
    this->ext_collect_array = 0;
    this->ext_array_size = 0;
    return this->num_collected;
}

//-------------------------------------------------------------------
/**
    @brief Collect all nodes touching given bounding box.

    - 17-Aug-00   floh    created
*/
int nOctree::CollectByBBox(bbox3& bbox, 
                           nOctElement **ext_array,
                           int size)
{
    n_assert(ext_array);
    n_assert(NULL == ext_collect_array);

    this->collect_bbox      = bbox;
    this->num_collected     = 0;
    this->ext_collect_array = ext_array;
    this->ext_array_size    = size;

    this->recurse_collect_by_bbox(this->tree_root);

    this->ext_collect_array = 0;
    this->ext_array_size = 0;
    return this->num_collected;
}

//-------------------------------------------------------------------
/**
    @brief Collect all nodes bounded by view frustum
    Assume symmetrical frustum.

    Uses a clip mask to minimize checks.  Hardcoded to only derive clip
    planes from frustum but no reason why it cannot be extended to use
    additional clip planes, up to a total max of 32.
*/
int nOctree::CollectByFrustum(nGfxServer2* gfx_server,
                              nOctElement** ext_array,
                              int size)
{
    n_assert(ext_array);
    n_assert(NULL == ext_collect_array);

    init_clip_planes_for_frustum(gfx_server);

    this->num_collected     = 0;
    this->ext_collect_array = ext_array;
    this->ext_array_size    = size;

    unsigned int active_clips = 0x003F;
    recurse_collect_within_clip_planes(this->tree_root, active_clips);

    this->ext_collect_array = 0;
    this->ext_array_size = 0;
    return num_collected;
}

//-------------------------------------------------------------------
/**
    Initialize clip planes for view frustum with view-projection matrix.

    this code from at:
    http://www2.ravensoft.com/users/ggribb/plane%20extraction.pdf
*/
void nOctree::init_clip_planes_for_frustum(nGfxServer2* gfx_server)
{
    const matrix44& inv_viewer = gfx_server->GetTransform(nGfxServer2::TransformType::InvView);
    const matrix44& projection = gfx_server->GetTransform(nGfxServer2::TransformType::Projection);

    matrix44 m = inv_viewer * projection;

    // front
    clipPlanes[0].x = m.M13;
    clipPlanes[0].y = m.M23;
    clipPlanes[0].z = m.M33;
    clipPlanes[0].w = m.M43;

    // back
    clipPlanes[1].x = (m.M14 - m.M13);
    clipPlanes[1].y = (m.M24 - m.M23);
    clipPlanes[1].z = (m.M34 - m.M33);
    clipPlanes[1].w = (m.M44 - m.M43);

    // left
    clipPlanes[2].x = (m.M14 + m.M11);
    clipPlanes[2].y = (m.M24 + m.M21);
    clipPlanes[2].z = (m.M34 + m.M31);
    clipPlanes[2].w = (m.M44 + m.M41);

    // right
    clipPlanes[3].x = (m.M14 - m.M11);
    clipPlanes[3].y = (m.M24 - m.M21);
    clipPlanes[3].z = (m.M34 - m.M31);
    clipPlanes[3].w = (m.M44 - m.M41);

    // top
    clipPlanes[4].x = (m.M14 - m.M12);
    clipPlanes[4].y = (m.M24 - m.M22);
    clipPlanes[4].z = (m.M34 - m.M32);
    clipPlanes[4].w = (m.M44 - m.M42);

    // bottom
    clipPlanes[5].x = (m.M14 + m.M12);
    clipPlanes[5].y = (m.M24 + m.M22);
    clipPlanes[5].z = (m.M34 + m.M32);
    clipPlanes[5].w = (m.M44 + m.M42);

    //normaize planes.
    float denom;
    vector3 tmp;
    
    for (int i=0; i<6; i++)
    {
        tmp.set (clipPlanes[i].x, clipPlanes[i].y, clipPlanes[i].z);
        denom = 1.0f / tmp.len();
        clipPlanes[i].x *= denom;
        clipPlanes[i].y *= denom;
        clipPlanes[i].z *= denom;
        clipPlanes[i].w *= denom;
    }
}

//-------------------------------------------------------------------
/**
    @brief Recursively collect within the clip planes.
*/
void nOctree::recurse_collect_within_clip_planes(nOctNode* on,
                                         unsigned int clip_mask)
{
    unsigned int out_clip_mask;

    // Clip-Status der aktuellen Node...
    if (false == box_clip_against_clip_planes(on->p0, on->p1, clipPlanes, out_clip_mask, clip_mask))
        return;

    // Node completely contained, gotta catch them all!
    if (0 == out_clip_mask)
    {
        recurse_collect_nodes_with_flags(on, nOctElement::N_COLLECT_CLIP_PLANES);
    }
    // Partially clipped by clip planes, future doodah could do
    // with passing in clip mask to reduce number of checks recursively
    else
    {
        this->collect_nodes_within_clip_planes(on, out_clip_mask);
        if (on->c[0])
        {
            for (int i = 0; i < 8; ++i)
                this->recurse_collect_within_clip_planes(on->c[i], out_clip_mask);
        }
    }
}

//-------------------------------------------------------------------
/**
    @brief Collect nodes in this node only.
*/
void nOctree::collect_nodes_within_clip_planes(nOctNode* on, unsigned int clip_mask)
{
    nOctElement *oe;
    for (oe = (nOctElement *) on->elm_list.GetHead();
         oe;
         oe = (nOctElement *) oe->GetSucc())
    {
        unsigned int out_clip_mask = 0;
        if (true == box_clip_against_clip_planes(oe->p0,oe->p1, clipPlanes, out_clip_mask, clip_mask))
        {
            oe->SetCollectFlags(nOctElement::N_COLLECT_CLIP_PLANES);
            this->collect(oe);
        }
    }
}

//-------------------------------------------------------------------
/**
    Clip AABB defined by p0, p1 to supplied planes.

    This code is derived from code provided by Ville Miettinen of Hybrid.fi
    on the Algorithms mailing list.  It is similar to the code that
    Hybrid uses in their product dPVS (http://www.hybrid.fi/dpvs.html).

    @param p0 Lower corner of AABB
    @param p1 Upper corner of AABB
    @param planes the array of planes to clip against (up to 32)
    @param out_clip_mask returned clip plane overlap
    @param in_clip_mask planes to clip against
    @return true if the AABB is at least partially inside clip
*/
bool nOctree::box_clip_against_clip_planes(vector3& p0, vector3& p1,
                                           vector4* planes,
                                           unsigned int& out_clip_mask,
                                           unsigned int in_clip_mask)
{
    // Figure out centre and half diagonal of AABB
    vector3 centre = (p0 + p1) / 2.0f;
    vector3 half_diagonal = p1 - centre;

    unsigned int mk = 1;
    out_clip_mask = 0;

    // loop while there are active planes..
    while (mk <= in_clip_mask)
    {
        if (in_clip_mask & mk)
        {
            float NP = float(half_diagonal.x * n_abs(planes->x) +
                             half_diagonal.y * n_abs(planes->y) +
                             half_diagonal.z * n_abs(planes->z));
            float MP = centre.x * planes->x +
                       centre.y * planes->y +
                       centre.z * planes->z +
                       planes->w;

            // Behind clip plane
            if ((MP + NP) < 0.0f)
                return false;

            // Cross this plane
            if ((MP - NP) < 0.0f)
                out_clip_mask |= mk;
        }

        mk <<= 1;
        ++planes;
    }

    // Contained in some way
    return true;
}

//-------------------------------------------------------------------
/**
    @brief Collect all nodes clipped by sphere.
    @param clip the clipping sphere
    @param ext_array provided collection array
    @param size the maximum number of collected nodes
    @return the number of nodes collected
*/
int nOctree::CollectBySphere(const sphere& clip,
                             nOctElement** ext_array,
                             int size)
{
    n_assert(ext_array);
    n_assert(NULL == ext_collect_array);

    this->num_collected     = 0;
    this->ext_collect_array = ext_array;
    this->ext_array_size    = size;

    this->recurse_collect_by_sphere(this->tree_root, clip, false);

    this->ext_collect_array = 0;
    this->ext_array_size = 0;
    return this->num_collected;
}

//-------------------------------------------------------------------
/**
    @brief Recursively collect within the clip planes.
*/
void nOctree::recurse_collect_by_sphere(nOctNode* on,
                                        const sphere& clip,
                                        bool full_intersect)
{
    bbox3 box(on->p0, on->p1);
    if (false == clip.intersects(box))
        return;

    // Check for full intersect
    {
        float s, d = 0.0f;

        if (clip.p.x > box.vmin.x) { s = clip.p.x - box.vmin.x; d += s * s; }
        else if (clip.p.x < box.vmax.x) { s = clip.p.x - box.vmax.x; d += s * s; }
        if (clip.p.y > box.vmin.y) { s = clip.p.y - box.vmin.y; d += s * s; }
        else if (clip.p.y < box.vmax.y) { s = clip.p.y - box.vmax.y; d += s * s; }
        if (clip.p.z > box.vmin.z) { s = clip.p.z - box.vmin.z; d += s * s; }
        else if (clip.p.z < box.vmax.z) { s = clip.p.z - box.vmax.z; d += s * s; }

        full_intersect = d <= clip.r * clip.r;
    }

    // Intersects with clipping sphere, go collect
    this->collect_nodes_in_sphere(on, clip, full_intersect);

    // Recurse if possible
    if (on->c[0])
    {
        for (int i = 0; i < 8; ++i)
            this->recurse_collect_by_sphere(on->c[i], clip, full_intersect);
    }
}
 
/**
    @brief Collect nodes in this node only.
*/
void nOctree::collect_nodes_in_sphere(nOctNode* on,
                                      const sphere& clip,
                                      bool full_intersect)
{
    nOctElement *oe;
    for (oe = (nOctElement *)on->elm_list.GetHead();
         oe;
         oe = (nOctElement *)oe->GetSucc())
    {
        if (true == full_intersect)
            this->collect(oe);
        else
        {
            sphere sphere(oe->pos, oe->radius);
            if (true == clip.intersects(sphere))
            {
                // oe->SetCollectFlags(nOctElement::N_COLLECT_SPHERE);
                this->collect(oe);
            }
        }
    }
}
//-------------------------------------------------------------------
//  EOF
//-------------------------------------------------------------------
