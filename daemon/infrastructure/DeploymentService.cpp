/* === This file is part of bxt ===
 *
 *   SPDX-FileCopyrightText: 2022 Artem Grinev <agrinev@manjaro.org>
 *   SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
#include "DeploymentService.h"

#include <iostream>
#include <random>

namespace bxt::Infrastructure {

coro::task<uint64_t> DeploymentService::deploy_start()
{
    std::mt19937_64 engine(std::random_device{}());
    std::uniform_int_distribution<uint64_t> distribution;
    auto session_id = distribution(engine);

    m_session_packages.try_emplace(session_id, std::set<PackageDTO>());

    co_return session_id;
}

coro::task<void> DeploymentService::deploy_push(PackageDTO package, uint64_t session_id)
{
    if (!std::filesystem::exists(package.filepath)) {
        throw std::invalid_argument("File not found");
    }

    m_session_packages.at(session_id).emplace(package);

    co_return;
}

coro::task<bool> DeploymentService::verify_session(uint64_t session_id)
{
    co_return m_session_packages.count(session_id) > 0;
}

coro::task<void> DeploymentService::process_package(PackageDTO package)
{

    std::filesystem::create_directories(m_options.pool(package.section));

    auto deployed_entity = PackageDTOMapper::to_entity(package);

    auto current_entitites = m_repository.find_by_section(
        SectionDTOMapper::to_entity(package.section));

    auto current_entity = std::ranges::find(current_entitites, package.name, &Package::name);

    if (current_entity != current_entitites.end()
        && deployed_entity.version() <= current_entity->version()) {
        co_return;
    }

    std::error_code ec;

    std::filesystem::rename(package.filepath,
                            m_options.pool(package.section) / package.filepath.filename(),
                            ec);

    // try copy + remove original
    if (ec) {
        std::filesystem::copy(package.filepath,
                              m_options.pool(package.section) / package.filepath.filename(),
                              std::filesystem::copy_options::overwrite_existing);

        std::filesystem::remove(package.filepath);
    }

    auto renamed_package = package;

    renamed_package.filepath = m_options.pool(package.section) / package.filepath.filename();

    co_await m_repository.add_async(PackageDTOMapper::to_entity(renamed_package));
    co_return;
}

coro::task<void> DeploymentService::deploy_end(uint64_t session_id)
{
    const auto &packages = m_session_packages.at(session_id);

    std::vector<coro::task<void>> tasks;
    tasks.reserve(packages.size());

    for (const auto &package : packages) {
        tasks.push_back(process_package(package));
    }

    co_await coro::when_all(std::move(tasks));

    co_await m_repository.commit_async();

    m_session_packages.erase(session_id);

    co_return;
}

} // namespace bxt::Infrastructure
