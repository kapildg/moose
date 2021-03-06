/****************************************************************/
/* MOOSE - Multiphysics Object Oriented Simulation Environment  */
/*                                                              */
/*          All contents are licensed under LGPL V2.1           */
/*             See LICENSE for full restrictions                */
/****************************************************************/

#include "GeometricCut3DUserObject.h"

// MOOSE includes
#include "MooseError.h"

#include "libmesh/string_to_enum.h"

// XFEM includes
#include "XFEMFuncs.h"

template <>
InputParameters
validParams<GeometricCut3DUserObject>()
{
  // Get input parameters from parent class
  InputParameters params = validParams<GeometricCutUserObject>();

  // Class description
  params.addClassDescription("Base class for 3D XFEM Geometric Cut UserObjects");
  // Return the parameters
  return params;
}

GeometricCut3DUserObject::GeometricCut3DUserObject(const InputParameters & parameters)
  : GeometricCutUserObject(parameters), _center(), _normal()
{
  _cut_time_ranges.push_back(std::make_pair(0.0, 0.0));
}

bool GeometricCut3DUserObject::active(Real /*time*/) const { return true; }

bool
GeometricCut3DUserObject::cutElementByGeometry(const Elem * /*elem*/,
                                               std::vector<CutEdge> & /*cut_edges*/,
                                               std::vector<CutNode> & /*cut_nodes*/,
                                               Real /*time*/) const
{
  mooseError("Invalid method: must use vector of element faces for 3D mesh cutting");
  return false;
}

bool
GeometricCut3DUserObject::cutElementByGeometry(const Elem * elem,
                                               std::vector<CutFace> & cut_faces,
                                               Real /*time*/) const
// TODO: Time evolving cuts not yet supported in 3D (hence the lack of use of the time variable)
{
  bool cut_elem = false;

  for (unsigned int i = 0; i < elem->n_sides(); ++i)
  {
    // This returns the lowest-order type of side.
    std::unique_ptr<Elem> curr_side = elem->side(i);
    if (curr_side->dim() != 2)
      mooseError("In cutElementByGeometry dimension of side must be 2, but it is ",
                 curr_side->dim());
    unsigned int n_edges = curr_side->n_sides();

    std::vector<unsigned int> cut_edges;
    std::vector<Real> cut_pos;

    for (unsigned int j = 0; j < n_edges; j++)
    {
      // This returns the lowest-order type of side.
      std::unique_ptr<Elem> curr_edge = curr_side->side(j);
      if (curr_edge->type() != EDGE2)
        mooseError("In cutElementByGeometry face edge must be EDGE2, but type is: ",
                   libMesh::Utility::enum_to_string(curr_edge->type()),
                   " base element type is: ",
                   libMesh::Utility::enum_to_string(elem->type()));
      Node * node1 = curr_edge->get_node(0);
      Node * node2 = curr_edge->get_node(1);

      Point intersection;
      if (intersectWithEdge(*node1, *node2, intersection))
      {
        cut_edges.push_back(j);
        cut_pos.push_back(getRelativePosition(*node1, *node2, intersection));
      }
    }

    if (cut_edges.size() == 2)
    {
      cut_elem = true;
      CutFace mycut;
      mycut.face_id = i;
      mycut.face_edge.push_back(cut_edges[0]);
      mycut.face_edge.push_back(cut_edges[1]);
      mycut.position.push_back(cut_pos[0]);
      mycut.position.push_back(cut_pos[1]);
      cut_faces.push_back(mycut);
    }
  }

  return cut_elem;
}

bool
GeometricCut3DUserObject::cutFragmentByGeometry(std::vector<std::vector<Point>> & /*frag_edges*/,
                                                std::vector<CutEdge> & /*cut_edges*/,
                                                Real /*time*/) const
{
  mooseError("Invalid method: must use vector of element faces for 3D mesh cutting");
  return false;
}

bool
GeometricCut3DUserObject::cutFragmentByGeometry(std::vector<std::vector<Point>> & /*frag_faces*/,
                                                std::vector<CutFace> & /*cut_faces*/,
                                                Real /*time*/) const
{
  // TODO: Need this for branching in 3D
  mooseError("cutFragmentByGeometry not yet implemented for 3D mesh cutting");
  return false;
}

bool
GeometricCut3DUserObject::intersectWithEdge(const Point & p1, const Point & p2, Point & pint) const
{
  bool has_intersection = false;
  double plane_point[3] = {_center(0), _center(1), _center(2)};
  double plane_normal[3] = {_normal(0), _normal(1), _normal(2)};
  double edge_point1[3] = {p1(0), p1(1), p1(2)};
  double edge_point2[3] = {p2(0), p2(1), p2(2)};
  double cut_point[3] = {0.0, 0.0, 0.0};

  if (Xfem::plane_normal_line_exp_int_3d(
          plane_point, plane_normal, edge_point1, edge_point2, cut_point) == 1)
  {
    Point temp_p(cut_point[0], cut_point[1], cut_point[2]);
    if (isInsideCutPlane(temp_p) && isInsideEdge(p1, p2, temp_p))
    {
      pint = temp_p;
      has_intersection = true;
    }
  }
  return has_intersection;
}

bool
GeometricCut3DUserObject::isInsideEdge(const Point & p1, const Point & p2, const Point & p) const
{
  Real dotp1 = (p1 - p) * (p2 - p1);
  Real dotp2 = (p2 - p) * (p2 - p1);
  return (dotp1 * dotp2 <= 0.0);
}

Real
GeometricCut3DUserObject::getRelativePosition(const Point & p1,
                                              const Point & p2,
                                              const Point & p) const
{
  // get the relative position of p from p1
  Real full_len = (p2 - p1).norm();
  Real len_p1_p = (p - p1).norm();
  return len_p1_p / full_len;
}
