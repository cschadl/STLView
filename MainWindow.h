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
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

class triangle_mesh;

class MainWindow : public Gtk::Window
{
private:
	boost::scoped_ptr<STLDrawArea>		m_stlDrawArea;
	boost::shared_ptr<triangle_mesh>	m_mesh;	// The current mesh to display

	Gtk::VBox		m_vBox;
	Gtk::MenuBar	m_menuBar;

	static const Glib::ustring		APP_NAME;

public:
	MainWindow();
	virtual ~MainWindow() { }

	/** Display an informational message box with an OK button.
	 *  @param	title	The title of the message box
	 *  @param	msg		msg The message to display
	 *  @returns		The message box response
	 */
	int DoMessageBox(const Glib::ustring& title, const Glib::ustring& msg) const;

protected:
	// Signal handlers
	//virtual bool on_key_press_event(GdkEventKey * event);

	// Callbacks
	void do_file_open_dialog();

	// Other stuff

	/** Sets the current window title.
	 *  @param current_fn	The current file that is open.
	 *  					Empty if no file is currently open.
	 */
	void set_window_title(const Glib::ustring& current_fn);

	void file_open(const Glib::ustring& filename);
};

#endif /* MAINWINDOW_H_ */
