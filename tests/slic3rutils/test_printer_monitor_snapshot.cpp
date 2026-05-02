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

