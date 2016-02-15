/*
 * STLDrawArea.h
 *
 *  Created on: May 5, 2013
 *      Author: cds
 */

#ifndef STLDRAWAREA_H_
#define STLDRAWAREA_H_

#include <memory>

#include <gtkglmm.h>
#include <gdkmm.h>

#include <vectors.h>
#include <geom.h>

#include <GL/gl.h>

#include "GLCamera.h"

class triangle_mesh;
class mesh_facet;
class DisplayObject;

class STLDrawArea : public Gtk::GL::DrawingArea
{
private:
	// Rotation state
	bool			m_is_dragging;
	maths::vector3f	m_last_track_pt;
	maths::vector2f m_last_drag_pt;
	GLfloat			m_obj_rot_matrix[16];
	GLfloat			m_zoom_factor;
	GLCamera		m_camera;

	std::shared_ptr<DisplayObject>	m_mesh_do;

public:
	STLDrawArea();
	virtual ~STLDrawArea() { }

	// Creates a GL display list for the given mesh
	void InitMeshDO(const std::shared_ptr<triangle_mesh>& mesh, bool include_edges);

	bool HasMeshDO() const { return !!m_mesh_do; }

	void Redraw();		///< Redraws the view
	void CenterView();	///< Centers the view and redraws

protected:

	// Helper function for getting the trackball point given the X, Y screen coordinates
	maths::vector3f get_trackball_point(int x, int y) const;
	maths::vector2f get_drag_point(int x, int y) const;

	void init_gl();

	void setup_lighting();
	void resize(GLuint width, GLuint height);	// Called from on_expose_event()

	maths::bbox3d get_mesh_bbox() const;

	void camera_rotate(const maths::vector3f& axis, const float rot_angle_deg);
	//void object_rotate(const maths::vector3f& axis, const float rot_angle_deg);
	void camera_pan(const maths::vector2f& dxy);	// drag origin with mouse
	void camera_zoom(const float dz);				// zoom with mouse wheel

	virtual void on_realize();
	virtual bool on_configure_event(GdkEventConfigure* event);
	virtual bool on_expose_event(GdkEventExpose* event);
	virtual bool on_motion_notify_event(GdkEventMotion* event);
	virtual bool on_button_press_event(GdkEventButton* button);
	virtual bool on_button_release_event(GdkEventButton* button);
};

#endif /* STLDRAWAREA_H_ */
