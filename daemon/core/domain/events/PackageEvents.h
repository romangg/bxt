/* === This file is part of bxt ===
 *
 *   SPDX-FileCopyrightText: 2023 Artem Grinev <agrinev@manjaro.org>
 *   SPDX-License-Identifier: AGPL-3.0-or-later
 *
 */
#pragma once

#include "core/domain/entities/Package.h"
#include "EventBase.h"

namespace bxt::Core::Domain::Events {

struct PackageAdded : public EventBase {
    PackageAdded(Package const& package)
        : package(package) {
    }
    Package package;

    virtual std::string message() const {
        return fmt::format("Package {}/{} added", package.section().string(), package.string());
    }
};

struct PackageRemoved : public EventBase {
    PackageRemoved(Package::TId const& id)
        : id(id) {
    }

    Package::TId id;

    virtual std::string message() const {
        return fmt::format("Package {}/{} removed", id.section.string(),
                           std::string(id.package_name));
    }
};

struct PackageUpdated : public EventBase {
    PackageUpdated(Package const& new_package, Package const& old_package)
        : new_package(new_package)
        , old_package(old_package) {
    }

    Package new_package;

    Package old_package;

    virtual std::string message() const {
        return fmt::format("Package {}/{} updated to {}/{}", old_package.section().string(),
                           old_package.string(), new_package.section().string(),
                           new_package.string());
    }
};

} // namespace bxt::Core::Domain::Events
