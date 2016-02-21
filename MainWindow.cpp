/*
 * MainWindow.cpp
 *
 *  Created on: May 4, 2013
 *      Author: cds
 */

#include "MainWindow.h"
#include "STLDrawArea.h"
#include "DisplayObject.h"

#include "stl_import.h"
#include "triangle_mesh.h"

#include <gtkmm/accelgroup.h>

#include <fstream>
#include <sstream>
#include <exception>
#include <memory>
#include <iomanip>

#include <string.h>
#include <errno.h>

#ifndef DEBUG
const Glib::ustring MainWindow::APP_NAME = "STLView";
#else
const Glib::ustring MainWindow::APP_NAME = "STLview (Debug)";
#endif

const Glib::ustring MainWindow::MENU_ITEM_DATA_KEYNAME = "MENU_ITEM_DATA";

const size_t MainWindow::MENU_ITEM_MESH_INFO_ID = 0x8001;

using std::shared_ptr;
using std::unique_ptr;

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
, m_show_edges(true)
{
	set_window_title("");

	add(m_vBox);

	// Add stuff to menu bar
	Gtk::MenuItem*	file_menubar_item 	= Gtk::manage(new Gtk::MenuItem("File"));
	Gtk::Menu* 		file_menu 			= Gtk::manage(new Gtk::Menu());
	Gtk::MenuItem*	file_open 			= Gtk::manage(new Gtk::MenuItem("Open..."));
	Gtk::MenuItem*	file_quit 			= Gtk::manage(new Gtk::MenuItem("Quit"));

	Gtk::MenuItem*		view_menubar_item	= Gtk::manage(new Gtk::MenuItem("View"));
	Gtk::Menu*			view_menu			= Gtk::manage(new Gtk::Menu());
	Gtk::CheckMenuItem*	view_show_edges		= Gtk::manage(new Gtk::CheckMenuItem("Show Edges"));
	Gtk::MenuItem* 		view_mesh_info		= Gtk::manage(new Gtk::MenuItem("Mesh Info..."));

	Gtk::MenuItem*	help_menubar_item	= Gtk::manage(new Gtk::MenuItem("Help"));
	Gtk::Menu*		help_menu			= Gtk::manage(new Gtk::Menu());
	Gtk::MenuItem*	help_opengl_info	= Gtk::manage(new Gtk::MenuItem("OpenGL Info..."));

	file_menu->append(*file_open);
	file_open->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::do_file_open_dialog));
	file_open->show();

	file_menu->append(*file_quit);
	file_quit->signal_activate().connect(sigc::ptr_fun(&Gtk::Main::quit));
	file_quit->show();

	file_menubar_item->set_submenu(*file_menu);
	file_menubar_item->show();

	view_menu->append(*view_show_edges);
	view_show_edges->set_active(m_show_edges);	// do this before we connect the callback
	view_show_edges->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_view_show_edges));
	view_show_edges->show();

	view_menu->append(*view_mesh_info);
	view_mesh_info->set_sensitive(!!m_mesh);
	view_mesh_info->set_data(MENU_ITEM_DATA_KEYNAME, (void *) MENU_ITEM_MESH_INFO_ID);
	view_mesh_info->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_view_mesh_info));
	view_mesh_info->show();

	view_menubar_item->set_submenu(*view_menu);
	view_menubar_item->show();

	help_menu->append(*help_opengl_info);
	help_opengl_info->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_help_opengl_info));
	help_opengl_info->show();

	help_menubar_item->set_submenu(*help_menu);
	help_menubar_item->show();

	m_menuBar.append(*file_menubar_item);
	m_menuBar.append(*view_menubar_item);
	m_menuBar.append(*help_menubar_item);

	file_menu->set_accel_group(get_accel_group());
	file_open->add_accelerator(	"activate", get_accel_group(),
								GDK_o, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);

	view_menu->set_accel_group(get_accel_group());
	view_show_edges->add_accelerator("activate", get_accel_group(),
									 GDK_e, Gdk::ModifierType::CONTROL_MASK, Gtk::AccelFlags::ACCEL_VISIBLE);

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

void MainWindow::FileOpen(const Glib::ustring& filename)
{
	ScopedWaitCursor wc(*this);

	shared_ptr<triangle_mesh> mesh;
	try
	{
		std::ifstream in_stream;
		in_stream.open(filename.c_str(), std::ifstream::in);

		if (in_stream.fail())
			throw std::runtime_error(std::string("Error opening file: ") + ::strerror(errno));

		stl_import importer(in_stream);
		mesh = std::make_shared<triangle_mesh>(importer.get_facets());
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

	Gtk::MenuItem* mesh_info_item = get_menu_item(MENU_ITEM_MESH_INFO_ID);
	mesh_info_item->set_sensitive(true);

	m_mesh = mesh;

	m_stlDrawArea->InitMeshDO(m_mesh, m_show_edges);
	m_stlDrawArea->CenterView();
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
		FileOpen(filename);
}

void MainWindow::on_view_show_edges()
{
	m_show_edges = !m_show_edges;

	if (!m_mesh)
		return;

	ScopedWaitCursor wc(*this);

	auto& do_children = m_stlDrawArea->GetDisplayObject()->GetChildren();
	DisplayObject::DOPtr& mesh_edges = do_children.front();

	mesh_edges->Suppressed() = !m_show_edges;

	m_stlDrawArea->Redraw();
}

void MainWindow::on_view_mesh_info()
{
	if (!m_mesh)
		return;

	std::stringstream ss;
	ss	<< "Number of facets: " << m_mesh->get_facets().size() << std::endl
		<< "Number of edges: " << m_mesh->get_edges().size() << std::endl
		<< "Number of vertices: " << m_mesh->get_vertices().size() << std::endl
		<< "Number of lamina edges: " << m_mesh->get_lamina_edges().size() << std::endl
		<< "Volume: " << m_mesh->volume() << std::endl
		<< "Area: " << m_mesh->area() << std::endl
		<< "Is Closed: " << (m_mesh->is_manifold() ? "TRUE" : "FALSE") << std::endl << std::endl
		<< "BBox dimensions: " << std::endl <<
		std::setprecision(4) << "X: " << m_mesh->bbox().extent_x() << " "
							 << "Y: " << m_mesh->bbox().extent_y() << " "
							 << "Z: " << m_mesh->bbox().extent_z() << std::endl;

	DoMessageBox("Mesh Info", ss.str().c_str());
}

void MainWindow::on_help_opengl_info()
{
	auto renderer_string = (const char*) glGetString(GL_RENDERER);
	auto version_string = (const char*) glGetString(GL_VERSION);
	auto vendor_string = (const char*) glGetString(GL_VENDOR);
	auto sl_string = (const char*) glGetString(GL_SHADING_LANGUAGE_VERSION);

	std::stringstream ss;
	ss << "GL_RENDERER : " << renderer_string << std::endl;
	ss << "GL_VERSION : " << version_string << std::endl;
	ss << "GL_VENDOR: "	 << vendor_string << std::endl;
	ss << "GL_SHADING_LANGUAGE_VERSION: " << sl_string << std::endl;

	DoMessageBox("OpenGL Info", ss.str().c_str());
}

void MainWindow::set_window_title(const Glib::ustring& current_fn)
{
	std::ostringstream title_ss;
	title_ss << APP_NAME.c_str() <<  " - " << current_fn.c_str();

	set_title(title_ss.str().c_str());
}

Gtk::MenuItem* MainWindow::get_menu_item(size_t menu_id)
{
	Gtk::MenuItem* found_menu_item = nullptr;

	std::vector<Gtk::Widget*> menu_items = m_menuBar.get_children();

	while (!menu_items.empty() && !found_menu_item)
	{
		auto widget = menu_items.front();
		menu_items.erase(menu_items.begin());

		auto menu_item = dynamic_cast<Gtk::MenuItem*>(widget);
		if (!menu_item)
			continue;	// not a menu item

		size_t const menu_item_id = (size_t) menu_item->get_data(MENU_ITEM_DATA_KEYNAME);
		if (menu_item_id == menu_id)
			found_menu_item = menu_item;

		Gtk::Menu* submenu = menu_item->get_submenu();
		if (!found_menu_item && submenu)
		{
			std::vector<Gtk::Widget*> submenu_children = submenu->get_children();
			std::copy(submenu_children.begin(), submenu_children.end(), std::back_inserter(menu_items));
		}
	}

	return found_menu_item;
}

