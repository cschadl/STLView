/*
 * stlview.cpp
 *
 *  Created on: May 12, 2013
 *      Author: cds
 */

#include <memory>

#include <gtkglmm.h>
#include <gtkmm.h>

#include "MainWindow.h"

int main(int argc, char** argv)
{
	Gtk::Main kit(argc, argv);
	Gtk::GL::init(argc, argv);

	std::auto_ptr<MainWindow> window(new MainWindow);
	window->resize(640, 480);

	kit.run(*window);

	return 0;
}
