#include "PrinterMonitorSnapshot.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>

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

PrinterMonitorSnapshot PrinterMonitorSnapshot::from_moonraker_status(const nlohmann::json& result)
{
    PrinterMonitorSnapshot out;
    if (!result.is_object())
        return out;

    if (result.contains("status") && result.at("status").is_object()) {
        const auto& status = result.at("status");

        const nlohmann::json* extruder = nullptr;
        if (status.contains("extruder") && status.at("extruder").is_object())
            extruder = &status.at("extruder");
        else {
            for (const auto& item : status.items()) {
                if (item.value().is_object() && item.key().rfind("extruder", 0) == 0) {
                    extruder = &item.value();
                    break;
                }
            }
        }

        if (extruder) {
            out.nozzle_temp   = get_optional_float(*extruder, "temperature");
            out.nozzle_target = get_optional_float(*extruder, "target");
        }

        if (status.contains("heater_bed") && status.at("heater_bed").is_object()) {
            const auto& bed = status.at("heater_bed");
            out.bed_temp     = get_optional_float(bed, "temperature");
            out.bed_target   = get_optional_float(bed, "target");
        }

        if (status.contains("virtual_sdcard") && status.at("virtual_sdcard").is_object()) {
            const auto& vsd = status.at("virtual_sdcard");
            if (vsd.contains("progress") && (vsd.at("progress").is_number_float() || vsd.at("progress").is_number_integer())) {
                const double progress = vsd.at("progress").get<double>();
                if (progress >= 0.0)
                    out.progress_percent = std::clamp(static_cast<int>(progress * 100.0 + 0.5), 0, 100);
            }
        }

        if (status.contains("print_stats") && status.at("print_stats").is_object()) {
            const auto& ps = status.at("print_stats");
            if (ps.contains("state") && ps.at("state").is_string()) {
                std::string s = ps.at("state").get<std::string>();
                for (auto& c : s)
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                if (s == "printing")
                    out.state = "RUNNING";
                else if (s == "paused")
                    out.state = "PAUSE";
                else if (s == "complete")
                    out.state = "FINISH";
                else if (s == "error" || s == "cancelled")
                    out.state = "FAILED";
                else
                    out.state = "IDLE";
            }
            if (ps.contains("filename") && ps.at("filename").is_string())
                out.file = ps.at("filename").get<std::string>();

            const auto total   = get_optional_float(ps, "total_duration");
            const auto elapsed = get_optional_float(ps, "print_duration");
            if (total && elapsed && *total > 0.0f && *elapsed >= 0.0f) {
                out.remaining_minutes = std::max(0, static_cast<int>((*total - *elapsed) / 60.0f));
            }
        }
    }

    return out;
}

PrinterMonitorSnapshot PrinterMonitorSnapshot::from_octoprint_job(const nlohmann::json& job)
{
    PrinterMonitorSnapshot out;
    if (!job.is_object())
        return out;

    if (job.contains("state") && job.at("state").is_string())
        out.state = job.at("state").get<std::string>();

    if (job.contains("progress") && job.at("progress").is_object()) {
        const auto& progress = job.at("progress");
        if (progress.contains("completion") && (progress.at("completion").is_number_float() || progress.at("completion").is_number_integer())) {
            out.progress_percent = std::clamp(static_cast<int>(progress.at("completion").get<double>() + 0.5), 0, 100);
        }
        if (progress.contains("printTimeLeft") && !progress.at("printTimeLeft").is_null()) {
            const auto secs = get_optional_int(progress, "printTimeLeft");
            if (secs)
                out.remaining_minutes = std::max(0, (*secs + 30) / 60);
        }
    }

    if (job.contains("job") && job.at("job").is_object()) {
        const auto& jobinfo = job.at("job");
        if (jobinfo.contains("file") && jobinfo.at("file").is_object()) {
            const auto& file = jobinfo.at("file");
            if (file.contains("name") && file.at("name").is_string())
                out.file = file.at("name").get<std::string>();
        }
    }

    if (job.contains("temps") && job.at("temps").is_object()) {
        const auto& temps = job.at("temps");
        if (temps.contains("tool0") && temps.at("tool0").is_object()) {
            const auto& t0 = temps.at("tool0");
            out.nozzle_temp   = get_optional_float(t0, "actual");
            out.nozzle_target = get_optional_float(t0, "target");
        }
        if (temps.contains("bed") && temps.at("bed").is_object()) {
            const auto& bed = temps.at("bed");
            out.bed_temp    = get_optional_float(bed, "actual");
            out.bed_target  = get_optional_float(bed, "target");
        }
    }

    return out;
}

}
