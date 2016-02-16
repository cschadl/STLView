/*
 * DisplayObject.cpp
 *
 *  Created on: May 27, 2013
 *      Author: cds
 */

#include <vectors.h>
#include <triangle_mesh.h>

#include <exception>
#include <stdexcept>

#include <GL/gl.h>

#include "DisplayObject.h"

using maths::vector3d;
using maths::bbox3d;

using std::shared_ptr;

DisplayObject::DisplayObject()
: m_display_id(glGenLists(1))
, m_transform(4, 4)
, m_suppressed(false)
{
	if (m_display_id == 0)
		throw std::runtime_error("Error creating display list");
}

DisplayObject::~DisplayObject()
{
	if (m_display_id > 0)
		glDeleteLists(m_display_id, 1);
}

void DisplayObject::build_child_display_lists()
{
	std::vector<DOPtr>	child_do_queue;
	child_do_queue.insert(child_do_queue.end(), m_children.begin(), m_children.end());

	while (!child_do_queue.empty())
	{
		DOPtr display_obj = *child_do_queue.begin();
		child_do_queue.erase(child_do_queue.begin());

		display_obj->BuildDisplayLists();

		const std::vector<DOPtr> display_obj_children = display_obj->GetChildren();
		child_do_queue.insert(child_do_queue.end(), display_obj_children.begin(), display_obj_children.end());
	}
}

void DisplayObject::AddChild(const DOPtr& display_object)
{
	m_children.push_back(display_object);
}

void DisplayObject::RemoveChild(const DOPtr& display_object)
{
	auto child_it = std::find(m_children.begin(), m_children.end(), display_object);
	if (child_it != m_children.end())
		m_children.erase(child_it);
}

void DisplayObject::Draw() const
{
	glCallList(display_id());

	std::vector<DOPtr> child_do_queue(m_children);

	while (!child_do_queue.empty())
	{
		DOPtr display_obj = *child_do_queue.begin();
		child_do_queue.erase(child_do_queue.begin());

		if (!display_obj->Suppressed())
		{
			glCallList(display_obj->display_id());

			const std::vector<DOPtr> display_obj_children = display_obj->GetChildren();
			child_do_queue.insert(child_do_queue.end(), display_obj_children.begin(), display_obj_children.end());
		}
	}
}

///////////////////////////
// MeshDisplayObject

MeshDisplayObject::MeshDisplayObject(shared_ptr<triangle_mesh> mesh)
: m_mesh(mesh)
{

}

//static
bool MeshDisplayObject::is_sharp_edge_boundary(const mesh_facet & f1, const mesh_facet & f2)
{
	// TODO - Maybe this should only be for convex facet edges
	// Also TODO - gank mesh edge concavity code from work ;)
	const double tol = 1.0e-4;
	const double cos_norms = f1.get_normal() * f2.get_normal();
	return cos_norms < M_PI_4 - tol;
}

//virtual
void MeshDisplayObject::BuildDisplayLists()
{
	glNewList(display_id(), GL_COMPILE);
	const std::vector<mesh_facet_ptr>& mesh_facets = m_mesh->get_facets();

	const maths::vector3d x(1.0, 0.0, 0.0);
	const maths::vector3d y(0.0, 1.0, 0.0);
	const maths::vector3d z(0.0, 0.0, 1.0);

	//glColor4fv(green);
	glBegin(GL_TRIANGLES);
	{
		for (const mesh_facet_ptr& facet : mesh_facets)
		{
			std::vector<mesh_vertex_ptr> verts = facet->get_verts();

			const vector3d& facet_normal = facet->get_normal();
			// TODO - enable per-facet normal

			const double facet_x = maths::abs(facet_normal * x);
			const double facet_y = maths::abs(facet_normal * y);
			const double facet_z = maths::abs(facet_normal * z);

			glColor3d(facet_x, facet_y, facet_z);

			//assert(verts.size() == 3);
			for (int i = 0 ; i < 3 ; i++)
			{
				mesh_vertex_ptr vert = verts[i];
				const vector3d v_point = vert->get_point();
				const vector3d v_normal = vert->get_normal();	// TODO - option for per-vertex normals

				// If the vertex has any adjacent triangles with facet normals > 45 degrees
				// from eachother, then just use the facet normal rather than the vertex normal.
				std::vector<mesh_facet_ptr> vert_facets = vert->get_adjacent_facets();
				std::vector<mesh_facet_ptr>::iterator sharp_neighbor =
					std::find_if(vert_facets.begin(), vert_facets.end(),
						[&facet](const mesh_facet_ptr & vf)
						{ return MeshDisplayObject::is_sharp_edge_boundary(*facet, *vf); });

				if (sharp_neighbor != vert_facets.end())
					glNormal3d(facet_normal.x(), facet_normal.y(), facet_normal.z());	// found sharp edge
				else
					glNormal3d(v_normal.x(), v_normal.y(), v_normal.z());

				glVertex3d(v_point.x(), v_point.y(), v_point.z());
			}
		}
	}
	glEnd(); // GL_TRIANGLES

	glEndList();

	build_child_display_lists();
}

//virtual
bbox3d MeshDisplayObject::GetBBox() const
{
	return m_mesh->bbox();
}

///////////////////////////
// MeshEdgesDisplayObject

MeshEdgesDisplayObject::MeshEdgesDisplayObject(shared_ptr<triangle_mesh> mesh)
: m_mesh(mesh)
{

}

//virtual
void MeshEdgesDisplayObject::BuildDisplayLists()
{
	glNewList(display_id(), GL_COMPILE);

	glLineWidth(2.5);
	glEnable(GL_BLEND);
	glBlendColor(0.0, 0.8, 0.2, 1.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POLYGON_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

	glBegin(GL_LINES);
	const std::vector<mesh_edge_ptr>& mesh_edges = m_mesh->get_edges();
	for (const mesh_edge_ptr& edge : mesh_edges)
	{
		const vector3d start_pt = edge->get_vertex()->get_point();
		const vector3d end_pt = edge->get_end_vertex()->get_point();

		if (!edge->is_lamina())
			glColor3d(0.0, 0.0, 0.0);
		else
			glColor3d(1.0, 1.0, 0.0);

		glVertex3d(start_pt.x(), start_pt.y(), start_pt.z());
		glVertex3d(end_pt.x(), end_pt.y(), end_pt.z());
	}
	glEnd(); // GL_LINES

	glDisable(GL_BLEND);

	glEndList();

	build_child_display_lists();
}

//virtual
bbox3d MeshEdgesDisplayObject::GetBBox() const
{
	return m_mesh->bbox();
}
