//=======================================================================
// Copyright (c) 2013-2018 Baptiste Wicht.
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <set>
#include <thread>

#include "cpp_utils/assert.hpp"

#include "server.hpp"
#include "expenses.hpp"
#include "earnings.hpp"
#include "accounts.hpp"
#include "assets.hpp"
#include "config.hpp"
#include "objectives.hpp"
#include "wishes.hpp"
#include "fortune.hpp"
#include "recurring.hpp"
#include "debts.hpp"
#include "currency.hpp"
#include "share.hpp"
#include "http.hpp"

#include "api/server_api.hpp"
#include "pages/server_pages.hpp"

using namespace budget;

namespace {

bool server_running = false;

httplib::Server * server_ptr = nullptr;
volatile bool cron = true;

void server_signal_handler(int signum) {
    std::cout << "INFO: Received signal (" << signum << ")" << std::endl;

    cron = false;

    if (server_ptr) {
        server_ptr->stop();
    }
}

void install_signal_handler() {
    struct sigaction action;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_handler = server_signal_handler;
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);

    std::cout << "INFO: Installed the signal handler" << std::endl;
}

void start_server(){
    std::cout << "INFO: Started the server thread" << std::endl;

    httplib::Server server;

    load_pages(server);
    load_api(server);

    install_signal_handler();

    auto port = get_server_port();
    auto listen = get_server_listen();
    server_ptr = &server;

    // Listen
    std::cout << "INFO: Server is starting to listen on " << listen << ':' << port << std::endl;
    server.listen(listen.c_str(), port);
    std::cout << "INFO: Server has exited" << std::endl;
}

void start_cron_loop(){
    std::cout << "INFO: Started the cron thread" << std::endl;
    size_t seconds = 0;

    while(cron){
        using namespace std::chrono_literals;

        std::this_thread::sleep_for(1s);
        ++seconds;

        check_for_recurrings();

        auto hours = seconds / 3600;

        // We save the cache once per day
        if (hours && hours % 24 == 0) {
            save_currency_cache();
            save_share_price_cache();
        }

        // Every four hours, we refresh the currency cache
        // Only current day rates are refreshed
        if (hours && hours % 4 == 0) {
            std::cout << "Refresh the currency cache" << std::endl;
            budget::refresh_currency_cache();
        }
    }

    std::cout << "INFO: Cron Thread has exited" << std::endl;
}

} //end of anonymous namespace

void budget::set_server_running(){
    // Indicates to the system that it's running in server mode
    server_running = true;
}

void budget::server_module::load(){
    load_accounts();
    load_expenses();
    load_earnings();
    load_assets();
    load_objectives();
    load_wishes();
    load_fortunes();
    load_recurrings();
    load_debts();
    load_wishes();
}

void budget::server_module::handle(const std::vector<std::string>& args){
    cpp_unused(args);

    std::cout << "Starting the threads" << std::endl;

    std::thread server_thread([](){ start_server(); });
    std::thread cron_thread([](){ start_cron_loop(); });

    server_thread.join();
    cron_thread.join();
}

bool budget::is_server_running(){
    return server_running;
}
