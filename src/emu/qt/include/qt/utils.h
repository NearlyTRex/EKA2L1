#pragma once

#include <services/ui/cap/oom_app.h>
#include <optional>

namespace eka2l1 {
    class window_server;
    class system;

    namespace config {
        struct app_setting;
        class app_settings;
    }

    namespace epoc {
        struct screen;
        struct window_group;
    }
}

static constexpr const char *RECENT_MOUNT_SETTINGS_NAME = "recentMountFolders";
static constexpr const char *SHOW_SCREEN_NUMBER_SETTINGS_NAME = "showScreenNumber";

eka2l1::window_server *get_window_server_through_system(eka2l1::system *sys);
eka2l1::epoc::screen *get_current_active_screen(eka2l1::system *sys, const int provided_num = -1);
eka2l1::config::app_setting *get_active_app_setting(eka2l1::system *sys, eka2l1::config::app_settings &settings, const int provided_num = -1);

std::optional<eka2l1::akn_running_app_info> get_active_app_info(eka2l1::system *sys, const int provided_num = -1);
