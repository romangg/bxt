/* === This file is part of bxt ===
 *
 *   SPDX-FileCopyrightText: 2023 Artem Grinev <agrinev@manjaro.org>
 *   SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
#include "Database.h"

#include "boost/none.hpp"
#include "core/application/dtos/PackageDTO.h"
#include "core/application/dtos/PackageSectionDTO.h"
#include "fmt/core.h"
#include "lmdb.h"
#include "lmdbxx/lmdb++.h"
#include "utilities/Error.h"
#include "utilities/alpmdb/Desc.h"
#include "utilities/box/Package.h"
#include "utilities/errors/DatabaseError.h"

#include <optional>
#include <string>
#include <string_view>
namespace bxt::Box {

std::optional<std::pair<PackageSectionDTO, std::string>>
    Database::deserialize_key(const std::string_view key) {
    std::vector<std::string> parts;
    boost::split(parts, key, boost::is_any_of("/"));
    if (parts.size() != 4) { return {}; }

    return std::make_pair(PackageSectionDTO {.branch = parts[0],
                                             .repository = parts[1],
                                             .architecture = parts[2]},
                          parts[3]);
}

std::string Database::serialize_key(PackageSectionDTO section,
                                    std::string name) {
    return fmt::format("{}/{}", std::string(section), name);
}

coro::task<Database::Result<void>>
    Database::add(const PackageSectionDTO section, const Package package) {
    auto desc = Utilities::AlpmDb::Desc::parse_package(package.filepath);

    if (!desc.has_value()) {
        co_return bxt::make_error_with_source<DatabaseError>(
            std::move(desc.error()), DatabaseError::ErrorType::InvalidArgument);
    }
    Box::Package package_entity {.name = package.name,
                                 .filepath = package.filepath,
                                 .signature_path = package.signature_path,
                                 .location = package.location,
                                 .description = std::move(*desc)};

    auto moved_path = m_manager.move_to(
        package_entity.filepath, package_entity.location, section.architecture);

    if (!moved_path.has_value()) {
        co_return bxt::make_error_with_source<DatabaseError>(
            std::move(moved_path.error()),
            DatabaseError::ErrorType::InvalidArgument);
    }
    package_entity.filepath = *moved_path;

    if (package_entity.signature_path.has_value()) {
        auto signature_moved_path =
            m_manager.move_to(*package_entity.signature_path,
                              package_entity.location, section.architecture);

        if (!signature_moved_path.has_value()) {
            co_return bxt::make_error_with_source<DatabaseError>(
                std::move(signature_moved_path.error()),
                DatabaseError::ErrorType::InvalidArgument);
        }
        package_entity.signature_path = *signature_moved_path;
    }

    auto result =
        co_await m_db.put(serialize_key(section, package.name), package_entity);

    if (!result.has_value()) {
        co_return bxt::make_error_with_source<DatabaseError>(
            std::move(result.error()),
            DatabaseError::ErrorType::InvalidArgument);
    }

    auto mark = m_archiver.mark_dirty_sections({section});
    co_await mark;

    co_return {};
}

coro::task<Database::Result<void>>
    bxt::Box::Database::remove(const PackageSectionDTO section,
                               const std::string name) {
    auto result = co_await m_db.del(serialize_key(section, name));

    if (!result.has_value()) {
        co_return bxt::make_error_with_source<DatabaseError>(
            std::move(result.error()),
            DatabaseError::ErrorType::InvalidArgument);
    }

    auto mark = m_archiver.mark_dirty_sections({section});
    co_await mark;

    co_return {};
}

coro::task<Database::Result<std::vector<Package>>>
    bxt::Box::Database::find_by_section(PackageSectionDTO section) {
    std::vector<Package> result;
    auto txn = co_await m_db.env()->begin_ro_txn();

    auto cursor = lmdb::cursor::open(txn->value, m_db.dbi());

    const auto prefix = std::string(section);

    std::string_view key = prefix;
    std::string_view value;

    if (cursor.get(key, value, MDB_SET_RANGE) && key.starts_with(prefix)) {
        do {
            const auto deserialized_value =
                decltype(m_db)::Serializer::deserialize(value);

            if (!deserialized_value.has_value()) {
                co_return bxt::make_error<DatabaseError>(
                    DatabaseError::ErrorType::InvalidEntityError);
            }

            result.emplace_back(*deserialized_value);

        } while (cursor.get(key, value, MDB_NEXT) && key.starts_with(prefix));
    }

    co_return result;
}
} // namespace bxt::Box