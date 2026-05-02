#include <catch2/catch_all.hpp>

#include <nlohmann/json.hpp>

#include "slic3r/Utils/PrinterMonitorSnapshot.hpp"

using namespace Slic3r;

TEST_CASE("PrinterMonitorSnapshot parses Moonraker push_status payload", "[PrinterMonitorSnapshot]")
{
    nlohmann::json payload = {
        {"print",
         {
             {"command", "push_status"},
             {"nozzle_temper", 215.5},
             {"nozzle_target_temper", 220},
             {"bed_temper", "60"},
             {"bed_target_temper", 60},
             {"mc_percent", 42},
             {"mc_remaining_time", 12},
             {"gcode_state", "RUNNING"},
             {"subtask_name", "test.gcode"},
         }},
    };

    const auto snapshot = PrinterMonitorSnapshot::from_bambu_push_status(payload);
    REQUIRE(snapshot.nozzle_temp.has_value());
    REQUIRE(snapshot.nozzle_temp.value() == Approx(215.5f));
    REQUIRE(snapshot.nozzle_target.has_value());
    REQUIRE(snapshot.nozzle_target.value() == Approx(220.0f));
    REQUIRE(snapshot.bed_temp.has_value());
    REQUIRE(snapshot.bed_temp.value() == Approx(60.0f));
    REQUIRE(snapshot.progress_percent == 42);
    REQUIRE(snapshot.remaining_minutes == 12);
    REQUIRE(snapshot.state == "RUNNING");
    REQUIRE(snapshot.file == "test.gcode");
}

TEST_CASE("PrinterMonitorSnapshot parses Moonraker printer.objects.query result", "[PrinterMonitorSnapshot]")
{
    nlohmann::json result = {
        {"status",
         {
             {"extruder", {{"temperature", 205.0}, {"target", 210.0}}},
             {"heater_bed", {{"temperature", 55.5}, {"target", 60.0}}},
             {"virtual_sdcard", {{"progress", 0.33}}},
             {"print_stats", {{"state", "printing"}, {"filename", "cube.gcode"}, {"total_duration", 600}, {"print_duration", 120}}},
         }},
    };

    const auto snapshot = PrinterMonitorSnapshot::from_moonraker_status(result);
    REQUIRE(snapshot.nozzle_temp.has_value());
    REQUIRE(snapshot.nozzle_temp.value() == Approx(205.0f));
    REQUIRE(snapshot.nozzle_target.has_value());
    REQUIRE(snapshot.nozzle_target.value() == Approx(210.0f));
    REQUIRE(snapshot.bed_temp.has_value());
    REQUIRE(snapshot.bed_temp.value() == Approx(55.5f));
    REQUIRE(snapshot.bed_target.has_value());
    REQUIRE(snapshot.bed_target.value() == Approx(60.0f));
    REQUIRE(snapshot.progress_percent.has_value());
    REQUIRE(snapshot.progress_percent.value() == 33);
    REQUIRE(snapshot.remaining_minutes.has_value());
    REQUIRE(snapshot.remaining_minutes.value() == 8);
    REQUIRE(snapshot.state == "RUNNING");
    REQUIRE(snapshot.file == "cube.gcode");
}

TEST_CASE("PrinterMonitorSnapshot parses OctoPrint job payload", "[PrinterMonitorSnapshot]")
{
    nlohmann::json job = {
        {"state", "Printing"},
        {"progress", {{"completion", 12.6}, {"printTimeLeft", 125}}},
        {"job", {{"file", {{"name", "benchy.gcode"}}}}},
        {"temps", {{"tool0", {{"actual", 200}, {"target", 205}}}, {"bed", {{"actual", 50.0}, {"target", 55.0}}}}},
    };

    const auto snapshot = PrinterMonitorSnapshot::from_octoprint_job(job);
    REQUIRE(snapshot.state == "Printing");
    REQUIRE(snapshot.file == "benchy.gcode");
    REQUIRE(snapshot.progress_percent.has_value());
    REQUIRE(snapshot.progress_percent.value() == 13);
    REQUIRE(snapshot.remaining_minutes.has_value());
    REQUIRE(snapshot.remaining_minutes.value() == 2);
    REQUIRE(snapshot.nozzle_temp.has_value());
    REQUIRE(snapshot.nozzle_temp.value() == Approx(200.0f));
    REQUIRE(snapshot.nozzle_target.has_value());
    REQUIRE(snapshot.nozzle_target.value() == Approx(205.0f));
    REQUIRE(snapshot.bed_temp.has_value());
    REQUIRE(snapshot.bed_temp.value() == Approx(50.0f));
    REQUIRE(snapshot.bed_target.has_value());
    REQUIRE(snapshot.bed_target.value() == Approx(55.0f));
}
