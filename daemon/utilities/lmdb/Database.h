/* === This file is part of bxt ===
 *
 *   SPDX-FileCopyrightText: 2023 Artem Grinev <agrinev@manjaro.org>
 *   SPDX-License-Identifier: AGPL-3.0-or-later
 *
 */
#pragma once
#include "core/application/errors/CrudError.h"
#include "coro/sync_wait.hpp"
#include "Environment.h"
#include "lmdb.h"
#include "utilities/Error.h"
#include "utilities/errors/DatabaseError.h"
#include "utilities/errors/Macro.h"
#include "utilities/lmdb/CerealSerializer.h"
#include "utilities/lmdb/Error.h"
#include "utilities/log/Logging.h"
#include "utilities/NavigationAction.h"

#include <exception>
#include <lmdbxx/lmdb++.h>
#include <string_view>
namespace bxt::Utilities::LMDB {

template<typename TEntity, typename TSerializer = CerealSerializer<TEntity>> class Database {
public:
    using Serializer = TSerializer;
    BXT_DECLARE_RESULT(bxt::DatabaseError)

    Database(std::shared_ptr<Environment> env, std::string_view name = "")
        : m_env(env) {
        auto txn = coro::sync_wait(m_env->begin_rw_txn());

        m_dbi = name.empty() ? lmdb::dbi::open(txn->value, nullptr, MDB_CREATE)
                             : lmdb::dbi::open(txn->value, name, MDB_CREATE);

        txn->value.commit();
    }
    coro::task<Result<bool>> put(lmdb::txn& txn, std::string_view key, TEntity const value) {
        bool result;
        try {
            auto value_string = TSerializer::serialize(value);

            if (!value_string.has_value()) {
                co_return bxt::make_error_with_source<DatabaseError>(
                    std::move(value_string.error()),
                    DatabaseError::ErrorType::DatabaseMalformedError);
            }

            result = m_dbi.put(txn, key, *value_string);

        } catch (lmdb::error const& err) {
            loge("LMDB::Database::put: {}", err.what());
            co_return bxt::make_error_with_source<DatabaseError>(
                LMDB::Error(std::move(err)), DatabaseError::ErrorType::DatabaseMalformedError);
        } catch (std::exception const& e) {
            co_return bxt::make_error<DatabaseError>(
                DatabaseError::ErrorType::DatabaseMalformedError);
        }

        co_return result;
    }

    coro::task<Result<bool>> del(lmdb::txn& txn, std::string_view key) {
        bool result;
        try {
            result = m_dbi.del(txn, key);

        } catch (lmdb::error const& err) {
            co_return bxt::make_error_with_source<DatabaseError>(
                LMDB::Error(std::move(err)), DatabaseError::ErrorType::DatabaseMalformedError);
        } catch (std::exception const& e) {
            co_return bxt::make_error<DatabaseError>(
                DatabaseError::ErrorType::DatabaseMalformedError);
        }

        co_return result;
    }

    coro::task<Result<TEntity>> get(lmdb::txn& txn, std::string_view key) {
        try {
            std::string_view value_string;

            if (!m_dbi.get(txn, key, value_string)) {
                co_return bxt::make_error<DatabaseError>(DatabaseError::ErrorType::EntityNotFound);
            }

            auto result = TSerializer::deserialize(std::string(value_string));

            if (!result.has_value()) {
                co_return bxt::make_error_with_source<DatabaseError>(
                    std::move(result.error()), DatabaseError::ErrorType::DatabaseMalformedError);
            }

            co_return *result;

        } catch (lmdb::error const& err) {
            co_return bxt::make_error_with_source<DatabaseError>(
                LMDB::Error(std::move(err)), DatabaseError::ErrorType::DatabaseMalformedError);
        } catch (std::exception const& e) {
            co_return bxt::make_error<DatabaseError>(
                DatabaseError::ErrorType::DatabaseMalformedError);
        }
    }

    coro::task<Result<void>>
        accept(lmdb::txn& txn,
               std::function<NavigationAction(std::string_view key, TEntity const& value)> visitor,
               std::string_view prefix = "") {
        {
            auto cursor = lmdb::cursor::open(txn, m_dbi);

            std::string_view key = prefix;
            std::string_view value;

            MDB_cursor_op operation = prefix.empty() ? MDB_FIRST : MDB_SET_RANGE;

            // If operation == MDB_SET_RANGE cursor.get will return the first
            // value >= key. We need to check the key to actually have our
            // prefix otherwise there are no found values
            if (!cursor.get(key, value, operation) || !key.starts_with(prefix)) {
                co_return {};
            }

            do {
                auto res = TSerializer::deserialize(std::string(value));

                if (!res.has_value()) {
                    co_return bxt::make_error_with_source<DatabaseError>(
                        std::move(res.error()), DatabaseError::ErrorType::InvalidEntityError);
                }

                auto const result = visitor(key, *res);

                switch (result) {
                case NavigationAction::Next:
                    operation = MDB_NEXT;
                    break;
                case NavigationAction::Previous:
                    operation = MDB_PREV;
                    break;
                case NavigationAction::Stop:
                    co_return {};
                }

            } while (cursor.get(key, value, operation) && key.starts_with(prefix));
        }

        co_return {};
    }

    lmdb::dbi& dbi() {
        return m_dbi;
    }

    std::shared_ptr<Environment> env() {
        return m_env;
    };

private:
    std::shared_ptr<Environment> m_env;
    lmdb::dbi m_dbi;
};

} // namespace bxt::Utilities::LMDB
