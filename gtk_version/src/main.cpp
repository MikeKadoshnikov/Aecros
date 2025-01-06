#include "window.h"
#include <gtkmm-3.0/gtkmm.h>

int main(int argc, char *argv[]) {
    auto app = Gtk::Application::create(argc, argv, "org.gtk.example");
    Window window;
    return app->run(window);
}