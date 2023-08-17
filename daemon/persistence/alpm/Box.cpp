/* === This file is part of bxt ===
 *
 *   SPDX-FileCopyrightText: 2022 Artem Grinev <agrinev@manjaro.org>
 *   SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
#include "Box.h"

#include "core/domain/events/PackageEvents.h"
#include "coro/sync_wait.hpp"

#include <fmt/format.h>
#include <infrastructure/PackageFile.h>

namespace bxt::Persistence {

coro::task<Box::TResult> Box::find_by_id_async(TId id) {
}

coro::task<Box::TResult>
    Box::find_first_async(std::function<bool(const Package &)>) {
}

coro::task<Box::TResults>
    Box::find_async(std::function<bool(const Package &)> condition) {
}

coro::task<Box::TResults> Box::all_async() {
}

coro::task<void> Box::add_async(const Package entity) {
    m_to_add.emplace_back(entity);
    co_return;
}

coro::task<void> Box::remove_async(const TId entity) {
    m_to_remove.emplace_back(entity);
    co_return;
}

coro::task<void> Box::add_async(const std::vector<Package> entities) {
    m_to_add.insert(m_to_add.end(), entities.begin(), entities.end());
    co_return;
}

coro::task<void> Box::update_async(const Package entity) {
    m_to_update.emplace_back(entity);
    co_return;
}

coro::task<std::vector<Core::Domain::Package>>
    Box::find_by_section_async(const Core::Domain::Section section) const {
    auto packages = m_map.at(SectionDTOMapper::to_dto(section)).packages();

    std::vector<Core::Domain::Package> result;
    result.reserve(packages.size());

    std::ranges::transform(
        packages, std::back_inserter(result), [section](const auto &package) {
            return Core::Domain::Package::from_filename(section, package);
        });

    co_return {result.begin(), result.end()};
}

coro::task<void> Box::commit_async() {
    phmap::node_hash_map<PackageSectionDTO, std::set<std::string>> paths_to_add;
    std::vector<coro::task<void>> tasks;

    for (const auto &entity : m_to_add) {
        auto section_dto = SectionDTOMapper::to_dto(entity.section());

        std::cout << fmt::format("Symlinking {}/{}\n", std::string(section_dto),
                                 entity.filepath().filename().string());

        std::error_code ec;

        auto target_path = std::filesystem::absolute(fmt::format(
            "{}/{}/{}", m_options.location, std::string(section_dto),
            entity.filepath().filename().string()));

        auto source_path = std::filesystem::relative(entity.filepath(),
                                                     target_path.parent_path());

        std::filesystem::create_symlink(source_path, target_path, ec);

        if (entity.has_signature()) {
            std::filesystem::create_symlink(
                fmt::format("{}.sig", source_path.string()),
                fmt::format("{}.sig", target_path.string()), ec);
        }

        paths_to_add[section_dto].emplace(entity.filepath().string());

        auto event = std::make_shared<Events::PackageAdded>(entity);

        m_event_store.emplace_back(event);
    }

    tasks.reserve(paths_to_add.size());

    for (const auto &[section, values] : paths_to_add) {
        tasks.emplace_back(m_map.at(section).add(values));
    }

    phmap::node_hash_map<PackageSectionDTO, std::set<std::string>>
        names_to_remove;

    /// TODO: Implement update and remove

    co_await coro::when_all(std::move(tasks));

    co_return;
}

coro::task<void> Box::rollback_async() {
    m_to_add.clear();
    m_to_remove.clear();
    m_to_update.clear();

    co_return;
}

std::vector<Events::EventPtr> Box::event_store() const {
    return m_event_store;
}

} // namespace bxt::Persistence
