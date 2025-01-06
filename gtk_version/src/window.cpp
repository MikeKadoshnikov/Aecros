#include "window.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <gstreamermm.h>
#include <glibmm.h>

namespace fs = std::filesystem;

Window::Window() :
	m_box(Gtk::ORIENTATION_VERTICAL),
	m_menu_item_file("_File", true),
	m_menu_item_file_add_file_folder("_Add File/Folder", true),
	m_menu_item_file_clear_files("_Clear Files", true),
	m_menu_item_file_settings("_Settings", true),
	m_menu_item_file_app_quit("_Quit", true),
	m_menu_item_visuals("_Visuals", true),
	m_media_controls_box(Gtk::ORIENTATION_HORIZONTAL),
	m_pipeline(nullptr),
	m_play_button(nullptr),
	m_pause_button(nullptr),
    m_stop_button(nullptr),
    m_time_slider(nullptr),
    m_volume_slider(nullptr)
{
	Gst::init();

    set_title("Aecros");
    set_resizable(true);
	set_size_request(800, 600);
    set_default_size(800, 600);

	setup_menu();

	m_media_list_box.set_vexpand(true);
	setup_media_controls();

	m_paned.set_orientation(Gtk::ORIENTATION_VERTICAL);
	m_paned.add1(m_media_list_box);
    m_paned.add2(m_media_controls_box);

	m_box.pack_start(m_menu_bar, Gtk::PACK_SHRINK);
	m_box.pack_start(m_paned);

	add(m_box);

	signal_size_allocate().connect(sigc::mem_fun(*this, &Window::on_window_resize));

	load_media_directories();
	show_all_children();

	Glib::signal_timeout().connect(sigc::mem_fun(*this, &Window::update_time_slider), 100);

}

Window::~Window() {
	std::cout << "Destroying window" << std::endl;
	save_media_directories();

	if (m_pipeline) {
		m_pipeline->set_state(Gst::STATE_NULL);
	}
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
}

void Window::setup_media_controls() {
    Gtk::Button* play_button = Gtk::manage(new Gtk::Button("Play"));
    Gtk::Button* pause_button = Gtk::manage(new Gtk::Button("Pause"));
    Gtk::Button* stop_button = Gtk::manage(new Gtk::Button("Stop"));

    m_volume_slider = Gtk::manage(new Gtk::Scale(Gtk::ORIENTATION_HORIZONTAL));
    m_volume_slider->set_range(0, 100);
    m_volume_slider->set_value(20);

    m_time_slider = Gtk::manage(new Gtk::Scale(Gtk::ORIENTATION_HORIZONTAL));
    m_time_slider->set_range(0, 1000);
    m_time_slider->set_value(0);

    Gtk::Box* slider_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));

    slider_box->pack_start(*m_volume_slider, Gtk::PACK_SHRINK);
    slider_box->pack_start(*m_time_slider, Gtk::PACK_EXPAND_WIDGET);

    m_media_controls_box.pack_start(*play_button, Gtk::PACK_SHRINK);
    m_media_controls_box.pack_start(*pause_button, Gtk::PACK_SHRINK);
    m_media_controls_box.pack_start(*stop_button, Gtk::PACK_SHRINK);
    m_media_controls_box.pack_start(*slider_box, Gtk::PACK_EXPAND_WIDGET);

    m_media_controls_box.set_border_width(10);

    play_button->signal_clicked().connect(sigc::mem_fun(*this, &Window::on_play_button_clicked));
    pause_button->signal_clicked().connect(sigc::mem_fun(*this, &Window::on_pause_button_clicked));
    stop_button->signal_clicked().connect(sigc::mem_fun(*this, &Window::on_stop_button_clicked));
    m_time_slider->signal_value_changed().connect(sigc::mem_fun(*this, &Window::on_time_slider_changed));
    m_volume_slider->signal_value_changed().connect(sigc::mem_fun(*this, &Window::on_volume_slider_changed), false);

}

void Window::on_play_button_clicked() {
    if (!m_current_media_file.empty() && m_pipeline) {
        std::cout << "Playing " << m_current_media_file << std::endl;

        // Only set the state to PLAYING if it's not already playing
        if (!m_is_playing) {
            m_pipeline->set_state(Gst::STATE_PLAYING);
            m_is_playing = true;

            // Ensure we have the duration of the media file
            gint64 duration;
            if (m_pipeline->query_duration(Gst::FORMAT_TIME, duration)) {
                m_time_slider->set_range(0, duration / 1000000);  // Set the slider range in seconds
                m_time_slider->set_value(0);  // Reset slider to start
            }
        }
    }
}

void Window::on_volume_slider_changed() {
    if (m_pipeline) {
        double volume = m_volume_slider->get_value() / 100.0; // Scale volume from 0-100 to 0-1
        m_pipeline->set_property("volume", volume); // Set the volume property of the pipeline
    }
}


void Window::on_pause_button_clicked() {
	std::cout << "Paused" << std::endl;
    if (m_pipeline) {
        m_pipeline->set_state(Gst::STATE_PAUSED);
        m_is_playing = false;
    }
}

void Window::on_stop_button_clicked() {
	std::cout << "Stopping" << std::endl;
    if (m_pipeline) {
        m_pipeline->set_state(Gst::STATE_NULL);
        m_is_playing = false;
    }
}

bool Window::update_time_slider() {
    if (m_is_playing && m_pipeline) {
        gint64 position;
        if (m_pipeline->query_position(Gst::FORMAT_TIME, position)) {
            gint64 current_time = position / 1000000000;  // Convert position from nanoseconds to seconds

            gint64 max_time = m_time_slider->get_adjustment()->get_upper();  // Get slider's maximum value
            if (current_time < max_time) {
                m_time_slider->set_value(current_time);  // Update slider
            }
        }
    }
    return true;  // Keep updating every 100ms
}

void Window::on_time_slider_changed() {
/*
    if (m_pipeline) {
        gint64 new_position = m_time_slider->get_value() * 1000000000;  // Convert seconds to nanoseconds

        // Seek to the new position, but avoid restarting the pipeline.
        if (!m_pipeline->seek(1.0, Gst::FORMAT_TIME,
                             Gst::SEEK_FLAG_ACCURATE,
                             Gst::SEEK_TYPE_SET, new_position,
                             Gst::SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
            std::cerr << "Seek failed!" << std::endl;
        }
    } */
	std::cout << "Slider changed is this the issue" << std::endl;
}

void Window::on_add_files_folders() {
	std::cout << "Adding folders" << std::endl;
    Gtk::FileChooserDialog dialog(*this, "Select Files or Folders", Gtk::FILE_CHOOSER_ACTION_OPEN);
	dialog.set_select_multiple(true);

	dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	dialog.add_button("_Open", Gtk::RESPONSE_OK);

	if (dialog.run() == Gtk::RESPONSE_OK) {
		std::vector<std::string> filenames = dialog.get_filenames();
		for (const auto& filename : filenames) {
			if(fs::exists(filename)) {
				m_media_directories.push_back(filename);
			}
		}

		update_media_list();
	}
}

void Window::on_clear_files() {
	std::cout << "Clearing files" << std::endl;
	m_media_directories.clear();
	update_media_list();
}

void Window::on_settings() {
	std::cout << "Settings..." << std::endl;
}

void Window::on_app_quit() {
	std::cout << "Quitting..." << std::endl;
	save_media_directories();
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

void Window::load_media_directories() {
	std::ifstream file(media_file);

	if(!file) {
		std::ofstream new_file(media_file);
		if(!new_file) {
			std::cerr << "Unable to open " << media_file << std::endl;
		} else {
			std::cout << media_file << " created successfully" << std::endl;
		}
		return;
	}

	std::string line;
	while (std::getline(file, line)) {
		if (fs::exists(line)) {
			m_media_directories.push_back(line);
		}
	}
	update_media_list();
}

void Window::save_media_directories() {
	std::ofstream file(media_file);
	if(!file) {
		std::cerr << "Unable to write to " << media_file << std::endl;
        return;
	}

	for (const auto& dir : m_media_directories) {
		file << dir << std::endl;
	}
}

void Window::update_media_list() {
	std::vector<Gtk::Widget*> children = m_media_list_box.get_children();
	for (auto* widget : children) {
		m_media_list_box.remove(*widget);
	}

	for (const auto& dir : m_media_directories) {
		Gtk::Button* button = Gtk::manage(new Gtk::Button(dir));
		button->signal_clicked().connect(sigc::bind(&Window::on_label_click, this, dir));
		m_media_list_box.append(*button);
	}

	m_media_list_box.show_all_children();
}

void Window::on_label_click(const std::string& media_file) {
    std::cout << "Playing file: " << media_file << std::endl;
    m_current_media_file = media_file;

    // Ensure pipeline is only initialized once
    if (m_pipeline) {
        m_pipeline->set_state(Gst::STATE_NULL);  // Stop any current media
        m_pipeline.clear();
    }

    try {
        // Create the pipeline only once
        m_pipeline = Gst::ElementFactory::create_element("playbin");
        if (m_pipeline) {
            // Set the file URI to the pipeline
            m_pipeline->set_property("uri", "file://" + m_current_media_file);
            m_pipeline->set_state(Gst::STATE_PLAYING);
            m_is_playing = true;

            // Query the duration of the media
            Gst::Pad* pad = m_pipeline->get_static_pad("video_sink").get();
            if (pad) {
                gint64 duration;
                if (pad->query_duration(Gst::FORMAT_TIME, duration)) {
                    gint64 duration_seconds = duration / 1000000000; // Convert nanoseconds to seconds
                    std::cout << "Duration: " << duration_seconds << " seconds" << std::endl;

                    // Set the slider's range based on the media duration
                    m_time_slider->set_range(0, duration_seconds);  // Range in seconds
                    m_time_slider->set_value(0);  // Set initial value to 0
                }
            }
        }
    } catch (const Glib::Exception& e) {
        std::cerr << "Error creating pipeline: " << e.what() << std::endl;
    }
}