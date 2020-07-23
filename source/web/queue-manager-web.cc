//  This file is part of Project Name
//  Copyright (C) Michigan State University, 2017.
//  Released under the MIT Software license; see doc/LICENSE
// PD WORLD EXAMPLE

#include <functional>
#include <string>

#include "../configsetup.h"
#include "../queue-manager.h"
#include "../simplepdworld.h"
#include "web/web.h"

namespace UI = emp::web;

const double world_size = 600;

UI::Document doc("emp_base");
// Config created
emp::SettingConfig config = emp::setup();
emp::QueueManager run_list(config);
emp::SimplePDWorld world;

int cur_x = -1;
int cur_y = -1;

void DrawCanvas() {
    UI::Canvas canvas = doc.Canvas("canvas");
    canvas.Clear("black");

    const emp::vector<emp::Org>& pop = world.GetPop();

    if (cur_x >= 0) {
        canvas.Circle(cur_x, cur_y, world_size * world.GetR(), "pink");
    }

    for (const emp::Org& org : pop) {
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
    int x = 4;
    std::function<std::string()> my_func = [x]() { return std::to_string(x * x); };
    run_list.AddDepVariable(my_func, "Epoch");
    run_list.AddDepVariable(my_func, "Num Coop");
    run_list.AddDepVariable(my_func, "Num Defect");
    doc << "<h2>Spatial Prisoner's Dilema</h2>";
    auto canvas = doc.AddCanvas(world_size, world_size, "canvas");
    // canvas.On("click", CanvasClick);
    auto& anim = doc.AddAnimation("anim_world", []() {
        // if queue has runs
        if (!run_list.IsEmpty()) {
            emp::RunInfo run = run_list.FrontRun();  // Referencing current run
            if (run.cur_epoch == 0) {                // Are we starting a new run?
                world.Setup(run.runinfo_config.GetValue<double>("r_value"), run.runinfo_config.GetValue<double>("u_value"), run.runinfo_config.GetValue<size_t>("N_value"), run.runinfo_config.GetValue<size_t>("E_value"));

                DrawCanvas();
            }
        }
        world.Run(anim_step);
        DrawCanvas();
        if (!run_list.IsEmpty()) {
            run_list.SetEpoch(world.GetEpoch());
            run_list.SetNumCoop(world.CountCoop());
            run_list.DivTableCalc();  //calculations for table
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

    size_t num_runs = run_list.DivAddTextArea();
    world.SetNumRuns(num_runs);

    size_t queue_runs = world.GetNumRuns();
    run_list.DivButton(queue_runs);

    doc << "<br>";

    doc << run_list.GetDiv();
    run_list.DivAddTable(1, 8, "result_tab");

    DrawCanvas();
}