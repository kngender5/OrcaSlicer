#include "PrinterMonitorSnapshot.hpp"

#include <nlohmann/json.hpp>

namespace Slic3r {
namespace {

std::optional<float> get_optional_float(const nlohmann::json& obj, const char* key)
{
    if (!obj.contains(key) || obj.at(key).is_null())
        return std::nullopt;
    const auto& v = obj.at(key);
    if (v.is_number_float() || v.is_number_integer())
        return v.get<float>();
    if (v.is_string()) {
        try {
            return std::stof(v.get<std::string>());
        } catch (...) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<int> get_optional_int(const nlohmann::json& obj, const char* key)
{
    if (!obj.contains(key) || obj.at(key).is_null())
        return std::nullopt;
    const auto& v = obj.at(key);
    if (v.is_number_integer())
        return v.get<int>();
    if (v.is_number_float())
        return static_cast<int>(v.get<double>() + 0.5);
    if (v.is_string()) {
        try {
            return std::stoi(v.get<std::string>());
        } catch (...) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::string get_optional_string(const nlohmann::json& obj, const char* key)
{
    if (!obj.contains(key) || obj.at(key).is_null())
        return {};
    const auto& v = obj.at(key);
    if (v.is_string())
        return v.get<std::string>();
    return v.dump();
}

} // namespace

PrinterMonitorSnapshot PrinterMonitorSnapshot::from_bambu_push_status(const nlohmann::json& payload)
{
    PrinterMonitorSnapshot out;
    if (!payload.is_object() || !payload.contains("print") || !payload.at("print").is_object())
        return out;

    const auto& p = payload.at("print");
    out.nozzle_temp   = get_optional_float(p, "nozzle_temper");
    out.nozzle_target = get_optional_float(p, "nozzle_target_temper");
    out.bed_temp      = get_optional_float(p, "bed_temper");
    out.bed_target    = get_optional_float(p, "bed_target_temper");

    out.progress_percent  = get_optional_int(p, "mc_percent");
    out.remaining_minutes = get_optional_int(p, "mc_remaining_time");

    out.state = get_optional_string(p, "gcode_state");
    out.file  = get_optional_string(p, "subtask_name");
    if (out.file.empty())
        out.file = get_optional_string(p, "gcode_file");

    return out;
}

}
