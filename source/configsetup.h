#pragma once

#include "config/SettingConfig.h"

namespace emp {

SettingConfig setup() {
    SettingConfig config;
    // Testing purposes
    config.AddSetting<double>("r_value") = {0.02};
    config.AddSetting<double>("u_value") = {0.175};
    config.AddSetting<size_t>("N_value") = {6400};
    config.AddSetting<size_t>("E_value") = {5000};

    return config;
}

}  // namespace emp