/* === This file is part of bxt ===
 *
 *   SPDX-FileCopyrightText: 2022 Artem Grinev <agrinev@manjaro.org>
 *   SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
#pragma once

#include "ArchRepoOptions.h"
#include "core/application/services/SyncService.h"
#include "infrastructure/PackageFile.h"

#include <coro/thread_pool.hpp>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

namespace bxt::Infrastructure {

class ArchRepoSyncService : public bxt::Core::Application::SyncService {
public:
    ArchRepoSyncService(ArchRepoOptions& options) : m_options(options) {}

    virtual coro::task<void> sync(const PackageSectionDTO& section) override;
    virtual coro::task<void> sync_all() override;

protected:
    coro::task<std::vector<std::string>>
        get_available_packages(const PackageSectionDTO& section);
    coro::task<PackageFile> download_package(const PackageSectionDTO& section,
                                             const std::string& name);

    coro::task<std::unique_ptr<httplib::SSLClient>>
        get_client(const std::string& url);

private:
    ArchRepoOptions& m_options;
    coro::thread_pool tp {coro::thread_pool::options {.thread_count = 4}};
};

} // namespace bxt::Infrastructure