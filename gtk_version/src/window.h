//
// Created by aegis on 12/6/24.
//

#ifndef WINDOW_H
#define WINDOW_H

#include <gtkmm-3.0/gtkmm.h>
#include <vector>
#include <string>
#include <gstreamermm.h>

class Window : public Gtk::Window {
    public:
        Window();
        ~Window();

    protected:
        Gtk::Box m_box;
        Gtk::MenuBar m_menu_bar;
        Gtk::Menu m_file_menu;
        Gtk::MenuItem m_menu_item_file;
        Gtk::MenuItem m_menu_item_file_add_file_folder;
        Gtk::MenuItem m_menu_item_file_clear_files;
        Gtk::MenuItem m_menu_item_file_settings;
        Gtk::MenuItem m_menu_item_file_app_quit;
        Gtk::MenuItem m_menu_item_visuals;
        Gtk::Paned m_paned;
        Gtk::ListBox m_media_list_box;
        Gtk::Box m_media_controls_box;

        Glib::RefPtr<Gst::Element> m_pipeline;
        Gst::Element* m_source;
        Gst::Element* m_audio_link;
        bool m_is_playing = false;
        bool m_slider_dragging = false;
        std::string m_current_media_file;

        Gtk::Button* m_play_button;
        Gtk::Button* m_pause_button;
        Gtk::Button* m_stop_button;
        Gtk::Scale* m_time_slider;
        Gtk::Scale* m_volume_slider;

        void setup_menu();
        void setup_media_controls();
        void on_add_files_folders();
        void on_clear_files();
        void on_settings();
        void on_app_quit();
        void on_visuals();
        void on_window_resize(Gtk::Allocation& allocation);

        // New methods
        void load_media_directories();
        void save_media_directories();
        void update_media_list();

        void on_play_button_clicked();
        void on_pause_button_clicked();
        void on_stop_button_clicked();
        bool update_time_slider();
        void on_time_slider_changed();
        void on_time_slider_value_changed();
        void on_time_slider_release();
        void on_volume_slider_changed();
        void on_label_click(const std::string& media_file);

        // New members
        std::vector<std::string> m_media_directories;  // List of media directories

        // Constants
        const std::string media_file = "../media/directories.txt";

};

#endif //WINDOW_H
