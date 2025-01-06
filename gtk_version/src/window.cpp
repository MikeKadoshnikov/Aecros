#include "window.h"
#include <iostream>

Window::Window() :
	m_box(Gtk::ORIENTATION_VERTICAL),
	m_menu_item_file("_File", true),
	m_menu_item_file_add_file_folder("_Add File/Folder", true),
	m_menu_item_file_clear_files("_Clear Files", true),
	m_menu_item_file_settings("_Settings", true),
	m_menu_item_file_app_quit("_Quit", true),
	m_menu_item_visuals("_Visuals", true)
{
    set_title("Aecros");
    set_resizable(true);
	set_size_request(800, 600);
    set_default_size(800, 600);

	setup_menu();

	show_all_children();
}

Window::~Window() {
  std::cout << "Destroying window" << std::endl;
}

void Window::setup_menu() {
	m_file_menu.append(m_menu_item_file_add_file_folder);
	m_file_menu.append(m_menu_item_file_clear_files);
	m_file_menu.append(m_menu_item_file_settings);
	m_file_menu.append(m_menu_item_file_app_quit);

	m_menu_item_file.set_submenu(m_file_menu);

	m_menu_bar.append(m_menu_item_file);
	m_menu_bar.append(m_menu_item_visuals);

	m_menu_item_file_add_file_folder.signal_activate().connect(sigc::mem_fun(*this, &Window::on_add_files_folders));
	m_menu_item_file_clear_files.signal_activate().connect(sigc::mem_fun(*this, &Window::on_clear_files));
	m_menu_item_file_settings.signal_activate().connect(sigc::mem_fun(*this, &Window::on_settings));
	m_menu_item_file_app_quit.signal_activate().connect(sigc::mem_fun(*this, &Window::on_app_quit));

	m_menu_item_visuals.signal_activate().connect(sigc::mem_fun(*this, &Window::on_visuals));

	m_box.pack_start(m_menu_bar, Gtk::PACK_SHRINK);
	m_paned.set_orientation(Gtk::ORIENTATION_VERTICAL);
	m_paned.add1(m_box);

	add(m_paned);

	signal_size_allocate().connect(sigc::mem_fun(*this, &Window::on_window_resize));
}

void Window::on_add_files_folders() {
	std::cout << "Adding folders" << std::endl;
}

void Window::on_clear_files() {
	std::cout << "Clearing files" << std::endl;
}

void Window::on_settings() {
	std::cout << "Settings..." << std::endl;
}

void Window::on_app_quit() {
	std::cout << "Quitting..." << std::endl;
	exit(0);
}

void Window::on_visuals() {
	std::cout << "Visuals" << std::endl;
}

void Window::on_window_resize(Gtk::Allocation& allocation) {
  int window_height = allocation.get_height();
  int pane_position = static_cast<int>(window_height * 0.8);
  m_paned.set_position(pane_position);
}