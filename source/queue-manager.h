#pragma once

#include <functional>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/vector.h"
#include "config/SettingConfig.h"
#include "simplepdworld.h"
#include "tools/Random.h"
#include "tools/math.h"
#include "web/Div.h"
#include "web/web.h"

namespace emp {

struct RunInfo {
    SettingConfig runinfo_config;

    size_t id;

    size_t cur_epoch;
    size_t num_coop;
    size_t num_defect;

    RunInfo(SettingConfig _config, size_t _id)
        : runinfo_config(_config), id(_id), cur_epoch(0), num_coop(0), num_defect(0) { ; }
};

class QueueManager {
   private:
    SettingConfig queue_config;
    std::queue<RunInfo> runs;
    emp::web::Div display_div;
    std::string table_id;
    // ordered names of dependant headers with associated column #'s
    std::vector<std::pair<std::string, int>> ordered_names;
    std::unordered_map<std::string, std::function<std::string()>> dependant_headers;
    // for SimplePDWorld
    size_t epoch_ = 0;
    size_t coop_ = 0;

   public:
    void SetEpoch(size_t epoch) { epoch_ = epoch; }
    void SetNumCoop(size_t coop) { coop_ = coop; }

    /// Default constructor
    QueueManager() = default;

    /// Config constructor
    QueueManager(SettingConfig user_config) : queue_config(user_config) { ; }

    /// Checks if queue is empty
    bool IsEmpty() {
        return runs.empty();
    }

    /// Checks how runs are in the queue
    size_t RunsRemaining() {
        return runs.size();
    }

    /// Adds run to queue with run info for paramters
    void AddRun(SettingConfig other) {
        RunInfo new_run(other, runs.size());
        runs.push(new_run);
    }

    /// Remove run from front of queue
    void RemoveRun() {
        emp_assert(!IsEmpty(), "Queue is empty! Cannot remove!");
        runs.pop();
    }

    /// Front Run Getter
    RunInfo& FrontRun() {
        emp_assert(!IsEmpty(), "Queue is empty! Cannot access Front!");
        return runs.front();
    }

    /// Returns this dic
    emp::web::Div GetDiv() {
        return display_div;
    }
    /// Clears the content of this div
    void ResetDiv() {
        display_div.Clear();
    }

    /// Initializes table to web
    void DivAddTable(size_t row, size_t col, std::string id) {
        table_id = id;
        emp::web::Table result_tab(row, col, id);
        result_tab.SetCSS("border-collapse", "collapse");
        result_tab.SetCSS("border", "3px solid black");
        result_tab.CellsCSS("border", "1px solid black");

        result_tab.GetCell(0, 0).SetHeader() << "Run";
        int column_count = 1;
        std::vector<std::string> setting_names = queue_config.GetSettingMapNames();
        for (const auto& p : setting_names) {
            result_tab.GetCell(0, column_count).SetHeader() << "<i>" << p << "</i>";
            ++column_count;
        }

        /* if adding more features after this point, keep in mind of where
        the col count will be */
        for (auto& i : ordered_names) {
            i.second = column_count;
            result_tab.GetCell(0, column_count++).SetHeader() << i.first;
        }

        display_div << result_tab;
    }

    /// Extends table once button is clicked
    void DivButtonTable(int run_id) {
        emp::web::Table my_table = display_div.Find(table_id);

        // Have a way to iterate through the SettingConfig without knowing the Setting types

        // Update the table.
        int line_id = my_table.GetNumRows();
        my_table.Rows(line_id + 1);
        int col_count = 0;
        my_table.GetCell(line_id, col_count++) << run_id;
        for (const auto& p : queue_config.GetSettingMapNames()) {
            my_table.GetCell(line_id, col_count++) << queue_config.GetValue<size_t>(p);
        }
        //my_table.GetCell(line_id, 1) << queue_config.GetValue<size_t>("E_value");
        //my_table.GetCell(line_id, 2) << queue_config.GetValue<size_t>("N_value");
        //my_table.GetCell(line_id, 3) << queue_config.GetValue<double>("r_value");
        //my_table.GetCell(line_id, 4) << queue_config.GetValue<double>("u_value");

        for (int i = 0; dependant_headers.size() <= i; i++) {
            my_table.GetCell(line_id, col_count++) << "Waiting...";  // world.GetE();
        }
        //my_table.GetCell(line_id, 5) << "Waiting...";  // world.GetE();
        //my_table.GetCell(line_id, 6) << "Waiting...";  // world.CountCoop();
        //my_table.GetCell(line_id, 7) << "Waiting...";  // (world.GetN() - world.CountCoop());

        // Draw the new table.
        my_table.CellsCSS("border", "1px solid black");
        my_table.Redraw();
    }

    /// Run info in table is updated
    void DivInfoTable(size_t id, size_t cur_epoch, size_t num_coop, size_t num_defect) {
        emp::web::Table my_table = display_div.Find(table_id);
        my_table.Freeze();
        my_table.GetCell(id + 1, 5).ClearChildren() << cur_epoch;
        my_table.GetCell(id + 1, 6).ClearChildren() << num_coop;
        my_table.GetCell(id + 1, 7).ClearChildren() << num_defect;
        my_table.Activate();
    }

    /// Calculations required for updating table
    void DivTableCalc() {
        size_t id = FrontRun().id;
        size_t cur_epoch = epoch_;
        RunInfo& current_run = FrontRun();
        if (current_run.runinfo_config.GetValue<size_t>("E_value") <= cur_epoch) {  // Are we done with this run?
            RemoveRun();                                                            // Updates to the next run
        }
        current_run.cur_epoch = cur_epoch;
        current_run.num_coop = coop_;
        current_run.num_defect = current_run.runinfo_config.GetValue<size_t>("N_value") - current_run.num_coop;

        DivInfoTable(id, cur_epoch, current_run.num_coop, current_run.num_defect);
    }

    /// Creates area for user to input how many runs will be queued
    void DivAddTextArea(SimplePDWorld& world) {
        emp::web::TextArea run_input([&world](const std::string& str) {
            size_t num_runs = emp::from_string<size_t>(str);
            world.SetNumRuns(num_runs);
        },
                                     "run_count");

        run_input.SetText(emp::to_string(world.GetNumRuns()));
        display_div << run_input;
    }

    /// Creates queue button
    void DivButton(size_t num_runs) {
        emp::web::Button my_button([this, num_runs]() {
            for (int run_id = 0; run_id < num_runs; run_id++) {
                AddRun(queue_config);
                DivButtonTable(run_id);
            }
        },
                                   "Queue", "queue_but");
        display_div << my_button;
    }

    /// Adds dependant variables to tables
    void AddDepVariable(std::function<std::string()> func, std::string header_name) {
        ordered_names.push_back({header_name, 0});
        dependant_headers.insert({header_name, func});
    }
};

}  // namespace emp
