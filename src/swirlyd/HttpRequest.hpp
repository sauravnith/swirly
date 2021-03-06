/*
 * The Restful Matching-Engine.
 * Copyright (C) 2013, 2017 Swirly Cloud Limited.
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program; if
 * not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#ifndef SWIRLYD_HTTPREQUEST_HPP
#define SWIRLYD_HTTPREQUEST_HPP

#include <swirly/ws/HttpHandler.hpp>
#include <swirly/ws/RestBody.hpp>
#include <swirly/ws/Url.hpp>

namespace swirly {

class HttpRequest : public BasicUrl<HttpRequest> {
  public:
    HttpRequest() noexcept = default;
    ~HttpRequest() noexcept;

    // Copy.
    HttpRequest(const HttpRequest&) = delete;
    HttpRequest& operator=(const HttpRequest&) = delete;

    // Move.
    HttpRequest(HttpRequest&&) = delete;
    HttpRequest& operator=(HttpRequest&&) = delete;

    auto method() const noexcept { return method_; }
    auto url() const noexcept { return +url_; }
    auto accnt() const noexcept { return +accnt_; }
    auto perm() const noexcept { return +perm_; }
    auto time() const noexcept { return +time_; }
    const auto& body() const noexcept { return body_; }
    auto partial() const noexcept { return partial_; }
    void clear() noexcept
    {
        BasicUrl<HttpRequest>::reset();
        method_ = HttpMethod::Get;
        url_.clear();
        field_.clear();
        value_ = nullptr;

        accnt_.clear();
        perm_.clear();
        time_.clear();
        body_.reset();
        partial_ = false;
    }
    void flush() { BasicUrl<HttpRequest>::parse(); }
    void setMethod(HttpMethod method) noexcept { method_ = method; }
    void appendUrl(std::string_view sv) { url_ += sv; }
    void appendHeaderField(std::string_view sv, bool first)
    {
        if (first) {
            field_ = sv;
        } else {
            field_ += sv;
        }
    }
    void appendHeaderValue(std::string_view sv, bool first)
    {
        if (first) {
            if (field_ == "Swirly-Accnt"_sv) {
                value_ = &accnt_;
            } else if (field_ == "Swirly-Perm"_sv) {
                value_ = &perm_;
            } else if (field_ == "Swirly-Time"_sv) {
                value_ = &time_;
            } else {
                value_ = nullptr;
            }
        }
        if (value_) {
            value_->append(sv);
        }
    }
    void appendBody(std::string_view sv) { partial_ = !body_.parse(sv); }

  private:
    HttpMethod method_{HttpMethod::Get};
    String<128> url_;
    String<16> field_;
    String<24>* value_{nullptr};
    // Header fields.
    String<24> accnt_;
    String<24> perm_;
    String<24> time_;
    RestBody body_;
    bool partial_{false};
};

} // swirly

#endif // SWIRLYD_HTTPREQUEST_HPP
