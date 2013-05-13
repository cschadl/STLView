/*
 * GLCamera.h
 *
 *  Created on: May 14, 2012
 *      Author: cds
 */

#ifndef GLCAMERA_H_
#define GLCAMERA_H_

#include <matrix.h>
#include <vectors.h>
#include <GL/gl.h>

class GLCamera
{
protected:
	maths::matrix<float>	m_rotation;	///< Camera rotation matrix (not inverted)
	maths::matrix<float>	m_translation;	///< Camera translation matrix (not inverted)

public:
	/**
	 *	Default constructor.
	 *	Camera is centered at origin, pointing at (0, 0, -1), with (0, 1, 0) as the up vector.
	 */
	GLCamera();

	/**
	 * Constructor.
	 */
	GLCamera(const maths::vector3f& origin,		/**< The camera origin (the "eye coordinate") */
			 const maths::vector3f& look_at,	/**< Coordinate to point the camera at */
			 const maths::vector3f& up			/**< Up vector for the camera */);

//	GLCamera& SetOrigin(const maths::vector3f& p) 	{ m_origin = p; return *this; }
//	GLCamera& LookAt(const maths::vector3f& p)		{ m_look_point = p; return *this; }
//	GLCamera& SetUpVector(const maths::vector3f& p) { m_up = p; return *this; }


	/**
	 * Pans the camera in XY
	 */
	GLCamera& Pan(const maths::vector2f& dxy);

	/**
	 *  Rotates the camera origin about the look_at point.
	 */
	GLCamera& Orbit(const maths::vector3f& axis, const float angle_deg);

	/**
	 * "Zooms" the camera closer to the look point.
	 */
	GLCamera& Zoom(const float dist);

	/**
	 * Loads the camera matrix onto the GL_MODELVIEW stack.
	 * @remark  this is actually the inverse of the view matrix defined by this camera
	 */
	void GetMatrixForModelview() const;

	maths::vector3f GetLookVector() const;

	float GetViewDistance() const;

	/**
	 * Like GLU's gluLookAt() function.
	 * Creates a camera at the specified coordinates, looking at the specified point,
	 * and loads it into the GL_MODELVIEW stack.
	 */
	void LookAt(const maths::vector3f& origin,
				const maths::vector3f& look_at,
				const maths::vector3f& up);

protected:
	// Constructs the rotation matrix from the given vectors
	static maths::matrix<float> construct_rotation(const maths::vector3f& origin,
												   const maths::vector3f& look_at,
												   const maths::vector3f& up);
};

#endif /* GLCAMERA_H_ */
