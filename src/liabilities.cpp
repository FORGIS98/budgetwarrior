//=======================================================================
// Copyright (c) 2013-2020 Baptiste Wicht.
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>
#include <map>
#include <random>

#include "liabilities.hpp"
#include "assets.hpp"
#include "budget_exception.hpp"
#include "args.hpp"
#include "data.hpp"
#include "guid.hpp"
#include "config.hpp"
#include "utils.hpp"
#include "console.hpp"
#include "writer.hpp"
#include "currency.hpp"
#include "share.hpp"

using namespace budget;

namespace {

static data_handler<liability> liabilities { "liabilities", "liabilities.data" };

std::vector<std::string> get_liabilities_names(){
    std::vector<std::string> names;

    for (auto& liability : all_liabilities()) {
        names.push_back(liability.name);
    }

    return names;
}

} //end of anonymous namespace

std::map<std::string, std::string> budget::liability::get_params(){
    std::map<std::string, std::string> params;

    params["input_id"]       = budget::to_string(id);
    params["input_guid"]     = guid;
    params["input_name"]     = name;
    params["input_currency"] = currency;

    return params;
}

void budget::liabilities_module::load(){
    load_liabilities();
    load_asset_values();
}

void budget::liabilities_module::unload(){
    save_liabilities();
    save_asset_values();
}

void budget::liabilities_module::handle(const std::vector<std::string>& args){
    console_writer w(std::cout);

    if(args.size() == 1){
        show_liabilities(w);
    } else {
        auto& subcommand = args[1];

        if(subcommand == "show"){
            show_liabilities(w);
        } else if(subcommand == "add"){
            liability liability;
            liability.guid = generate_guid();

            edit_string(liability.name, "Name", not_empty_checker());

            if(liability_exists(liability.name)){
                throw budget_exception("A liability with this name already exists");
            }

            liability.currency = budget::get_default_currency();
            edit_string(liability.currency, "Currency", not_empty_checker());

            auto id = liabilities.add(liability);
            std::cout << "liability " << id << " has been created" << std::endl;
        } else if(subcommand == "delete"){
            size_t id = 0;

            if(args.size() >= 3){
                enough_args(args, 3);

                id = to_number<size_t>(args[2]);

                if (!liabilities.exists(id)) {
                    throw budget_exception("There are no liability with id " + args[2]);
                }
            } else {
                std::string name;
                edit_string_complete(name, "Liability", get_liabilities_names(), not_empty_checker(), liability_checker());

                id = budget::get_liability(name).id;
            }

            for (auto& value : all_asset_values()) {
                if (value.liability && value.asset_id == id) {
                    throw budget_exception("There are still asset values linked to liability " + args[2]);
                }
            }

            liabilities.remove(id);

            std::cout << "Liablitiy " << id << " has been deleted" << std::endl;
        } else if(subcommand == "edit"){
            size_t id = 0;

            if(args.size() >= 3){
                enough_args(args, 3);

                id = to_number<size_t>(args[2]);

                if(!liabilities.exists(id)){
                    throw budget_exception("There are no liability with id " + args[2]);
                }
            } else {
                std::string name;
                edit_string_complete(name, "Liability", get_liabilities_names(), not_empty_checker(), liability_checker());

                id = get_liability(name).id;
            }

            auto& liability = liabilities[id];

            edit_string(liability.name, "Name", not_empty_checker());

            //Verify that there are no OTHER liability with this name

            for (auto& other_asset : all_liabilities()) {
                if (other_asset.id != id) {
                    if (other_asset.name == liability.name) {
                        throw budget_exception("There is already an liability with the name " + liability.name);
                    }
                }
            }

            edit_string(liability.currency, "Currency", not_empty_checker());

            if (liabilities.edit(liability)) {
                std::cout << "Liability " << id << " has been modified" << std::endl;
            }
        } else if (subcommand == "value") {
            if (args.size() == 2) {
                budget::show_asset_values(w, true);
            } else {
                auto& subsubcommand = args[2];

                if (subsubcommand == "set") {
                    asset_value asset_value;
                    asset_value.guid = generate_guid();
                    asset_value.liability = true;

                    std::string liability_name;
                    edit_string_complete(liability_name, "Liability", get_liabilities_names(), not_empty_checker(), liability_checker());
                    asset_value.asset_id = get_liability(liability_name).id;

                    edit_money(asset_value.amount, "Amount", not_negative_checker());

                    asset_value.set_date = budget::local_day();
                    edit_date(asset_value.set_date, "Date");

                    auto id = add_asset_value(asset_value);
                    std::cout << "Asset Value " << id << " has been created" << std::endl;
                } else if (subsubcommand == "show") {
                    show_asset_values(w, true);
                } else if (subsubcommand == "list") {
                    list_asset_values(w, true);
                } else if (subsubcommand == "edit") {
                    size_t id = 0;

                    enough_args(args, 4);

                    id = to_number<size_t>(args[3]);

                    if (!asset_value_exists(id)) {
                        throw budget_exception("There are no asset values with id " + args[3]);
                    }

                    auto& value = asset_value_get(id);

                    if (!value.liability) {
                        throw budget_exception("Cannot edit asset value from the liability module");
                    }

                    std::string liability_name = get_liability(value.asset_id).name;
                    edit_string_complete(liability_name, "Asset", get_liabilities_names(), not_empty_checker(), liability_checker());
                    value.asset_id = get_liability(liability_name).id;

                    edit_money(value.amount, "Amount", not_negative_checker());
                    edit_date(value.set_date, "Date");

                    if (edit_asset_value(value)) {
                        std::cout << "Asset Value " << id << " has been modified" << std::endl;
                    }
                } else if (subsubcommand == "delete") {
                    size_t id = 0;

                    enough_args(args, 4);

                    id = to_number<size_t>(args[3]);

                    if (!liabilities.exists(id)) {
                        throw budget_exception("There are no asset value with id " + args[2]);
                    }

                    auto& value = asset_value_get(id);

                    if (!value.liability) {
                        throw budget_exception("Cannot edit asset value from the liability module");
                    }

                    asset_value_delete(id);

                    std::cout << "Asset value " << id << " has been deleted" << std::endl;
                } else {
                    throw budget_exception("Invalid subcommand value \"" + subsubcommand + "\"");
                }
            }
        } else {
            throw budget_exception("Invalid subcommand \"" + subcommand + "\"");
        }
    }
}

void budget::load_liabilities(){
    liabilities.load();
}

void budget::save_liabilities(){
    liabilities.save();
}

budget::liability& budget::get_liability(size_t id){
    return liabilities[id];
}

budget::liability& budget::get_liability(std::string name){
    for(auto& liability : liabilities.data){
        if(liability.name == name){
            return liability;
        }
    }

    cpp_unreachable("The liability does not exist");
}

std::ostream& budget::operator<<(std::ostream& stream, const liability& liability){
    std::string classes;

    return stream << liability.id << ':' << liability.guid << ':' << liability.name << ':'
                  << liability.currency;
}

void budget::operator>>(const std::vector<std::string>& parts, liability& liability){
    bool random = config_contains("random");

    liability.id       = to_number<size_t>(parts[0]);
    liability.guid     = parts[1];
    liability.currency = parts[3];

    if(liability.guid == "XXXXX"){
        liability.guid = generate_guid();
    }

    if (random) {
        liability.name = budget::random_name(5);
    } else {
        liability.name = parts[2];
    }
}

bool budget::liability_exists(const std::string& name){
    for (auto& liability : all_liabilities()) {
        if (liability.name == name) {
            return true;
        }
    }

    return false;
}

std::vector<liability>& budget::all_liabilities(){
    return liabilities.data;
}

budget::date budget::liability_start_date(const budget::liability& liability) {
    budget::date start = budget::local_day();

    for (auto & value : all_asset_values()) {
        if (value.liability && value.asset_id == liability.id) {
            start = std::min(value.set_date, start);
        }
    }

    return start;
}

budget::date budget::liability_start_date() {
    budget::date start = budget::local_day();

    //TODO If necessary, avoid double loops

    for (auto & liability : all_liabilities()) {
        start = std::min(liability_start_date(liability), start);
    }

    return start;
}

void budget::set_liabilities_changed(){
    liabilities.set_changed();
}

void budget::set_liabilities_next_id(size_t next_id){
    liabilities.next_id = next_id;
}

void budget::show_liabilities(budget::writer& w){
    if (!liabilities.data.size()) {
        w << "No liabilities" << end_of_line;
        return;
    }

    w << title_begin << "Liabilities " << add_button("liabilities") << title_end;

    std::vector<std::string> columns = {"ID", "Name", "Currency", "Edit"};

    std::vector<std::vector<std::string>> contents;

    // Display the liabilities

    for(auto& liability : liabilities.data){
        std::vector<std::string> line;

        line.emplace_back(to_string(liability.id));
        line.emplace_back(liability.name);

        line.emplace_back(to_string(liability.currency));
        line.emplace_back("::edit::liabilities::" + budget::to_string(liability.id));

        contents.emplace_back(std::move(line));
    }

    w.display_table(columns, contents);
}

bool budget::liability_exists(size_t id){
    return liabilities.exists(id);
}

void budget::liability_delete(size_t id) {
    if (!liabilities.exists(id)) {
        throw budget_exception("There are no liability with id ");
    }

    liabilities.remove(id);
}

liability& budget::liability_get(size_t id) {
    if (!liabilities.exists(id)) {
        throw budget_exception("There are no liability with id ");
    }

    return liabilities[id];
}

void budget::add_liability(budget::liability& liability){
    liabilities.add(liability);
}

budget::money budget::get_liability_value(budget::liability & liability, budget::date d) {
    size_t asset_value_id  = 0;
    bool asset_value_found = false;

    for (auto& asset_value : all_asset_values()) {
        if (asset_value.liability && asset_value.asset_id == liability.id) {
            if (asset_value.set_date <= d) {
                if (!asset_value_found) {
                    asset_value_id = asset_value.id;
                } else if (asset_value.set_date >= get_asset_value(asset_value_id).set_date) {
                    asset_value_id = asset_value.id;
                }

                asset_value_found = true;
            }
        }
    }

    if (asset_value_found) {
        return get_asset_value(asset_value_id).amount;
    }

    return {};
}

budget::money budget::get_liability_value(budget::liability & liability) {
    return get_liability_value(liability, budget::local_day());
}

budget::money budget::get_liability_value_conv(budget::liability & liability) {
    return get_liability_value_conv(liability, budget::local_day());
}

budget::money budget::get_liability_value_conv(budget::liability & liability, budget::date d) {
    auto amount = get_liability_value(liability, d);
    return amount * exchange_rate(liability.currency, d);
}

budget::money budget::get_liability_value_conv(budget::liability & liability, const std::string& currency) {
    return get_liability_value_conv(liability, budget::local_day(), currency);
}

budget::money budget::get_liability_value_conv(budget::liability & liability, budget::date d, const std::string& currency) {
    auto amount = get_liability_value(liability, d);
    return amount * exchange_rate(liability.currency, currency, d);
}
