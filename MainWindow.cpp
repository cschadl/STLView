/*
 * MainWindow.cpp
 *
 *  Created on: May 4, 2013
 *      Author: cds
 */

#include "MainWindow.h"
#include "STLDrawArea.h"
#include "stl_import.h"
#include "triangle_mesh.h"

#include <fstream>
#include <sstream>
#include <exception>

#ifndef DEBUG
const Glib::ustring MainWindow::APP_NAME = "STLView";
#else
const Glib::ustring MainWindow::APP_NAME = "STLview (Debug)";
#endif

ScopedWaitCursor::ScopedWaitCursor(Gtk::Widget& widget)
: m_window(widget.get_window())
{
	if (!m_window)
		throw std::runtime_error("Couldn't get Gdk::Window from widget!");

	Gdk::Cursor wait_cursor(Gdk::WATCH);
	m_window->set_cursor(wait_cursor);

	while (Gtk::Main::events_pending())
		Gtk::Main::iteration();	//???
}

ScopedWaitCursor::~ScopedWaitCursor()
{
	m_window->set_cursor();
	while (Gtk::Main::events_pending())
		Gtk::Main::iteration();
}

MainWindow::MainWindow()
: m_stlDrawArea(new STLDrawArea)
, m_vBox(false /* homogeneous */, 0 /* spacing */)
{
	set_window_title("");

	add(m_vBox);

	{
		// Add stuff to menu bar
		Gtk::MenuItem* file_menubar_item 	= Gtk::manage(new Gtk::MenuItem("File"));
		Gtk::Menu* file_menu 				= Gtk::manage(new Gtk::Menu());
		Gtk::MenuItem* file_open 			= Gtk::manage(new Gtk::MenuItem("Open"));
		Gtk::MenuItem* file_quit 			= Gtk::manage(new Gtk::MenuItem("Quit"));

		file_menu->append(*file_open);
		file_open->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::do_file_open_dialog));
		file_open->show();

		file_menu->append(*file_quit);
		file_quit->signal_activate().connect(sigc::ptr_fun(&Gtk::Main::quit));
		file_quit->show();

		file_menubar_item->set_submenu(*file_menu);
		file_menubar_item->show();

		m_menuBar.append(*file_menubar_item);
	}

	m_vBox.pack_start(m_menuBar, Gtk::PACK_SHRINK);
	m_vBox.pack_end(*m_stlDrawArea, Gtk::PACK_EXPAND_WIDGET, 0);

	show_all();
}

int MainWindow::DoMessageBox(const Glib::ustring& title, const Glib::ustring& msg) const
{
	// TODO - this message box is pretty tiny
	// maybe add an icon or something to it?

	Gtk::Dialog dlg(title);
	Gtk::Label label(msg);

	dlg.get_vbox()->pack_start(label, Gtk::PACK_EXPAND_WIDGET, 0);
	dlg.add_button("OK", Gtk::RESPONSE_OK);
	dlg.show_all();

	return dlg.run();
}

void MainWindow::do_file_open_dialog()
{
	Gtk::FileChooserDialog fcd(*this /* parent */, "Open File" /* title */);
	fcd.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);
	fcd.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);

	Gtk::FileFilter stl_files;
	stl_files.set_name("STL Files");
	stl_files.add_pattern("*.stl");
	stl_files.add_pattern("*.STL");
	fcd.add_filter(stl_files);

	Gtk::FileFilter all_files;
	all_files.set_name("All Files");
	all_files.add_pattern("*");
	fcd.add_filter(all_files);

	const int response = fcd.run();
	const Glib::ustring filename = fcd.get_filename();

	fcd.hide_all();
	if (response == Gtk::RESPONSE_OK)
		file_open(filename);
}

void MainWindow::set_window_title(const Glib::ustring& current_fn)
{
	std::ostringstream title_ss;
	title_ss << APP_NAME.c_str() <<  " - " << current_fn.c_str();

	set_title(title_ss.str().c_str());
}

void MainWindow::file_open(const Glib::ustring& filename)
{
	ScopedWaitCursor wc(*this);

	boost::shared_ptr<triangle_mesh> mesh;
	try
	{
		std::ifstream in_stream;
		in_stream.open(filename.c_str(), std::ifstream::in);

		// Why is the instream not open / good?
		//if (!in_stream.is_open() || !in_stream.good());
		//	throw std::runtime_error("Error opening file");

		stl_import importer(in_stream);
		mesh = boost::shared_ptr<triangle_mesh>(new triangle_mesh(importer.get_facets()));
	}
	catch (std::exception& ex)
	{
		std::stringstream ss;
		ss << "There was an error reading the STL file: " << std::endl << ex.what();
		std::cout << ss.str() << std::endl;

		DoMessageBox("Error", ss.str().c_str());
		return;
	}

	// We should have a mesh now
	if (!mesh)
		throw std::runtime_error("no mesh (weird)");

	set_window_title(filename);

	m_mesh = mesh;
	m_stlDrawArea->DrawMesh(m_mesh);
}
