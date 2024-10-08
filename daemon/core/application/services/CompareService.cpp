/* === This file is part of bxt ===
 *
 *   SPDX-FileCopyrightText: 2023 Artem Grinev <agrinev@manjaro.org>
 *   SPDX-License-Identifier: AGPL-3.0-or-later
 *
 */
#include "CompareService.h"

namespace bxt::Core::Application {
coro::task<CompareService::Result<CompareService::CompareResult>>
    CompareService::compare(std::vector<PackageSectionDTO> const sections) {
    CompareResult result;

    for (auto const& section : sections) {
        auto const packages = co_await m_package_service.get_packages(section);

        if (!packages.has_value() || packages->empty()) {
            continue;
        }

        result.sections.emplace_back(section);

        for (auto const& package : *packages) {
            for (auto const& [location, entry] : package.pool_entries) {
                result.compare_table[{package.name, section, location}] = entry.version;
            }
        }
    }

    co_return result;
}
} // namespace bxt::Core::Application
