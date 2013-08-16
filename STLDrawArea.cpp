/*
 * STLDrawArea.cpp
 *
 *  Created on: May 5, 2013
 *      Author: cds
 */

#include "STLDrawArea.h"
#include "DisplayObject.h"
#include "triangle_mesh.h"
#include <assert.h>

using Glib::RefPtr;
using Gdk::GL::Drawable;
using maths::vector3f;
using maths::vector2f;
using maths::vector3d;
using boost::shared_ptr;

STLDrawArea::STLDrawArea()
: m_is_dragging(false)
, m_zoom_factor(1.0f)

{
	// Initialize a double-buffered RGB visual
	const Gdk::GL::ConfigMode mode = Gdk::GL::MODE_RGB | Gdk::GL::MODE_DEPTH | Gdk::GL::MODE_DOUBLE;
	RefPtr<Gdk::GL::Config> gl_config = Gdk::GL::Config::create(mode);

	if (!gl_config)
		throw std::runtime_error("Unable to create OpenGL display context!");

	if (!set_gl_capability(gl_config))
		throw std::runtime_error("Could not enable GL capability for Drawable widget!");

	// Set up events for mouse-down, mouse-up, mouse movement, and mouse scroll
	add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::POINTER_MOTION_MASK | Gdk::SCROLL_MASK);
}

void STLDrawArea::DrawMesh(shared_ptr<triangle_mesh> mesh)
{
	m_zoom_factor = 1.0f;

	glShadeModel(GL_SMOOTH);

	// TODO - selectable color
	//const GLfloat green[] = {0.0, 0.8, 0.2, 1.0};	// TODO - adjustable alpha

	shared_ptr<DisplayObject> mesh_do(new MeshDisplayObject(mesh));
	shared_ptr<DisplayObject> edge_do(new MeshEdgesDisplayObject(mesh));
	m_mesh_do = shared_ptr<DisplayObject>(mesh_do);
	m_mesh_do->AddChild(edge_do);
	m_mesh_do->BuildDisplayLists();

	center_view();	// redraws
}

vector3f STLDrawArea::get_trackball_point(int x, int y) const
{
	const vector2f dxy = get_drag_point(x, y);
	const float dxy_len = dxy.length();

	const float pi2 = M_2_PI;
	const float dz = cos(pi2 * dxy_len < 1.0 ? dxy_len : 1.0);

	const vector3f dxyz(dxy.x(), dxy.y(), dz);

	return dxyz.make_unit();
}

vector2f STLDrawArea::get_drag_point(int x, int y) const
{
	const float width = (float) get_width();
	const float height = (float) get_height();

	const float dx = (2.0f * x - width) / width;
	const float dy = (height - 2.0f * y) / height;

	return vector2f(dx, dy);
}

void STLDrawArea::init_gl()
{
	RefPtr<Drawable> gl_drawable = get_gl_drawable();
	gl_drawable->gl_begin(get_gl_context());

	// Initialize rotation matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glGetFloatv(GL_MODELVIEW_MATRIX, m_obj_rot_matrix);

	assert(glGetError() == GL_NO_ERROR);

	gl_drawable->gl_end();

	setup_lighting();
}

void STLDrawArea::setup_lighting()
{
	RefPtr<Drawable> gl_drawable = get_gl_drawable();
	gl_drawable->gl_begin(get_gl_context());

	GLfloat mat_ambient_diff[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_ambient_diff);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	GLfloat light_position_1[] = { 5.0f, 5.0f, 10.0f, 1.0f };
	GLfloat light_position_2[] = { 0.0f, 10.0f, 0.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, light_position_1);
	glLightfv(GL_LIGHT1, GL_POSITION, light_position_2);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);

	assert(glGetError() == GL_NO_ERROR);

	gl_drawable->gl_end();
}

void STLDrawArea::resize(GLuint width, GLuint height)
{
	RefPtr<Drawable> gl_drawable = get_gl_drawable();
	gl_drawable->gl_begin(get_gl_context());

	const GLfloat aspect = (GLfloat) width / (GLfloat) height;
	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	const maths::bbox3d mesh_bbox = get_mesh_bbox();

	const double c_x = mesh_bbox.is_empty() ? 0.0 : mesh_bbox.center().x();
	const double c_y = mesh_bbox.is_empty() ? 0.0 : mesh_bbox.center().y();
	const double diam = mesh_bbox.is_empty() ? 1.0 : mesh_bbox.max_extent();
	double left = c_x - diam;
	double right = c_x + diam;
	double bottom = c_y - diam;
	double top = c_y + diam;

	const double z_near = -2.0 * (m_camera.GetViewDistance() + diam);
	const double z_far = 2.0 * (m_camera.GetViewDistance() + diam);

	if (aspect < 1.0)
	{
		bottom /= aspect;
		top /= aspect;
	}
	else
	{
		left *= aspect;
		right *= aspect;
	}

	//const float view_dist = m_camera.GetViewDistance() == 0.0 ? 1.0 : m_camera.GetViewDistance();
	glOrtho(m_zoom_factor * left, m_zoom_factor * right, m_zoom_factor * bottom, m_zoom_factor * top, z_near, z_far);

	assert(glGetError() == GL_NO_ERROR);

	gl_drawable->gl_end();
}

void STLDrawArea::redraw()
{
	RefPtr<Drawable> gl_drawable = get_gl_drawable();
	gl_drawable->gl_begin(get_gl_context());

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(1.0, 1.0, 1.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Load "view" matrix onto GL_MODELVIEW stack
	m_camera.GetMatrixForModelview();

	glEnable(GL_CULL_FACE);
	// Draw stuff
	glPushMatrix();
	glMultMatrixf(m_obj_rot_matrix);

	// TODO - move obj rot matrix to DisplayObject::Draw
	if (m_mesh_do)
		m_mesh_do->Draw();

	glPopMatrix();
	glDisable(GL_CULL_FACE);

	assert(glGetError() == GL_NO_ERROR);

	gl_drawable->swap_buffers();
	gl_drawable->gl_end();
}

void STLDrawArea::center_view()
{
	assert(!get_mesh_bbox().is_empty());

	const vector3d mesh_c = get_mesh_bbox().center();
	const double diam = std::max(get_mesh_bbox().extent_x(), get_mesh_bbox().extent_y());

	vector3f origin(0.0f, 0.0f, 2 * diam);
	vector3f view_dir((float) mesh_c.x(), (float) mesh_c.y(), (float) mesh_c.z());
	m_camera.LookAt(origin, view_dir, vector3f(0.0f, 1.0f, 0.0f));

	resize(get_width(), get_height());
	redraw();
}

maths::bbox3d STLDrawArea::get_mesh_bbox() const
{
	maths::bbox3d bbox;
	if (m_mesh_do)
		bbox = m_mesh_do->GetBBox();

	return bbox;
}

void STLDrawArea::camera_rotate(const vector3f& axis, const float rot_angle_deg)
{
	m_camera.Orbit(axis, rot_angle_deg);

	redraw();
}

void STLDrawArea::camera_pan(const vector2f& dxy)
{
	m_camera.Pan(dxy);

	redraw();
}

void STLDrawArea::camera_zoom(const float dz)
{
	//m_camera.Zoom(dz);
	m_zoom_factor += dz;

	// Orothographic projection needs a resize after zooming
	resize(get_width(), get_height());
	redraw();
}

bool STLDrawArea::on_configure_event(GdkEventConfigure * event)
{
	resize(get_width(), get_height());

	return true;
}

bool STLDrawArea::on_expose_event(GdkEventExpose * event)
{
	redraw();

	return true;
}

bool STLDrawArea::on_button_press_event(GdkEventButton * event)
{
	if (event->type == GDK_BUTTON_PRESS)
	{
		m_is_dragging = true;
		switch (event->button)
		{
		case 1:	// rotate
			m_last_track_pt = get_trackball_point((int) event->x, (int) event->y);
			break;
		case 3:	// pan / zoom
			m_last_drag_pt = get_drag_point((int) event->x, (int) event->y);
			break;
		default:
			m_is_dragging = false;
		}
	}

	return true;
}

bool STLDrawArea::on_button_release_event(GdkEventButton * event)
{
	if (event->type == GDK_BUTTON_RELEASE && (event->button == 1 || event->button == 3))
		m_is_dragging = false;

	return true;
}

bool STLDrawArea::on_motion_notify_event(GdkEventMotion * event)
{
	if (!m_is_dragging)
		return true;

	if (event->state & Gdk::BUTTON3_MASK)
	{
		const vector2f cur_drag_point = get_drag_point((int) event->x, (int) event->y);
		const vector2f dxy = (cur_drag_point - m_last_drag_pt);
		const float zoom_sensitivity = 5.0f;

		if (event->state & Gdk::CONTROL_MASK)
			camera_zoom(dxy.y() * zoom_sensitivity);
		else
			camera_pan(dxy * m_camera.GetViewDistance());

		m_last_drag_pt = cur_drag_point;

		return true;
	}

	const vector3f cur_track_pt = get_trackball_point((int) event->x, (int) event->y);
	const vector3f dv = cur_track_pt - m_last_track_pt;

	vector3f rot_axis = m_last_track_pt % cur_track_pt;
	if (rot_axis.length() > 1.0e-5)
	{
		const float rot_angle_deg = 90.0f * dv.length();
		rot_axis.unit();

		camera_rotate(rot_axis, rot_angle_deg);

		m_last_track_pt = cur_track_pt;
	}

	return true;
}

void STLDrawArea::on_realize()
{
	Gtk::GL::DrawingArea::on_realize();

	const char* version_string = (const char *) glGetString(GL_VERSION);
	std::cout << version_string << std::endl;

	init_gl();
}

