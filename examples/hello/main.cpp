#include <swole/swole.hpp>

using namespace swole;

int main(int argc, char** argv) {
    Application app(argc, argv);

    WindowConfig cfg;
    cfg.title = "swole – hello";
    cfg.size  = {480, 320};
    Window win(cfg);

    // Root layout: vertical box with some padding
    auto vbox = std::make_unique<BoxLayout>(BoxLayout::v());
    vbox->set_margin(16);
    vbox->set_spacing(10);

    auto* heading = win.root().emplace_child<Label>("Hello from swole!");
    heading->set_font(Font{"", 20.f, FontWeight::Bold});

    auto* sub = win.root().emplace_child<Label>("SDL3 + Skia — modern C++ UI framework");
    sub->set_color(Color{80, 80, 80});

    // Horizontal row: text input + button
    auto* row = win.root().emplace_child<Widget>();
    {
        auto hbox = std::make_unique<BoxLayout>(BoxLayout::h());
        hbox->set_margin(0);
        hbox->set_spacing(8);

        auto* edit = row->emplace_child<TextEdit>();
        edit->set_placeholder("Type something…");
        edit->set_focus_policy(FocusPolicy::Click);

        auto* btn = row->emplace_child<Button>("Greet");
        btn->set_focus_policy(FocusPolicy::Click);

        (void)        (void)btn->on_clicked.connect([edit, sub]() {
            std::string name = edit->text();
            if (name.empty()) name = "world";
            sub->set_text("Hello, " + name + "!");
        });

        hbox->add_widget(edit, 1);
        hbox->add_widget(btn,  0);
        row->set_layout(std::move(hbox));
    }

    auto* quit_btn = win.root().emplace_child<Button>("Quit");
    quit_btn->set_focus_policy(FocusPolicy::Click);
    (void)    (void)quit_btn->on_clicked.connect([] { Application::instance().quit(); });

    vbox->add_widget(heading);
    vbox->add_widget(sub);
    vbox->add_stretch();
    vbox->add_widget(row);
    vbox->add_widget(quit_btn);
    win.set_layout(std::move(vbox));

    win.center_on_screen();
    win.show();

    return app.run();
}
