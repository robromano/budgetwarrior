//=======================================================================
// Copyright (c) 2013-2018 Baptiste Wicht.
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <set>

#include "api/server_api.hpp"
#include "api/assets_api.hpp"

#include "assets.hpp"
#include "accounts.hpp"
#include "guid.hpp"
#include "http.hpp"

using namespace budget;

void budget::add_assets_api(const httplib::Request& req, httplib::Response& res) {
    if (!api_start(req, res)) {
        return;
    }

    if (!parameters_present(req, {"input_name", "input_int_stocks", "input_dom_stocks", "input_bonds", "input_cash", "input_portfolio", "input_alloc", "input_share_based", "input_ticker"})) {
        api_error(req, res, "Invalid parameters");
        return;
    }

    asset asset;
    asset.guid            = budget::generate_guid();
    asset.name            = req.get_param_value("input_name");
    asset.int_stocks      = budget::parse_money(req.get_param_value("input_int_stocks"));
    asset.dom_stocks      = budget::parse_money(req.get_param_value("input_dom_stocks"));
    asset.bonds           = budget::parse_money(req.get_param_value("input_bonds"));
    asset.cash            = budget::parse_money(req.get_param_value("input_cash"));
    asset.portfolio       = req.get_param_value("input_portfolio") == "yes";
    asset.portfolio_alloc = budget::parse_money(req.get_param_value("input_alloc"));
    asset.currency        = req.get_param_value("input_currency");
    asset.share_based     = req.get_param_value("input_share_based") == "yes";
    asset.ticker          = req.get_param_value("input_ticker");

    if (asset.int_stocks + asset.dom_stocks + asset.bonds + asset.cash != money(100)) {
        api_error(req, res, "The total allocation of the asset is not 100%");
        return;
    }

    add_asset(std::move(asset));

    api_success(req, res, "asset " + to_string(asset.id) + " has been created", to_string(asset.id));
}

void budget::edit_assets_api(const httplib::Request& req, httplib::Response& res) {
    if (!api_start(req, res)) {
        return;
    }

    if (!parameters_present(req, {"input_id", "input_name", "input_int_stocks", "input_dom_stocks", "input_bonds", "input_cash", "input_portfolio", "input_alloc", "input_share_based", "input_ticker"})) {
        api_error(req, res, "Invalid parameters");
        return;
    }

    auto id = req.get_param_value("input_id");

    if (!budget::asset_exists(budget::to_number<size_t>(id))) {
        api_error(req, res, "asset " + id + " does not exist");
        return;
    }

    asset& asset          = asset_get(budget::to_number<size_t>(id));
    asset.name            = req.get_param_value("input_name");
    asset.int_stocks      = budget::parse_money(req.get_param_value("input_int_stocks"));
    asset.dom_stocks      = budget::parse_money(req.get_param_value("input_dom_stocks"));
    asset.bonds           = budget::parse_money(req.get_param_value("input_bonds"));
    asset.cash            = budget::parse_money(req.get_param_value("input_cash"));
    asset.portfolio       = req.get_param_value("input_portfolio") == "yes";
    asset.portfolio_alloc = budget::parse_money(req.get_param_value("input_alloc"));
    asset.currency        = req.get_param_value("input_currency");
    asset.share_based     = req.get_param_value("input_share_based") == "yes";
    asset.ticker          = req.get_param_value("input_ticker");

    if (asset.int_stocks + asset.dom_stocks + asset.bonds + asset.cash != money(100)) {
        api_error(req, res, "The total allocation of the asset is not 100%");
        return;
    }

    set_assets_changed();

    api_success(req, res, "asset " + to_string(asset.id) + " has been modified");
}

void budget::delete_assets_api(const httplib::Request& req, httplib::Response& res) {
    if (!api_start(req, res)) {
        return;
    }

    if (!parameters_present(req, {"input_id"})) {
        api_error(req, res, "Invalid parameters");
        return;
    }

    auto id = req.get_param_value("input_id");

    if (!budget::asset_exists(budget::to_number<size_t>(id))) {
        api_error(req, res, "The asset " + id + " does not exit");
        return;
    }

    budget::asset_delete(budget::to_number<size_t>(id));

    api_success(req, res, "asset " + id + " has been deleted");
}

void budget::list_assets_api(const httplib::Request& req, httplib::Response& res) {
    if (!api_start(req, res)) {
        return;
    }

    std::stringstream ss;

    for (auto& asset : all_assets()) {
        ss << asset;
        ss << std::endl;
    }

    api_success_content(req, res, ss.str());
}

void budget::add_asset_values_api(const httplib::Request& req, httplib::Response& res) {
    if (!api_start(req, res)) {
        return;
    }

    if (!parameters_present(req, {"input_asset", "input_date", "input_amount"})) {
        api_error(req, res, "Invalid parameters");
        return;
    }

    asset_value asset_value;
    asset_value.guid     = budget::generate_guid();
    asset_value.amount   = budget::parse_money(req.get_param_value("input_amount"));
    asset_value.asset_id = budget::to_number<size_t>(req.get_param_value("input_asset"));
    asset_value.set_date = budget::from_string(req.get_param_value("input_date"));

    add_asset_value(std::move(asset_value));

    api_success(req, res, "Asset value " + to_string(asset_value.id) + " has been created", to_string(asset_value.id));
}

void budget::edit_asset_values_api(const httplib::Request& req, httplib::Response& res) {
    if (!api_start(req, res)) {
        return;
    }

    if (!parameters_present(req, {"input_id", "input_asset", "input_date", "input_amount"})) {
        api_error(req, res, "Invalid parameters");
        return;
    }

    auto id = req.get_param_value("input_id");

    if (!budget::asset_value_exists(budget::to_number<size_t>(id))) {
        api_error(req, res, "Asset value " + id + " does not exist");
        return;
    }

    asset_value& asset_value = asset_value_get(budget::to_number<size_t>(id));
    asset_value.amount       = budget::parse_money(req.get_param_value("input_amount"));
    asset_value.asset_id     = budget::to_number<size_t>(req.get_param_value("input_asset"));
    asset_value.set_date     = budget::from_string(req.get_param_value("input_date"));

    set_asset_values_changed();

    api_success(req, res, "Asset " + to_string(asset_value.id) + " has been modified");
}

void budget::delete_asset_values_api(const httplib::Request& req, httplib::Response& res) {
    if (!api_start(req, res)) {
        return;
    }

    if (!parameters_present(req, {"input_id"})) {
        api_error(req, res, "Invalid parameters");
        return;
    }

    auto id = req.get_param_value("input_id");

    if (!budget::asset_value_exists(budget::to_number<size_t>(id))) {
        api_error(req, res, "The asset value " + id + " does not exit");
        return;
    }

    budget::asset_value_delete(budget::to_number<size_t>(id));

    api_success(req, res, "The asset value " + id + " has been deleted");
}

void budget::list_asset_values_api(const httplib::Request& req, httplib::Response& res) {
    if (!api_start(req, res)) {
        return;
    }

    std::stringstream ss;

    for (auto& asset_value : all_asset_values()) {
        ss << asset_value;
        ss << std::endl;
    }

    api_success_content(req, res, ss.str());
}

void budget::batch_asset_values_api(const httplib::Request& req, httplib::Response& res) {
    if (!api_start(req, res)) {
        return;
    }

    for (auto& asset : all_assets()) {
        auto input_name = "input_amount_" + budget::to_string(asset.id);

        if (req.has_param(input_name.c_str())) {
            auto new_amount = budget::parse_money(req.get_param_value(input_name.c_str()));

            budget::money current_amount;

            for (auto& asset_value : all_asset_values()) {
                if (asset_value.asset_id == asset.id) {
                    current_amount = asset_value.amount;
                }
            }

            // If the amount changed, update it
            if (current_amount != new_amount) {
                asset_value asset_value;
                asset_value.guid     = budget::generate_guid();
                asset_value.amount   = new_amount;
                asset_value.asset_id = asset.id;
                asset_value.set_date = budget::from_string(req.get_param_value("input_date"));

                add_asset_value(std::move(asset_value));
            }
        }
    }

    api_success(req, res, "Asset values have been updated");
}

void budget::add_asset_shares_api(const httplib::Request& req, httplib::Response& res) {
    if (!api_start(req, res)) {
        return;
    }

    if (!parameters_present(req, {"input_asset", "input_shares", "input_price", "input_date"})) {
        api_error(req, res, "Invalid parameters");
        return;
    }

    asset_share asset_share;
    asset_share.guid     = budget::generate_guid();
    asset_share.asset_id = budget::to_number<size_t>(req.get_param_value("input_asset"));
    asset_share.shares   = budget::to_number<size_t>(req.get_param_value("input_shares"));
    asset_share.price    = budget::parse_money(req.get_param_value("input_price"));
    asset_share.date     = budget::from_string(req.get_param_value("input_date"));

    add_asset_share(std::move(asset_share));

    api_success(req, res, "Asset share " + to_string(asset_share.id) + " has been created", to_string(asset_share.id));
}

void budget::edit_asset_shares_api(const httplib::Request& req, httplib::Response& res) {
    if (!api_start(req, res)) {
        return;
    }

    if (!parameters_present(req, {"input_id", "input_asset", "input_shares", "input_price", "input_date"})) {
        api_error(req, res, "Invalid parameters");
        return;
    }

    auto id = req.get_param_value("input_id");

    if (!budget::asset_share_exists(budget::to_number<size_t>(id))) {
        api_error(req, res, "Asset share " + id + " does not exist");
        return;
    }

    asset_share& asset_share = asset_share_get(budget::to_number<size_t>(id));
    asset_share.asset_id     = budget::to_number<size_t>(req.get_param_value("input_asset"));
    asset_share.shares       = budget::to_number<size_t>(req.get_param_value("input_shares"));
    asset_share.price        = budget::parse_money(req.get_param_value("input_price"));
    asset_share.date         = budget::from_string(req.get_param_value("input_date"));

    set_asset_shares_changed();

    api_success(req, res, "Asset " + to_string(asset_share.id) + " has been modified");
}

void budget::delete_asset_shares_api(const httplib::Request& req, httplib::Response& res) {
    if (!api_start(req, res)) {
        return;
    }

    if (!parameters_present(req, {"input_id"})) {
        api_error(req, res, "Invalid parameters");
        return;
    }

    auto id = req.get_param_value("input_id");

    if (!budget::asset_share_exists(budget::to_number<size_t>(id))) {
        api_error(req, res, "The asset share " + id + " does not exit");
        return;
    }

    budget::asset_share_delete(budget::to_number<size_t>(id));

    api_success(req, res, "The asset share " + id + " has been deleted");
}

void budget::list_asset_shares_api(const httplib::Request& req, httplib::Response& res) {
    if (!api_start(req, res)) {
        return;
    }

    std::stringstream ss;

    for (auto& asset_share : all_asset_shares()) {
        ss << asset_share;
        ss << std::endl;
    }

    api_success_content(req, res, ss.str());
}
