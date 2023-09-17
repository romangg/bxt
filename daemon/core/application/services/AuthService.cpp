/* === This file is part of bxt ===
 *
 *   SPDX-FileCopyrightText: 2022 Artem Grinev <agrinev@manjaro.org>
 *   SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
#include "AuthService.h"

#include "core/application/errors/AuthError.h"
namespace bxt::Core::Application {

coro::task<AuthService::Result<void>>
    AuthService::auth(const std::string &name, const std::string &password) {
    auto entity = co_await m_user_repository.find_by_id_async(name);

    if (!entity.has_value()) {
        co_return bxt::make_error<AuthError>(
            AuthError::ErrorType::UserNotFound);
    }

    if (entity->password() != password) {
        co_return bxt::make_error<AuthError>(
            AuthError::ErrorType::InvalidCredentials);
    }

    co_return {};
}

coro::task<AuthService::Result<void>>
    AuthService::verify(const std::string &token) const {
}

} // namespace bxt::Core::Application
