/*
 * MainWindow.h
 *
 *  Created on: May 4, 2013
 *      Author: cds
 */

#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include "STLDrawArea.h"

#include <string>
#include <memory>
#include <gtkmm.h>
#include <gtkmm/box.h>

class triangle_mesh;

/**	Displays a wait cursor for the given window until the object goes out of scope
 *  Does Gtkmm not have this? */
class ScopedWaitCursor
{
private:
	// This window is obtained from the Gtk::Widget
	// Must be "realizable"
	Glib::RefPtr<Gdk::Window>	m_window;

public:
	/** Constructor.
	 *  Sets the wait cursor for the given widget.
	 *  @throws std::runtime_exception if the widget does not
	 *  		have a Gtk::Window.
	 */
	ScopedWaitCursor(Gtk::Widget& widget);
	~ScopedWaitCursor();
};

class MainWindow : public Gtk::Window
{
private:
	std::unique_ptr<STLDrawArea>	m_stlDrawArea;
	std::shared_ptr<triangle_mesh>	m_mesh;	// The current mesh to display

	Gtk::VBox		m_vBox;
	Gtk::MenuBar	m_menuBar;

	bool 			m_show_edges;

	static const Glib::ustring		APP_NAME;
	static const Glib::ustring		MENU_ITEM_DATA_KEYNAME;

	static const size_t				MENU_ITEM_MESH_INFO_ID;
	static const size_t				MENU_ITEM_ENABLE_BFC_ID;

public:
	MainWindow();
	virtual ~MainWindow() { }

	/** Display an informational message box with an OK button.
	 *  @param	title	The title of the message box
	 *  @param	msg		msg The message to display
	 *  @returns		The message box response
	 */
	int DoMessageBox(const Glib::ustring& title, const Glib::ustring& msg);

	void FileOpen(const Glib::ustring& filename);

protected:
	// Signal handlers
	//virtual bool on_key_press_event(GdkEventKey * event);

	// Callbacks
	void do_file_open_dialog();
	void on_view_show_edges();
	void on_view_enable_back_face_culling();
	void on_view_mesh_info();
	void on_help_opengl_info();

	// Other stuff

	/** Sets the current window title.
	 *  @param current_fn	The current file that is open.
	 *  					Empty if no file is currently open.
	 */
	void set_window_title(const Glib::ustring& current_fn);

	/** Gets the menu item with get_name() == menu_item_name
	 *
	 */
	Gtk::MenuItem* get_menu_item(size_t menu_id);

	static Glib::RefPtr<Gdk::Pixbuf> get_application_icon();
};

#endif /* MAINWINDOW_H_ */
