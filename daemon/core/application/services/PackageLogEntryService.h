/* === This file is part of bxt ===
 *
 *   SPDX-FileCopyrightText: %YEAR% Artem Grinev <agrinev@manjaro.org>
 *   SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
#pragma once

#include "core/application/dtos/PackageLogEntryDTO.h"
#include "core/domain/entities/PackageLogEntry.h"
#include "core/domain/repositories/RepositoryBase.h"
#include "dexode/EventBus.hpp"

#include <memory>

namespace bxt::Core::Application {

class PackageLogEntryService {
public:
    PackageLogEntryService(
        std::shared_ptr<dexode::EventBus> evbus,
        bxt::Core::Domain::ReadWriteRepositoryBase<Domain::PackageLogEntry>
            &repository)
        : m_evbus(evbus),
          m_listener(dexode::EventBus::Listener::createNotOwning(*m_evbus)),
          m_repository(repository) {}

    void init();

    coro::task<std::vector<PackageLogEntryDTO>> events();

private:
    std::shared_ptr<dexode::EventBus> m_evbus;
    dexode::EventBus::Listener m_listener;
    bxt::Core::Domain::ReadWriteRepositoryBase<Domain::PackageLogEntry>
        &m_repository;
};

} // namespace bxt::Core::Application
