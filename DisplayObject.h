/*
 * DisplayObject.h
 *
 *  Created on: May 27, 2013
 *      Author: cds
 */

#ifndef DISPLAYOBJECT_H_
#define DISPLAYOBJECT_H_

#include <matrix.h>
#include <geom.h>

#include <vector>
#include <memory>

class triangle_mesh;

// An OpenGL display object
// All displayed objects must subclass this object
class DisplayObject
{
public:
	typedef std::shared_ptr<DisplayObject>			DOPtr;
	typedef std::shared_ptr<const DisplayObject>	ConstDOPtr;

private:
	GLuint						m_display_id;
	maths::matrix<float>		m_transform;
	std::vector<DOPtr>			m_children;
	bool						m_suppressed;

protected:
	void 	build_child_display_lists();
	GLuint	display_id() const 							{ return m_display_id; }
			maths::matrix<float>& transform() 			{ return m_transform; }
	const 	maths::matrix<float>& transform() const 	{ return m_transform; }

public:
	DisplayObject();
	virtual ~DisplayObject();

	/** Builds the OpenGL display lists for this object.
	 *  The implementation is responsible for things like calling glNewList,
	 *  generating call lists for children, etc.
	 */
	virtual void BuildDisplayLists() = 0;
	virtual maths::bbox3d GetBBox() const = 0;

	void AddChild(const DOPtr& display_object);
	void RemoveChild(const DOPtr& display_object);
	void RemoveAllChildren() { m_children.clear(); }
	const std::vector<DOPtr>& GetChildren() const { return m_children; }

	/** @{
	 *  If true, then this DisplayObject and all children will not be drawn
	 */
	bool& Suppressed() { return m_suppressed; }
	const bool& Suppressed() const { return m_suppressed; }
	/** @} */

	/** Calls all display lists */
	void Draw() const;
};

class MeshDisplayObject : public DisplayObject
{
private:
	std::shared_ptr<triangle_mesh> m_mesh;

	static bool is_sharp_edge_boundary(const mesh_facet & f1, const mesh_facet & f2);

public:
	MeshDisplayObject(std::shared_ptr<triangle_mesh> mesh);

	virtual void BuildDisplayLists();
	virtual maths::bbox3d GetBBox() const;
};

class MeshEdgesDisplayObject : public DisplayObject
{
private:
	std::shared_ptr<triangle_mesh> m_mesh;

public:
	MeshEdgesDisplayObject(std::shared_ptr<triangle_mesh> mesh);

	virtual void BuildDisplayLists();
	virtual maths::bbox3d GetBBox() const;
};

#endif /* DISPLAYOBJECT_H_ */
