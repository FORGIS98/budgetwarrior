//=======================================================================
// Copyright (c) 2013-2018 Baptiste Wicht.
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include "incomes.hpp"

#include "writer.hpp"
#include "pages/incomes_pages.hpp"
#include "http.hpp"

using namespace budget;

void budget::incomes_page(const httplib::Request& req, httplib::Response& res) {
    std::stringstream content_stream;
    if (!page_start(req, res, content_stream, "All Incomes")) {
        return;
    }

    budget::html_writer w(content_stream);
    budget::show_incomes(w);

    make_tables_sortable(w);

    page_end(w, req, res);
}
