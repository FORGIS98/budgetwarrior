//=======================================================================
// Copyright (c) 2013-2020 Baptiste Wicht.
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <set>

#include "api/server_api.hpp"
#include "api/fortunes_api.hpp"

#include "fortune.hpp"
#include "accounts.hpp"
#include "guid.hpp"
#include "http.hpp"

using namespace budget;

void budget::add_fortunes_api(const httplib::Request& req, httplib::Response& res) {
    if (!api_start(req, res)) {
        return;
    }

    if (!req.has_param("input_amount") || !req.has_param("input_date")) {
        api_error(req, res, "Invalid parameters");
        return;
    }

    fortune fortune;
    fortune.guid       = budget::generate_guid();
    fortune.check_date = budget::from_string(req.get_param_value("input_date"));
    fortune.amount     = budget::parse_money(req.get_param_value("input_amount"));

    add_fortune(std::move(fortune));

    api_success(req, res, "Fortune " + to_string(fortune.id) + " has been created", to_string(fortune.id));
}

void budget::edit_fortunes_api(const httplib::Request& req, httplib::Response& res) {
    if (!api_start(req, res)) {
        return;
    }

    if (!req.has_param("input_id") || !req.has_param("input_amount") || !req.has_param("input_date")) {
        api_error(req, res, "Invalid parameters");
        return;
    }

    auto id = req.get_param_value("input_id");

    if (!budget::fortune_exists(budget::to_number<size_t>(id))) {
        api_error(req, res, "Fortune " + id + " does not exist");
        return;
    }

    fortune& fortune   = fortune_get(budget::to_number<size_t>(id));
    fortune.check_date = budget::from_string(req.get_param_value("input_date"));
    fortune.amount     = budget::parse_money(req.get_param_value("input_amount"));

    set_fortunes_changed();

    api_success(req, res, "Fortune " + to_string(fortune.id) + " has been modified");
}

void budget::delete_fortunes_api(const httplib::Request& req, httplib::Response& res) {
    if (!api_start(req, res)) {
        return;
    }

    if (!req.has_param("input_id")) {
        api_error(req, res, "Invalid parameters");
        return;
    }

    auto id = req.get_param_value("input_id");

    if (!budget::fortune_exists(budget::to_number<size_t>(id))) {
        api_error(req, res, "The fortune " + id + " does not exit");
        return;
    }

    budget::fortune_delete(budget::to_number<size_t>(id));

    api_success(req, res, "fortune " + id + " has been deleted");
}

void budget::list_fortunes_api(const httplib::Request& req, httplib::Response& res) {
    if (!api_start(req, res)) {
        return;
    }

    std::stringstream ss;

    for (auto& fortune : all_fortunes()) {
        ss << fortune;
        ss << std::endl;
    }

    api_success_content(req, res, ss.str());
}

