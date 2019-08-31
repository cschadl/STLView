/*
 * GLCamera.cpp
 *
 *  Created on: May 14, 2012
 *      Author: cds
 */

#include <misc.h>

#include <assert.h>
#include <stdexcept>

#include "matrix.h"

#include "GLCamera.h"

using maths::vector3f;
using maths::vector2f;
using maths::matrix;

GLCamera::GLCamera()
: m_rotation(construct_rotation(vector3f(0, 0, 0), vector3f(0, 0, -1), vector3f(0, 1, 0)))
, m_translation(matrix<float>::translation(vector3f(0, 0, 0)))
{

}

GLCamera::GLCamera(const vector3f& origin, const vector3f& look_at, const vector3f& up)
: m_rotation(construct_rotation(origin, look_at, up.make_unit()))
, m_translation(matrix<float>::translation(origin))
{

}

void GLCamera::GetMatrixForModelview() const
{
	GLfloat gl_transform[16];
	matrix<float> m_i(4, 4);
	matrix<float>::invert4x4(m_rotation * m_translation, m_i);
	m_i.transpose().as_array(&gl_transform[0]);

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(gl_transform);
}

vector3f GLCamera::GetLookVector() const
{
	vector3f look_vector(-m_translation(0, 3), -m_translation(1, 3), -m_translation(2, 3));
	return look_vector;
}

float GLCamera::GetViewDistance() const
{
	return GetLookVector().length();
}

GLCamera& GLCamera::Pan(const vector2f& dxy)
{
	const vector3f dxyz(-dxy.x(), -dxy.y(), 0.0);
	const matrix<float> t = matrix<float>::translation(dxyz);
	m_translation = m_translation * t;

	return *this;
}

GLCamera& GLCamera::Orbit(const vector3f& axis, float angle_deg)
{
	const matrix<float> r = matrix<float>::rotation(axis, -angle_deg);
	m_rotation = m_rotation * r;

	return *this;
}

GLCamera& GLCamera::Zoom(const float dist)
{
	// TODO - this sucks
	vector3f zoom_vector;
	zoom_vector.z() = dist;

	const matrix<float> z = matrix<float>::translation(zoom_vector);
	m_translation = z * m_translation;

	return *this;
}

void GLCamera::LookAt(const vector3f& origin, const vector3f& look_at, const vector3f& up)
{
	*this = GLCamera(origin, look_at, up);
	GetMatrixForModelview();
}

//static
matrix<float> GLCamera::construct_rotation(const vector3f& origin, const vector3f& look_at, const vector3f& up)
{
	const vector3f n = (origin - look_at).unit();
	const vector3f u = up % n;
	const vector3f v = n % u;

	matrix<float> m(4, 4);
	m(0, 0) = u.x(); m(0, 1) = v.x(); m(0, 2) = n.x(); m(0, 3) = 0.0;
	m(1, 0) = u.y(); m(1, 1) = v.y(); m(1, 2) = n.y(); m(1, 3) = 0.0;
	m(2, 0) = u.z(); m(2, 1) = v.z(); m(2, 2) = n.z(); m(3, 3) = 0.0;
	m(3, 0) = 0.0;   m(3, 1) = 0.0;   m(3, 2) = 0.0;   m(3, 3) = 1.0;

	return m;
}
