#pragma once

#include <algorithm>
#include <iostream>
#include <queue>
#include <string>

#include "base/vector.h"
#include "config/config.h"
#include "tools/Random.h"
#include "tools/math.h"
#include "web/Div.h"
#include "web/web.h"

EMP_BUILD_CONFIG(MyConfig,
                 GROUP(DEFAULT_GROUP, "General Settings"),
                 VALUE(TEST_R, double, 0.02, "neighborhood radius"),
                 VALUE(TEST_U, double, 0.175, "cost / benefit ratio"),
                 VALUE(TEST_N, size_t, 6400, "population size"),
                 VALUE(TEST_E, size_t, 5000, "How many epochs should a population run for?"), )

namespace emp {

struct Org {
    double x;
    double y;
    bool coop;
    double fitness;

    emp::vector<size_t> neighbors;
};

// Create a class to maintain a simple Prisoner's Dilema world.
class SimplePDWorld {
   public:
    // Parameters
    double r;         // Neighborhood radius
    double u;         // cost / benefit ratio
    size_t N;         // Population size
    size_t E;         // How many epochs should a popuilation run for?
    size_t num_runs;  // How many runs should we do?
    bool use_ave;     // Use the average payoff for fitness instead if the total.

    emp::Random random;  // All-purpose random-number generator
    size_t epoch;        // What epoch are we currently on?

    // Calculations we'll need later.
    double r_sqr;  // r squared (for comparisons)
    emp::vector<Org> pop;

    // Prisoner's Dilema payout table...
    double payoff_CC;
    double payoff_CD;
    double payoff_DC;
    double payoff_DD;

    // Helper functions
    void CalcFitness(size_t id);
    void Repro();

   public:
    SimplePDWorld(double _r = 0.02, double _u = 0.175, size_t _N = 6400, size_t _E = 5000, bool _ave = false, int seed = 0)
        : num_runs(10), random(seed) {
        Setup(_r, _u, _N, _E, _ave);  // Call Setup since we a starting a new population.
    }

    const emp::vector<Org>& GetPop() const { return pop; }
    double GetR() const { return r; }
    double GetU() const { return u; }
    size_t GetN() const { return N; }
    size_t GetE() const { return E; }
    size_t GetNumRuns() const { return num_runs; }
    size_t GetEpoch() const { return epoch; }

    void SetR(double _r) { r = _r; }
    void SetU(double _u) { u = _u; }
    void SetN(size_t _N) { N = _N; }
    void SetE(size_t _E) { E = _E; }
    void SetNumRuns(size_t n) { num_runs = n; }

    void UseAve(bool _in = true) { use_ave = _in; }

    void Setup(double _r = 0.02, double _u = 0.0025, size_t _N = 6400, size_t _E = 5000, bool _ave = false) {
        // Store the input values.
        r = _r;
        u = _u;
        N = _N;
        E = _E, use_ave = _ave;
        epoch = 0;

        // Calculations we'll need later.
        r_sqr = r * r;  // r squared (for comparisons)
        pop.resize(N);

        // Setup the payout matric.
        payoff_CC = 1.0;
        payoff_CD = 0.0;
        payoff_DC = 1.0 + u;
        payoff_DD = u;

        // Initialize each organism
        for (Org& org : pop) {
            org.x = random.GetDouble(1.0);
            org.y = random.GetDouble(1.0);
            org.coop = random.P(0.5);
            org.neighbors.resize(0);
        }

        // Determine if pairs of organisms are neighbors;
        // @CAO: Can speed up by using Surface2D.
        for (size_t i = 1; i < N; i++) {
            for (size_t j = 0; j < i; j++) {
                Org& org1 = pop[i];
                Org& org2 = pop[j];
                double x_dist = emp::Abs(org1.x - org2.x);
                if (x_dist > (1.0 - x_dist)) x_dist = 1.0 - x_dist;
                double y_dist = emp::Abs(org1.y - org2.y);
                if (x_dist > (1.0 - x_dist)) x_dist = 1.0 - x_dist;
                double dist_sqr = x_dist * x_dist + y_dist * y_dist;

                // Test if this pair are within neighbor radius...
                if (dist_sqr < r_sqr) {
                    org1.neighbors.push_back(j);
                    org2.neighbors.push_back(i);
                }
            }
        }

        // Calculate the initial fitness for each organism in the population.
        for (size_t id = 0; id < N; id++) {
            CalcFitness(id);
        }
    }

    void Reset() { Setup(r, u, N, E); }

    void Run(size_t steps = -1) {
        if (steps > E) steps = E;
        // Run the organisms!
        size_t end_epoch = epoch + steps;
        while (epoch < end_epoch) {
            for (size_t o = 0; o < N; o++) Repro();
            epoch++;
        }
    }

    size_t CountCoop();
    void PrintNeighborInfo(std::ostream& os);
};

// To calculate the fitness of an organism, have it play
// against all its neighbors and take the average payout.
void SimplePDWorld::CalcFitness(size_t id) {
    Org& org = pop[id];

    int C_count = 0;
    int D_count = 0;
    for (size_t n : org.neighbors) {
        if (pop[n].coop)
            C_count++;
        else
            D_count++;
    }

    double C_value = payoff_CC;
    double D_value = payoff_CD;

    if (!org.coop) {
        C_value = payoff_DC;
        D_value = payoff_DD;
    }

    double total_C = C_value * (double)C_count;
    double total_D = D_value * (double)D_count;
    org.fitness = total_C + total_D;

    if (use_ave) org.fitness /= (double)org.neighbors.size();
}

// Reproduce into a single, random cell.
void SimplePDWorld::Repro() {
    size_t id = random.GetUInt(N);
    Org& org = pop[id];
    bool start_coop = org.coop;

    // Determine the total fitness of neighbors.
    double total_fitness = 0;
    for (size_t n : org.neighbors) {
        total_fitness += pop[n].fitness;
    }

    // If neighbor fitnesses are non-zero, choose one of them.
    if (total_fitness > 0) {
        // Include the focal organism in the pool
        double choice = random.GetDouble(total_fitness + org.fitness);

        // If we aren't keeping the focal organism, we have to pick
        if (choice < total_fitness) {
            for (size_t n : org.neighbors) {
                if (choice < pop[n].fitness) {
                    org.coop = pop[n].coop;  // Copy strategy of winner!
                    break;
                }
                choice -= pop[n].fitness;
            }
        }
    }

    // If we haven't changed our strategy, no need to continue.
    if (org.coop == start_coop) return;

    // Now that we have updated the organism, calculate its fitness again
    // (even if no change, since neighbors may have changed).
    CalcFitness(id);
    // Also update neighbors' fitnesses
    for (size_t n : org.neighbors) {
        CalcFitness(n);
    }
}

// Count how many cooperators we currently have in the population.
size_t SimplePDWorld::CountCoop() {
    size_t count = 0.0;
    for (Org& org : pop)
        if (org.coop) count++;
    return count;
}

// Print out a histogram of neighborhood sizes.
void SimplePDWorld::PrintNeighborInfo(std::ostream& os) {
    size_t total = 0;
    size_t max_size = 0;
    size_t min_size = pop[0].neighbors.size();
    for (Org& org : pop) {
        size_t cur_size = org.neighbors.size();
        total += cur_size;
        if (cur_size > max_size) max_size = cur_size;
        if (cur_size < min_size) min_size = cur_size;
    }
    emp::vector<int> hist(max_size + 1, 0);
    for (Org& org : pop) {
        size_t cur_size = org.neighbors.size();
        hist[cur_size]++;
    }
    // double avg_size = ((double) total) / (double) N;

    os << "neighbors,count\n";
    //    << "average," << avg_size << '\n';
    for (size_t i = 0; i < hist.size(); i++) {
        os << i << ',' << hist[i] << '\n';
    }
    os.flush();
}

struct RunInfo {
    MyConfig config;

    size_t id;

    size_t cur_epoch;
    size_t num_coop;
    size_t num_defect;

    RunInfo(MyConfig _config, size_t _id)
        : config(_config), id(_id), cur_epoch(0), num_coop(0), num_defect(0) { ; }
};

class QueueManager {
   private:
    MyConfig queue_config;

    std::queue<RunInfo> runs;
    emp::web::Div my_div_;
    std::string table_id;

    /// config variables to display to user
    emp::vector<std::string> active_parameters;

   public:
    /// Default constructor
    QueueManager() = default;

    /// Constructor w/ config object
    QueueManager(const MyConfig& _config) : queue_config(_config) {
        for (auto group : queue_config.group_set) {
            for (size_t i = 0; i < group->GetSize(); ++i) {
                active_parameters.push_back(group->GetEntry(i)->GetName());
            }
        }
    }

    /// Checks if queue is empty
    bool IsEmpty() {
        return runs.empty();
    }

    /// Checks how runs are in the queue
    size_t RunsRemaining() {
        return runs.size();
    }

    /// Adds run to queue with run info for parameters
    void AddRun(const MyConfig& config) {
        RunInfo new_run(config, runs.size());
        std::cout << "in AddRun - before push" << std::endl;
        runs.push(new_run);
        std::cout << "in AddRun - after push" << std::endl;
    }

    /// Remove run from front of queue
    void RemoveRun() {
        emp_assert(IsEmpty(), "Queue is empty! Cannot remove!");
        runs.pop();
    }

    /// Returns the element at the front of the queue
    RunInfo& FrontRun() {
        emp_assert(IsEmpty(), "Queue is empty! Cannot remove!");
        return runs.front();
    }

    /// Returns this div
    emp::web::Div GetDiv() {
        return my_div_;
    }
    /// Clears the content of this div
    void ResetDiv() {
        my_div_.Clear();
    }

    /// Deletes a table parameter BEFORE runs are queued - TWEAK SO IT DOESN"T HAVE TO RE-DRAW TABLE EVERY TIME
    void DeleteParameter(const std::string& parameter) {
        active_parameters.erase(std::remove(active_parameters.begin(), active_parameters.end(), parameter), active_parameters.end());
        DivAddTable(1, active_parameters.size() + 4, table_id);
    }

    /// Initializes table to web
    void DivAddTable(size_t row, size_t col, std::string id) {
        table_id = id;
        emp::web::Table result_tab(row, col, id);
        result_tab.SetCSS("border-collapse", "collapse");
        result_tab.SetCSS("border", "3px solid black");
        result_tab.CellsCSS("border", "1px solid black");

        result_tab.GetCell(0, 0).SetHeader() << "Run";

        // Displays config parameters onto table
        int count = 1;
        for (auto group : queue_config.group_set) {
            for (size_t i = 0; i < group->GetSize(); ++i) {
                if (std::find(active_parameters.begin(), active_parameters.end(), group->GetEntry(i)->GetName()) != active_parameters.end()) {
                    result_tab.GetCell(0, count).SetHeader() << group->GetEntry(i)->GetName();
                    ++count;
                }
            }
        }

        /*
        result_tab.GetCell(0, 1).SetHeader() << "<i>r</i>";
        result_tab.GetCell(0, 2).SetHeader() << "<i>u</i>";
        result_tab.GetCell(0, 3).SetHeader() << "<i>N</i>";
        result_tab.GetCell(0, 4).SetHeader() << "<i>E</i>";
        */
        result_tab.GetCell(0, count).SetHeader() << "Epoch";
        result_tab.GetCell(0, ++count).SetHeader() << "Num Coop";
        result_tab.GetCell(0, ++count).SetHeader() << "Num Defect";

        my_div_ << result_tab;
    }

    /// Extends table once button is clicked
    void DivButtonTable(SimplePDWorld world, int run_id) {
        emp::web::Table my_table = my_div_.Find(table_id);
        // Update the table.
        int line_id = my_table.GetNumRows();
        std::cout << line_id << std::endl;
        my_table.Rows(line_id + 1);
        my_table.GetCell(line_id, 0) << run_id;
        my_table.GetCell(line_id, 1) << world.GetR();
        my_table.GetCell(line_id, 2) << world.GetU();
        my_table.GetCell(line_id, 3) << world.GetN();
        my_table.GetCell(line_id, 4) << world.GetE();
        my_table.GetCell(line_id, 5) << "Waiting...";  // world.GetE();
        my_table.GetCell(line_id, 6) << "Waiting...";  // world.CountCoop();
        my_table.GetCell(line_id, 7) << "Waiting...";  // (world.GetN() - world.CountCoop());

        // Draw the new table.
        my_table.CellsCSS("border", "1px solid black");
        my_table.Redraw();
    }

    /// Run info in table is updated
    void DivInfoTable(size_t id, size_t cur_epoch, size_t num_coop, size_t num_defect) {
        emp::web::Table my_table = my_div_.Find(table_id);
        my_table.Freeze();
        my_table.GetCell(id + 1, 5).ClearChildren() << cur_epoch;
        my_table.GetCell(id + 1, 6).ClearChildren() << num_coop;
        my_table.GetCell(id + 1, 7).ClearChildren() << num_defect;
        my_table.Activate();
    }

    void DivTableCalc(SimplePDWorld& world) {
        size_t id = FrontRun().id;
        size_t cur_epoch = world.GetEpoch();
        RunInfo& current_run = FrontRun();

        if ((current_run.config).TEST_E() <= cur_epoch) {  // Are we done with this run?
            RemoveRun();                                   // Updates to the next run
        }

        current_run.cur_epoch = cur_epoch;
        current_run.num_coop = world.CountCoop();
        current_run.num_defect = (current_run.config).TEST_N() - current_run.num_coop;

        DivInfoTable(id, cur_epoch, current_run.num_coop, current_run.num_defect);
    }

    /// Creates area for user to input how many runs will be queued
    void DivAddTextArea(SimplePDWorld& world) {
        emp::web::TextArea run_input([&world](const std::string& str) {
            size_t num_runs = emp::from_string<size_t>(str);
            world.SetNumRuns(num_runs); }, "run_count");

        run_input.SetText(emp::to_string(world.GetNumRuns()));
        my_div_ << run_input;
    }

    /// Creates queue button - TWEAK SO THIS CAN WORK FOR OTHER WORLDS, NOT JUST SimplePDWorld
    void DivButton(SimplePDWorld& world) {
        emp::web::Button my_button([&]() {
            size_t num_runs = world.GetNumRuns();
            for (int run_id = 0; run_id < num_runs; run_id++) {
                std::cout << num_runs << std::endl;
                std::cout << "run: " << run_id << std::endl;
                MyConfig new_config = queue_config;
                /*
                for (auto group : new_config.group_set) {
                    for (size_t i = 0; i < group->GetSize(); ++i) {
                        // hard coded - fix!!
                        std::string setting_name = group->GetEntry(i)->GetName();
                        if (i == 0) {
                            std::string get_str = std::to_string(world.GetR());
                            new_config(setting_name, get_str);
                        } else if (i == 1) {
                            std::string get_str = std::to_string(world.GetU());
                            new_config(setting_name, get_str);
                        } else if (i == 2) {
                            std::string get_str = std::to_string(world.GetN());
                            new_config(setting_name, get_str);
                        } else if (i == 3) {
                            std::string get_str = std::to_string(world.GetE());
                            new_config(setting_name, get_str);
                        }
                    }
                }
                */
                new_config.TEST_R(world.GetR());
                new_config.TEST_U(world.GetU());
                new_config.TEST_N(world.GetN());
                new_config.TEST_E(world.GetE());
                for (auto group : new_config.group_set) {
                    std::cout << run_id << " - new config below: " << std::endl;
                    for (size_t i = 0; i < group->GetSize(); ++i) {
                        std::string str = group->GetEntry(i)->GetName();
                        std::cout << str << " , value =  " << new_config(str) << std::endl;
                    }
                    std::cout << std::endl;
                }
                std::cout << "before AddRun" << std::endl;
                AddRun(new_config);
                std::cout << "made it past AddRun!" << std::endl;
                DivButtonTable(world, run_id);
                std::cout << "made it past DivButtonTable" << std::endl;
            }
        },
                                   "Queue", "queue_but");
        my_div_ << my_button;
    }
};

}  // namespace emp