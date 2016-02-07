#include <map>

#include "ao/kernel/render/dc.hpp"
#include "ao/kernel/render/region.hpp"
#include "ao/kernel/render/octree.hpp"

////////////////////////////////////////////////////////////////////////////////

/*
 *  Helper struct that can be passed around when meshing
 */
namespace DC
{

struct Worker
{
    /*
     *  Mutually recursive functions to get a mesh from an Octree
     */
    void cell(const Octree* c);
    void face(const Octree* a, const Octree* b, Octree::Axis axis);
    void edge(const Octree* a, const Octree* b,
              const Octree* c, const Octree* d, Octree::Axis axis);

    /*
     *  Write out the given quad into the mesh
     */
    void quad(const Octree* a, const Octree* b,
              const Octree* c, const Octree* d);

    /*
     *  Return new axes such that a, q, r is right-handed
     */
    static Octree::Axis Q(Octree::Axis a);
    static Octree::Axis R(Octree::Axis a);

    std::map<const Octree*, unsigned> verts;
    Mesh mesh;
};

////////////////////////////////////////////////////////////////////////////////

Octree::Axis Worker::Q(Octree::Axis a)
{
    return (a == Octree::AXIS_X) ? Octree::AXIS_Y :
           (a == Octree::AXIS_Y) ? Octree::AXIS_Z
                                 : Octree::AXIS_X;
}

Octree::Axis Worker::R(Octree::Axis a)
{
    return (a == Octree::AXIS_X) ? Octree::AXIS_Z :
           (a == Octree::AXIS_Y) ? Octree::AXIS_X
                                 : Octree::AXIS_Y;
}

void Worker::cell(const Octree* c)
{
    if (c->getType() == Octree::BRANCH)
    {
        // Recurse, calling the cell procedure for every child
        for (uint8_t i=0; i < 8; ++i)
        {
            cell(c->child(i));
        }

        // Then call the face procedure on every pair of cells
        face(c->child(0), c->child(Octree::AXIS_X), Octree::AXIS_X);
        face(c->child(Octree::AXIS_Y),
             c->child(Octree::AXIS_Y | Octree::AXIS_X),
             Octree::AXIS_X);
        face(c->child(Octree::AXIS_Z),
             c->child(Octree::AXIS_Z | Octree::AXIS_X),
             Octree::AXIS_X);
        face(c->child(Octree::AXIS_Y | Octree::AXIS_Z),
             c->child(Octree::AXIS_Y | Octree::AXIS_Z | Octree::AXIS_X),
             Octree::AXIS_X);

        face(c->child(0), c->child(Octree::AXIS_Y), Octree::AXIS_Y);
        face(c->child(Octree::AXIS_X),
             c->child(Octree::AXIS_X | Octree::AXIS_Y),
             Octree::AXIS_Y);
        face(c->child(Octree::AXIS_Z),
             c->child(Octree::AXIS_Z | Octree::AXIS_Y),
             Octree::AXIS_Y);
        face(c->child(Octree::AXIS_X | Octree::AXIS_Z),
             c->child(Octree::AXIS_X | Octree::AXIS_Z | Octree::AXIS_Y),
             Octree::AXIS_Y);

        face(c->child(0), c->child(Octree::AXIS_Z), Octree::AXIS_Z);
        face(c->child(Octree::AXIS_X),
             c->child(Octree::AXIS_X | Octree::AXIS_Z),
             Octree::AXIS_Z);
        face(c->child(Octree::AXIS_Y),
             c->child(Octree::AXIS_Y | Octree::AXIS_Z),
             Octree::AXIS_Z);
        face(c->child(Octree::AXIS_X | Octree::AXIS_Y),
             c->child(Octree::AXIS_X | Octree::AXIS_Y | Octree::AXIS_Z),
             Octree::AXIS_Z);

        // Finally, call the edge function 6 times
        edge(c->child(0),
             c->child(Octree::AXIS_X),
             c->child(Octree::AXIS_Y),
             c->child(Octree::AXIS_X | Octree::AXIS_Y),
             Octree::AXIS_Z);
        edge(c->child(Octree::AXIS_Z),
             c->child(Octree::AXIS_X | Octree::AXIS_Z),
             c->child(Octree::AXIS_Y | Octree::AXIS_Z),
             c->child(Octree::AXIS_X | Octree::AXIS_Y | Octree::AXIS_Z),
             Octree::AXIS_Z);

        edge(c->child(0),
             c->child(Octree::AXIS_Y),
             c->child(Octree::AXIS_Z),
             c->child(Octree::AXIS_Y | Octree::AXIS_Z),
             Octree::AXIS_X);
        edge(c->child(Octree::AXIS_X),
             c->child(Octree::AXIS_Y | Octree::AXIS_X),
             c->child(Octree::AXIS_Z | Octree::AXIS_X),
             c->child(Octree::AXIS_Y | Octree::AXIS_Z | Octree::AXIS_X),
             Octree::AXIS_X);

        edge(c->child(0),
             c->child(Octree::AXIS_Z),
             c->child(Octree::AXIS_X),
             c->child(Octree::AXIS_Z | Octree::AXIS_X),
             Octree::AXIS_Y);
        edge(c->child(Octree::AXIS_Y),
             c->child(Octree::AXIS_Z | Octree::AXIS_Y),
             c->child(Octree::AXIS_X | Octree::AXIS_Y),
             c->child(Octree::AXIS_Z | Octree::AXIS_X | Octree::AXIS_Y),
             Octree::AXIS_Y);
    }
}

void Worker::face(const Octree* a, const Octree* b, Octree::Axis axis)
{
    if (a->getType() == Octree::BRANCH || b->getType() == Octree::BRANCH)
    {
        Octree::Axis q = Q(axis);
        Octree::Axis r = R(axis);

        face(a->child(axis), b->child(0), axis);
        face(a->child(q|axis), b->child(q), axis);
        face(a->child(r|axis), b->child(r), axis);
        face(a->child(q|r|axis), b->child(q|r), axis);

        edge(a->child(axis), a->child(r|axis), b->child(0), b->child(r), q);
        edge(a->child(q|axis), a->child(q|r|axis), b->child(q), b->child(q|r), q);

        edge(a->child(axis), b->child(0), a->child(axis|q), b->child(q), r);
        edge(a->child(r|axis), b->child(r), a->child(r|axis|q), b->child(r|q), r);
    }
}

void Worker::edge(const Octree* a, const Octree* b,
                  const Octree* c, const Octree* d,
                  Octree::Axis axis)
{
    Octree::Axis q = Q(axis);
    Octree::Axis r = R(axis);

    if (a->getType() == Octree::LEAF && b->getType() == Octree::LEAF &&
        c->getType() == Octree::LEAF && d->getType() == Octree::LEAF &&
        (a->corner(q|r) != a->corner(q|r|axis) ||
         b->corner(r) != b->corner(r|axis) ||
         c->corner(q) != c->corner(q|axis) ||
         d->corner(0) != d->corner(axis)))
    {
        if (d->corner(0))
        {
            quad(a, b, c, d);
        }
        else
        {
            quad(a, c, b, d);
        }
    }
    else if (a->getType() == Octree::BRANCH || b->getType() == Octree::BRANCH ||
             c->getType() == Octree::BRANCH || d->getType() == Octree::BRANCH)
    {
        Octree::Axis q = Q(axis);
        Octree::Axis r = R(axis);

        edge(a->child(q|r), b->child(r), c->child(q), d->child(0), axis);
        edge(a->child(q|r|axis), b->child(r|axis),
             c->child(q|axis), d->child(axis), axis);
    }
}

void Worker::quad(const Octree* a, const Octree* b,
                  const Octree* c, const Octree* d)
{
    auto index = [&](const Octree* o)
    {
        auto i = verts.find(o);
        if (i == verts.end())
        {
            verts[o] = mesh.verts.size();
            mesh.verts.push_back(o->getVertex());
        }
        return verts[o];
    };

    unsigned ia = index(a);
    unsigned ib = index(b);
    unsigned ic = index(c);
    unsigned id = index(d);

    mesh.tris.push_back({ia, ib, ic});
    mesh.tris.push_back({ic, ib, id});
}

////////////////////////////////////////////////////////////////////////////////

Mesh Render(Tree* t, const Region& r)
{
    auto o = Octree::Render(t, r);

    Worker w;
    w.cell(o);

    return w.mesh;
}

} // DC namespace
