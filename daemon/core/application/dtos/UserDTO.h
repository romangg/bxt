/* === This file is part of bxt ===
 *
 *   SPDX-FileCopyrightText: 2022 Artem Grinev <agrinev@manjaro.org>
 *   SPDX-License-Identifier: AGPL-3.0-or-later
 *
 */
#pragma once

#include "core/domain/entities/User.h"
#include "utilities/StaticDTOMapper.h"

#include <cereal/types/set.hpp>
#include <optional>
#include <ranges>

namespace bxt::Core::Application {
struct UserDTO {
    std::string name;
    std::optional<std::string> password;
    std::optional<std::set<std::string>> permissions;

    template<class Archive> void serialize(Archive& ar) {
        ar(name, password, permissions);
    }
};

using UserDTOMapper = bxt::Utilities::StaticDTOMapper<bxt::Core::Domain::User, UserDTO>;

} // namespace bxt::Core::Application

namespace bxt::Utilities {

namespace {
    using namespace bxt::Core::Domain;
    using namespace bxt::Core::Application;
} // namespace

template<> struct StaticDTOMapper<User, UserDTO> {
    static User to_entity(UserDTO const& from) {
        User to(from.name, *from.password);

        std::set<Permission> mapped_permissions;
        std::ranges::transform(
            *from.permissions, std::inserter(mapped_permissions, mapped_permissions.end()),
            [](std::string const& permission) { return Permission(permission); });

        to.set_permissions(mapped_permissions);
        return to;
    }

    static UserDTO to_dto(User const& from) {
        UserDTO to;
        to.name = std::string(from.name());
        to.password = from.password();
        std::set<std::string> mapped_permissions;

        std::ranges::transform(
            from.permissions(), std::inserter(mapped_permissions, mapped_permissions.end()),
            [](Permission const& permission) { return std::string(permission); });

        to.permissions = mapped_permissions;
        return to;
    }
};

} // namespace bxt::Utilities
