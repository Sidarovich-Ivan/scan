#pragma once

#include "parse.hpp"
#include "types.hpp"

#include <cstddef>
#include <expected>
#include <tuple>
#include <utility>

namespace stdx {

template <typename... Ts, std::size_t... Is>
std::expected<std::tuple<Ts...>, details::scan_error> make_tuple(const details::parse_result &parsed_str,
                                                                 std::index_sequence<Is...>) {

    std::expected<std::tuple<Ts...>, details::scan_error> exp_res = std::tuple<Ts...>();

    auto process = [&]<std::size_t I>() -> bool {
        using T = std::tuple_element_t<I, std::tuple<Ts...>>;

        auto parsed_val = details::parse_value_with_format<T>(parsed_str.second[I], parsed_str.first[I]);

        if (!parsed_val) {
            exp_res = std::unexpected(parsed_val.error());
            return false;
        }

        std::get<I>(exp_res.value()) = std::move(parsed_val.value());
        return true;
    };

    (process.template operator()<Is>() && ...);

    return exp_res;
}

template <details::supported_type... Ts>
std::expected<details::scan_result<Ts...>, details::scan_error> scan(std::string_view input, std::string_view format) {

    auto parsed_str = details::parse_sources(input, format);

    if (!parsed_str)
        return std::unexpected(parsed_str.error());

    constexpr size_t ts_size = sizeof...(Ts);

    if (ts_size != parsed_str.value().first.size())
        return std::unexpected(details::scan_error("The number of types and the string template do not match."));

    auto res_tuple = make_tuple<std::remove_cvref_t<Ts>...>(parsed_str.value(), std::index_sequence_for<Ts...>{});

    if (!res_tuple)
        return std::unexpected(res_tuple.error());

    return details::scan_result<Ts...>{std::move(res_tuple.value())};

}  // namespace stdx

}  // namespace stdx