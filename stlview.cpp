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

	const int width_default = 1024;
	const int height_default = 768;

	std::unique_ptr<MainWindow> window(new MainWindow);	// TODO - file open command-line?
	window->resize(width_default, height_default);

	kit.run(*window);

	return 0;
}
