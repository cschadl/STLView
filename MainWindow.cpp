/*
 * MainWindow.cpp
 *
 *  Created on: May 4, 2013
 *      Author: cds
 */

#include "MainWindow.h"
#include "STLDrawArea.h"
#include "DisplayObject.h"

#include "stl_importer.h"
#include "triangle_mesh.h"

#include <gtkmm/accelgroup.h>

#include <fstream>
#include <sstream>
#include <exception>
#include <memory>
#include <iomanip>

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <libgen.h>

#include <type_traits>
#include <sigc++/sigc++.h>

const Glib::ustring MainWindow::APP_NAME = "STLView";
const Glib::ustring MainWindow::MENU_ITEM_DATA_KEYNAME = "MENU_ITEM_DATA";

const size_t MainWindow::MENU_ITEM_MESH_INFO_ID = 0x8001;

using std::shared_ptr;
using std::unique_ptr;

// Supposedly, this allows lambdas to work with sigc
namespace sigc
{
	template <typename Functor>
	struct functor_trait<Functor, false>
	{
		typedef decltype (::sigc::mem_fun (std::declval<Functor&> (),
							&Functor::operator())) _intermediate;

		typedef typename _intermediate::result_type result_type;
		typedef Functor functor_type;
	};
};

namespace
{
	class mesh_triangle_dispatcher : public std::iterator<std::output_iterator_tag, void, void, void, void>
	{
	private:
		triangle_mesh&	m_mesh;
		Glib::Dispatcher& m_sig;

	public:
		mesh_triangle_dispatcher() = delete;
		mesh_triangle_dispatcher(triangle_mesh& mesh, Glib::Dispatcher& sig)
		: m_mesh(mesh)
		, m_sig(sig)
		{

		}

		/* std::iterator boilerplate */
		mesh_triangle_dispatcher& operator*() { return *this; }
		mesh_triangle_dispatcher& operator++() { return *this; }
		mesh_triangle_dispatcher& operator++(int) { return *this; }

		mesh_triangle_dispatcher& operator=(const maths::triangle3d& t)
		{
			m_mesh.add_triangle(t);
			m_sig();

			return *this;
		}
	};

	class process_stl
	{
	private:
		std::shared_ptr<triangle_mesh>	m_mesh;
		stl_util::stl_importer&			m_importer;
		bool							m_done;

		Glib::Thread*			m_thread;
		Glib::Mutex				m_mutex;

		Glib::Dispatcher		m_sig_facet_processed;
		Glib::Dispatcher		m_sig_done;

		void run()
		{
			m_importer.import(mesh_triangle_dispatcher(*m_mesh, m_sig_facet_processed));
			m_sig_done();
		}

	public:
		process_stl(const std::shared_ptr<triangle_mesh> & mesh, stl_util::stl_importer& importer)
		: m_mesh(mesh)
		, m_importer(importer)
		, m_done(false)
		, m_thread(nullptr)
		{

		}

		~process_stl()
		{
			Glib::Mutex::Lock lock(m_mutex);
			m_done = true;

			if (m_thread)
				m_thread->join();	// block until exit
		}

		Glib::Dispatcher& sig_facet_processed() { return m_sig_facet_processed; }

		Glib::Dispatcher& sig_done() { return m_sig_done; }

		void start()
		{
			if (m_thread)
				return;

			m_thread = Glib::Thread::create(sigc::mem_fun(*this, &process_stl::run));
		}
	};
};

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

	auto app_icon = get_application_icon();
	if (app_icon)
		set_icon(app_icon);

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

int MainWindow::DoMessageBox(const Glib::ustring& title, const Glib::ustring& msg)
{
	// TODO - this message box is pretty tiny
	// maybe add an icon or something to it?

	Gtk::Dialog dlg(title);
	Gtk::Label label(msg);

	dlg.get_vbox()->pack_start(label, Gtk::PACK_EXPAND_WIDGET, 0);
	dlg.add_button("OK", Gtk::RESPONSE_OK);
	dlg.set_transient_for(*this);
	dlg.show_all();

	return dlg.run();
}

void MainWindow::FileOpen(const Glib::ustring& filename)
{
	ScopedWaitCursor wc(*this);

	shared_ptr<triangle_mesh> mesh;
	try
	{
		auto in_stream = std::make_shared<std::ifstream>();
		in_stream->open(filename.c_str(), std::fstream::binary);

		if (in_stream->fail())
			throw std::runtime_error(std::string("Error opening file: ") + ::strerror(errno));

		stl_util::stl_importer importer(in_stream);

		auto tmesh = std::make_shared<triangle_mesh>();

		// Create the progress dialog
		std::unique_ptr<Gtk::Dialog> progress_dialog(new Gtk::Dialog("Opening " + filename));
		Gtk::ProgressBar progress_bar;

		progress_dialog->set_size_request(300, 50);
		progress_dialog->set_resizable(false);
		progress_dialog->get_vbox()->pack_start(progress_bar, Gtk::PACK_EXPAND_WIDGET, 0);
		progress_dialog->set_transient_for(*this);
		progress_dialog->show_all();

		size_t const num_facets = importer.num_facets_expected();
		size_t count = 0;

		// hope there isn't a race condition here...
		process_stl stl_processor(tmesh, importer);

		stl_processor.sig_facet_processed().connect(
				[num_facets, &count, &progress_bar]() {
					count++;
					progress_bar.set_fraction((double) count / (double) num_facets);
				});
		stl_processor.sig_done().connect([&progress_dialog]() { progress_dialog.reset(); });
		stl_processor.start();

		progress_dialog->run();

		mesh = tmesh;
	}
	catch (std::exception& ex)
	{
		std::stringstream ss;
		ss << "There was an error reading the STL file: " << std::endl << ex.what();

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
	std::string app_name = APP_NAME;
#ifdef DEBUG
	app_name += " (Debug)";
#endif

	std::ostringstream title_ss;
	title_ss << app_name <<  " - " << current_fn.c_str();

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

//static
Glib::RefPtr<Gdk::Pixbuf> MainWindow::get_application_icon()
{
	Glib::RefPtr<Gdk::Pixbuf> app_icon;

	// Search the following paths for the icon file:
	// <process directory>/../
	// /usr/share/icons/hicolor/128x128/apps
	// $HOME/.local/share/icons/hicolor/128x128/apps

	std::string home_dir = std::getenv("HOME");

	std::string process_path;
	char proccess_path_buf[512];
	if (::readlink("/proc/self/exe", proccess_path_buf, 512) != -1)
	{
		process_path = ::dirname(proccess_path_buf);
	}

	std::string icon_path = "/share/icons/hicolor/128x128/apps/" + APP_NAME + ".png";

	std::vector<std::string> search_paths;

	if (!process_path.empty())
		search_paths.emplace_back(process_path + "/../" + APP_NAME + ".png");

	search_paths.emplace_back(home_dir + "/.local" + icon_path);
	search_paths.emplace_back("/usr/local" + icon_path);

	auto found_path = std::find_if(search_paths.begin(), search_paths.end(),
		[](const std::string & path) { return ::access(path.c_str(), R_OK) != -1; });

	if (found_path != search_paths.end())
		app_icon = Gdk::Pixbuf::create_from_file(*found_path);

	return app_icon;
}
