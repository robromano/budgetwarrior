//=======================================================================
// Copyright (c) 2013-2018 Baptiste Wicht.
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <iostream>
#include <fstream>
#include <sstream>

#include "objectives.hpp"
#include "expenses.hpp"
#include "earnings.hpp"
#include "accounts.hpp"
#include "args.hpp"
#include "data.hpp"
#include "guid.hpp"
#include "config.hpp"
#include "utils.hpp"
#include "console.hpp"
#include "budget_exception.hpp"
#include "compute.hpp"
#include "writer.hpp"

using namespace budget;

namespace {

static data_handler<objective> objectives { "objectives", "objectives.data" };

void edit(budget::objective& objective){
    edit_string(objective.name, "Name", not_empty_checker());
    edit_string_complete(objective.type, "Type", {"monthly","yearly"}, not_empty_checker(), one_of_checker({"monthly","yearly"}));
    edit_string_complete(objective.source, "Source", {"balance", "earnings", "expenses", "savings_rate"}, not_empty_checker(), one_of_checker({"balance", "earnings", "expenses", "savings_rate"}));
    edit_string_complete(objective.op, "Operator", {"min", "max"}, not_empty_checker(), one_of_checker({"min", "max"}));
    edit_money(objective.amount, "Amount");
}

} //end of anonymous namespace

std::map<std::string, std::string> budget::objective::get_params(){
    std::map<std::string, std::string> params;

    params["input_id"]      = budget::to_string(id);
    params["input_guid"]    = guid;
    params["input_date"]    = budget::to_string(date);
    params["input_name"]    = name;
    params["input_type"]    = type;
    params["input_source"]  = source;
    params["input_op"]      = op;
    params["input_amount"]  = budget::to_string(amount);

    return params;
}

void budget::yearly_objective_status(budget::writer& w, bool lines, bool full_align){
    size_t yearly = 0;

    for (auto& objective : objectives.data) {
        if (objective.type == "yearly") {
            ++yearly;
        }
    }

    if (yearly) {
        w << title_begin << "Year objectives" << title_end;

        if (lines) {
            w << end_of_line;
        }

        size_t width = 0;
        if (full_align) {
            for (auto& objective : objectives.data) {
                width = std::max(rsize(objective.name), width);
            }
        } else {
            for (auto& objective : objectives.data) {
                if (objective.type == "yearly") {
                    width = std::max(rsize(objective.name), width);
                }
            }
        }

        //Compute the year status
        auto year_status = budget::compute_year_status();

        std::vector<std::string> columns = {"Objective", "Status", "Progress"};
        std::vector<std::vector<std::string>> contents;

        for (auto& objective : objectives.data) {
            if (objective.type == "yearly") {
                contents.push_back({objective.name, get_status(year_status, objective), get_success(year_status, objective)});
            }
        }

        w.display_table(columns, contents);
    }
}

void budget::monthly_objective_status(budget::writer& w){
    w << title_begin << "Month objectives" << title_end;

    auto today         = budget::local_day();
    auto current_month = today.month();
    auto current_year  = today.year();
    auto sm            = start_month(current_year);

    for (auto& objective : objectives.data) {
        if (objective.type == "monthly") {
            std::vector<std::string> columns = {objective.name, "Status", "Progress"};
            std::vector<std::vector<std::string>> contents;

            for (unsigned short i = sm; i <= current_month; ++i) {
                budget::month month = i;

                // Compute the month status
                auto status = budget::compute_month_status(current_year, month);

                contents.push_back({to_string(month), get_status(status, objective), get_success(status, objective)});
            }

            w.display_table(columns, contents);
        }
    }
}

void budget::current_monthly_objective_status(budget::writer& w, bool full_align){
    if (objectives.data.empty()) {
        w << title_begin << "No objectives" << title_end;
        return;
    }

    auto monthly_objectives = std::count_if(objectives.data.begin(), objectives.data.end(), [](auto& objective) {
        return objective.type == "monthly";
    });

    if (!monthly_objectives) {
        w << title_begin << "No objectives" << title_end;
        return;
    }

    w << title_begin << "Month objectives" << title_end;

    auto today = budget::local_day();

    size_t width = 0;
    if (full_align) {
        for (auto& objective : objectives.data) {
            width = std::max(rsize(objective.name), width);
        }
    } else {
        for (auto& objective : objectives.data) {
            if (objective.type == "monthly") {
                width = std::max(rsize(objective.name), width);
            }
        }
    }

    std::vector<std::string> columns = {"Objective", "Status", "Progress"};
    std::vector<std::vector<std::string>> contents;

    // Compute the month status
    auto status = budget::compute_month_status(today.year(), today.month());

    for (auto& objective : objectives.data) {
        if (objective.type == "monthly") {
            contents.push_back({objective.name, get_status(status, objective), get_success(status, objective)});
        }
    }

    w.display_table(columns, contents);
}

int budget::compute_success(const budget::status& status, const budget::objective& objective){
    auto amount = objective.amount;

    budget::money basis;
    if (objective.source == "expenses") {
        basis = status.expenses;
    } else if (objective.source == "earnings") {
        basis = status.earnings;
    } else if (objective.source == "savings_rate") {
        auto savings        = status.income - status.expenses;
        double savings_rate = 0.0;

        if (savings.dollars() > 0) {
            savings_rate = 100 * (savings / status.income);
        }

        basis = budget::money(int(savings_rate));
    } else {
        basis = status.balance;
    }

    int success = 0;
    if (objective.op == "min") {
        auto percent = basis.dollars() / static_cast<double>(amount.dollars());
        success      = percent * 100;
    } else if (objective.op == "max") {
        auto percent = amount.dollars() / static_cast<double>(basis.dollars());
        success      = percent * 100;
    }

    success = std::max(0, success);

    return success;
}

void budget::objectives_module::load(){
    load_expenses();
    load_earnings();
    load_accounts();
    load_objectives();
}

void budget::objectives_module::unload(){
    save_objectives();
}

void budget::objectives_module::handle(const std::vector<std::string>& args){
    if(args.size() == 1){
        console_writer w(std::cout);
        status_objectives(w);
    } else {
        auto& subcommand = args[1];

        if(subcommand == "list"){
            console_writer w(std::cout);
            budget::list_objectives(w);
        } else if(subcommand == "status"){
            console_writer w(std::cout);
            status_objectives(w);
        } else if(subcommand == "add"){
            objective objective;
            objective.guid = generate_guid();
            objective.date = budget::local_day();

            edit(objective);

            auto id = objectives.add(std::move(objective));
            std::cout << "Objective " << id << " has been created" << std::endl;
        } else if(subcommand == "delete"){
            enough_args(args, 3);

            size_t id = to_number<size_t>(args[2]);

            if(!objectives.exists(id)){
                throw budget_exception("There are no objective with id " + args[2]);
            }

            objectives.remove(id);

            std::cout << "Objective " << id << " has been deleted" << std::endl;
        } else if(subcommand == "edit"){
            enough_args(args, 3);

            size_t id = to_number<size_t>(args[2]);

            if(!objectives.exists(id)){
                throw budget_exception("There are no objective with id " + args[2]);
            }

            auto& objective = objectives[id];

            edit(objective);

            if (objectives.edit(objective)) {
                std::cout << "Objective " << id << " has been modified" << std::endl;
            }
        } else {
            throw budget_exception("Invalid subcommand \"" + subcommand + "\"");
        }
    }
}

void budget::load_objectives(){
    objectives.load();
}

void budget::save_objectives(){
    objectives.save();
}

std::ostream& budget::operator<<(std::ostream& stream, const objective& objective){
    return stream
        << objective.id  << ':'
        << objective.guid << ':'
        << objective.name << ':'
        << objective.type << ':'
        << objective.source << ':'
        << objective.op << ':'
        << objective.amount << ':'
        << to_string(objective.date);
}

void budget::operator>>(const std::vector<std::string>& parts, objective& objective){
    bool random = config_contains("random");

    objective.id = to_number<size_t>(parts[0]);
    objective.guid = parts[1];
    objective.name = parts[2];
    objective.type = parts[3];
    objective.source = parts[4];
    objective.op = parts[5];
    objective.date = from_string(parts[7]);

    if(random){
        objective.amount = budget::random_money(1000, 10000);
    } else {
        objective.amount = parse_money(parts[6]);
    }
}

std::vector<objective>& budget::all_objectives(){
    return objectives.data;
}

void budget::set_objectives_changed(){
    objectives.set_changed();
}

void budget::set_objectives_next_id(size_t next_id){
    objectives.next_id = next_id;
}

void budget::list_objectives(budget::writer& w){
    w << title_begin << "Objectives " << add_button("objectives") << title_end;

    if (objectives.data.size() == 0) {
        w << "No objectives" << end_of_line;
    } else {
        std::vector<std::string> columns = {"ID", "Name", "Type", "Source", "Operator", "Amount", "Edit"};
        std::vector<std::vector<std::string>> contents;

        for (auto& objective : objectives.data) {
            contents.push_back({to_string(objective.id), objective.name, objective.type, objective.source, objective.op, to_string(objective.amount), "::edit::objectives::" + to_string(objective.id)});
        }

        w.display_table(columns, contents);
    }
}

void budget::status_objectives(budget::writer& w){
    w << title_begin << "Objectives " << add_button("objectives") << title_end;

    if(objectives.data.size() == 0){
        w << "No objectives" << end_of_line;
    } else {
        auto today = budget::local_day();

        if(today.day() < 12){
            w << p_begin << "WARNING: It is early in the month, no one can know what may happen ;)" << p_end;
        }

        size_t monthly = 0;
        size_t yearly = 0;

        for(auto& objective : objectives.data){
            if(objective.type == "yearly"){
                ++yearly;
            } else if(objective.type == "monthly"){
                ++monthly;
            }
        }

        if(yearly){
            budget::yearly_objective_status(w, true, false);

            if(monthly){
                w << end_of_line;
            }
        }

        if(monthly){
            budget::monthly_objective_status(w);
        }
    }
}

bool budget::objective_exists(size_t id){
    return objectives.exists(id);
}

void budget::objective_delete(size_t id) {
    if (!objectives.exists(id)) {
        throw budget_exception("There are no objective with id ");
    }

    objectives.remove(id);
}

objective& budget::objective_get(size_t id) {
    if (!objectives.exists(id)) {
        throw budget_exception("There are no objective with id ");
    }

    return objectives[id];
}

void budget::add_objective(budget::objective&& objective){
    objectives.add(std::forward<budget::objective>(objective));
}

std::string budget::get_status(const budget::status& status, const budget::objective& objective){
    std::string result;

    budget::money basis;
    if(objective.source == "expenses"){
        basis = status.expenses;
    } else if (objective.source == "earnings") {
        basis = status.earnings;
    } else if (objective.source == "savings_rate") {
        double savings_rate = 0.0;

        if(status.balance.dollars() > 0){
            savings_rate = 100 * (status.balance.dollars() / double((status.budget + status.earnings).dollars()));
        }

        basis = budget::money(int(savings_rate));
    } else {
        basis = status.balance;
    }

    result += to_string(basis.dollars());
    result += "/";
    result += to_string(objective.amount.dollars());

    return result;
}

std::string budget::get_success(const budget::status& status, const budget::objective& objective){
    auto success = compute_success(status, objective);

    return "::success" + std::to_string(success);
}
