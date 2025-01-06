//
// Created by aegis on 12/6/24.
//

#ifndef WINDOW_H
#define WINDOW_H

#include <gtkmm-3.0/gtkmm.h>

class Window : public Gtk::Window {
    public:
        Window();
        ~Window();


    private:
        void setup_menu();

        void on_add_files_folders();
        void on_clear_files();
        void on_settings();
        void on_app_quit();
        void on_visuals();
        void on_window_resize(Gtk::Allocation& allocation);

        Gtk::Paned m_paned;
        Gtk::Box m_box;
        Gtk::MenuBar m_menu_bar;

        Gtk::Menu m_file_menu;
        Gtk::MenuItem m_menu_item_file;

        Gtk::MenuItem m_menu_item_file_add_file_folder;
        Gtk::MenuItem m_menu_item_file_clear_files;
        Gtk::MenuItem m_menu_item_file_settings;
        Gtk::MenuItem m_menu_item_file_app_quit;

        Gtk::MenuItem m_menu_item_visuals;

};

#endif //WINDOW_H
