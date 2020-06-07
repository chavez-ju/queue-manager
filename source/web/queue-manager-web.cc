//  This file is part of Project Name
//  Copyright (C) Michigan State University, 2017.
//  Released under the MIT Software license; see doc/LICENSE
// PD WORLD EXAMPLE

#include "../queue-manager.h"
#include "web/web.h"

namespace UI = emp::web;

const double world_size = 600;

UI::Document doc("emp_base");
SimplePDWorld world;
QueueManager run_list;

int cur_x = -1;
int cur_y = -1;

void DrawCanvas() {
    UI::Canvas canvas = doc.Canvas("canvas");
    canvas.Clear("black");

    const emp::vector<Org>& pop = world.GetPop();

    if (cur_x >= 0) {
        canvas.Circle(cur_x, cur_y, world_size * world.GetR(), "pink");
    }

    for (const Org& org : pop) {
        if (org.coop) {
            canvas.Circle(org.x * world_size, org.y * world_size, 2, "blue", "#8888FF");
        } else {
            canvas.Circle(org.x * world_size, org.y * world_size, 2, "#FF8888", "red");
        }
    }

    doc.Text("ud_text").Redraw();
}

void CanvasClick(int x, int y) {
    cur_x = x;
    cur_y = y;
    DrawCanvas();
}

void TogglePlay() {
    auto& anim = doc.Animate("anim_world");
    anim.ToggleActive();
    auto but = doc.Button("start_but");
    if (anim.GetActive())
        but.SetLabel("Pause");
    else
        but.SetLabel("Start");

    but = doc.Button("run_but");
    if (anim.GetActive())
        but.SetLabel("Stop");
    else
        but.SetLabel("Fast Forward!");
}

int anim_step = 1;

int main() {
    doc << "<h2>Spatial Prisoner's Dilema</h2>";
    auto canvas = doc.AddCanvas(world_size, world_size, "canvas");
    // canvas.On("click", CanvasClick);
    auto& anim = doc.AddAnimation("anim_world", []() {
        // if queue has runs
        if (!run_list.IsEmpty()) {
            // Possibly can get rid of?
            auto& run = run_list.FrontRun();  // Referencing current run
            if (run.cur_epoch == 0) {         // Are we starting a new run?
                world.Setup(run.r, run.u, run.N, run.E);
                DrawCanvas();
            }
        }
        world.Run(anim_step);
        DrawCanvas();
        if (!run_list.IsEmpty()) {
            // Possibly can get rid of?
            size_t id = (run_list.FrontRun()).id;
            size_t cur_epoch = world.GetEpoch();
            RunInfo& current_run = run_list.FrontRun();
            if ((run_list.FrontRun()).E <= cur_epoch) {  // Are we done with this run?
                run_list.RemoveRun();                    // Updates to the next run
            }
            current_run.cur_epoch = cur_epoch;
            current_run.num_coop = world.CountCoop();
            current_run.num_defect = current_run.N - current_run.num_coop;

            auto result_tab = doc.Table("result_tab");
            result_tab.Freeze();
            result_tab.GetCell(id + 1, 5).ClearChildren() << cur_epoch;
            result_tab.GetCell(id + 1, 6).ClearChildren() << current_run.num_coop;
            result_tab.GetCell(id + 1, 7).ClearChildren() << current_run.num_defect;
            result_tab.Activate();
        }
    });

    doc << "<br>";
    doc.AddButton([&anim]() {
        anim_step = 1;
        TogglePlay();
    },
                  "Play", "start_but");
    doc.AddButton([]() { world.Run(1); DrawCanvas(); }, "Step", "step_but");
    doc.AddButton([&anim]() {
        anim_step = 100;
        TogglePlay();
    },
                  "Fast Forward!", "run_but");
    doc.AddButton([]() { world.Reset(); DrawCanvas(); }, "Randomize", "rand_but");
    auto ud_text = doc.AddText("ud_text");
    ud_text << " Epoch = " << UI::Live(world.epoch);

    doc << "<br>Radius (<i>r</i>) = ";
    doc.AddTextArea([](const std::string& str) {
           double r = emp::from_string<double>(str);
           world.SetR(r);
       },
                    "r_set")
        .SetText(emp::to_string(world.GetR()));

    doc << "<br>cost/benefit ratio (<i>u</i>) = ";
    doc.AddTextArea([](const std::string& str) {
           double u = emp::from_string<double>(str);
           world.SetU(u);
       },
                    "u_set")
        .SetText(emp::to_string(world.GetU()));

    doc << "<br>Population Size (<i>N</i>) = ";
    doc.AddTextArea([](const std::string& str) {
           size_t N = emp::from_string<size_t>(str);
           world.SetN(N);
       },
                    "N_set")
        .SetText(emp::to_string(world.GetN()));

    doc << "<br>Num Epochs on Run (<i>E</i>) = ";
    doc.AddTextArea([](const std::string& str) {
           size_t E = emp::from_string<size_t>(str);
           world.SetE(E);
       },
                    "E_set")
        .SetText(emp::to_string(world.GetE()));

    doc << "<br>"
        << "NOTE: You must hit 'Randomize' after changing any parameters for them to take effect."
        << "<hr>"
        << "<h3>Full Runs</h3>"
        << "You can perform many runs at once with the same configuration. "
        << "Setup the configuration above, choose the number of runs, and queue them up (as many as you like, even with different parameters). "
        << "The next time you start (or fast forward) above, it will start working its way through the queued runs. "
        << "<br>"
        << "How many runs? ";

    auto run_input = doc.AddTextArea([](const std::string& str) {
        size_t num_runs = emp::from_string<size_t>(str);
        world.SetNumRuns(num_runs);
    },
                                     "run_count");
    run_input.SetText(emp::to_string(world.GetNumRuns()));

    doc.AddButton([run_input]() {
        //size_t num_runs = emp::from_string<size_t>(run_input.GetText());
        size_t num_runs = world.GetNumRuns();
        auto result_tab = doc.Table("result_tab");
        for (int run_id = 0; run_id < num_runs; run_id++) {
            run_list.AddRun(world.GetR(), world.GetU(), world.GetN(), world.GetE());
            // Update the table.
            int line_id = result_tab.GetNumRows();
            result_tab.Rows(line_id + 1);
            result_tab.GetCell(line_id, 0) << run_id;
            result_tab.GetCell(line_id, 1) << world.GetR();
            result_tab.GetCell(line_id, 2) << world.GetU();
            result_tab.GetCell(line_id, 3) << world.GetN();
            result_tab.GetCell(line_id, 4) << world.GetE();
            result_tab.GetCell(line_id, 5) << "Waiting...";  // world.GetE();
            result_tab.GetCell(line_id, 6) << "Waiting...";  // world.CountCoop();
            result_tab.GetCell(line_id, 7) << "Waiting...";  // (world.GetN() - world.CountCoop());

            // Draw the new table.
            result_tab.CellsCSS("border", "1px solid black");
            result_tab.Redraw();

            run_list.DivAnimTable(world, run_id);
        }
    },
                  "Queue", "queue_but");

    doc << "<br>";

    auto result_tab = doc.AddTable(1, 8, "result_tab");
    result_tab.SetCSS("border-collapse", "collapse");
    result_tab.SetCSS("border", "3px solid black");
    result_tab.CellsCSS("border", "1px solid black");

    result_tab.GetCell(0, 0).SetHeader() << "ID";
    result_tab.GetCell(0, 1).SetHeader() << "<i>r</i>";
    result_tab.GetCell(0, 2).SetHeader() << "<i>u</i>";
    result_tab.GetCell(0, 3).SetHeader() << "<i>N</i>";
    result_tab.GetCell(0, 4).SetHeader() << "<i>E</i>";
    result_tab.GetCell(0, 5).SetHeader() << "Epoch";
    result_tab.GetCell(0, 6).SetHeader() << "Num Coop";
    result_tab.GetCell(0, 7).SetHeader() << "Num Defect";

    DrawCanvas();

    run_list.DivAddTable(1, 8, "test");
    doc << "<br>";
    doc << run_list.GetDiv();
}
