#pragma once

#include <optional>
#include <string>

#include <nlohmann/json_fwd.hpp>

namespace Slic3r {

struct PrinterMonitorSnapshot
{
    std::optional<float> nozzle_temp;
    std::optional<float> nozzle_target;
    std::optional<float> bed_temp;
    std::optional<float> bed_target;

    std::optional<int> progress_percent;
    std::optional<int> remaining_minutes;

    std::string state;
    std::string file;

    static PrinterMonitorSnapshot from_bambu_push_status(const nlohmann::json& payload);
};

}
