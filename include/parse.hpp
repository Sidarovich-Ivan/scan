#pragma once

#include <concepts>
#include <expected>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "types.hpp"

namespace stdx::details {

// default
template <typename T>
std::expected<T, scan_error> parse_value(std::string_view input) {
    return std::unexpected(scan_error("Unsupported type."));
};

template <std::integral T>
std::expected<T, scan_error> parse_value(std::string_view input) {
    T value{};

    auto [ptr, ec] = std::from_chars(input.data(), input.data() + input.size(), value);

    if (ec == std::errc::invalid_argument)
        return std::unexpected(scan_error("Invalid integer."));

    if (ec == std::errc::result_out_of_range)
        return std::unexpected(scan_error("Integer out of range."));

    if (ptr != input.data() + input.size())
        return std::unexpected(scan_error("Unexpected trailing characters."));

    return value;
};

template <details::string_type T>
std::expected<T, scan_error> parse_value(std::string_view input) {
    if constexpr (std::same_as<T, std::string>)
        return std::string(input);

    else
        return input;
};

template <std::floating_point T>
std::expected<T, scan_error> parse_value(std::string_view input) {
    T value{};

    auto [ptr, ec] = std::from_chars(input.data(), input.data() + input.size(), value);

    if (ec == std::errc::invalid_argument)
        return std::unexpected(scan_error("Invalid floating point number."));

    if (ec == std::errc::result_out_of_range)
        return std::unexpected(scan_error("Floating point value out of range."));

    if (ptr != input.data() + input.size())
        return std::unexpected(scan_error("Unexpected trailing characters."));

    return value;
};

template <typename T>
std::expected<T, scan_error> parse_value_with_format(std::string_view input, std::string_view fmt) {

    if (fmt.empty())
        return parse_value<T>(input);

    if constexpr (std::unsigned_integral<T>) {

        if (fmt != "%u")
            return std::unexpected(scan_error("The format does not match unsigned type."));
    }

    else if constexpr (std::integral<T>) {

        if (fmt != "%d")
            return std::unexpected(scan_error("The format does not match integral type."));
    }

    else if constexpr (details::string_type<T>) {

        if (fmt != "%s")
            return std::unexpected(scan_error("The format does not match string type."));
    }

    else if constexpr (std::floating_point<T>) {

        if (fmt != "%f")
            return std::unexpected(scan_error("The format does not match float num type."));
    }

    return parse_value<T>(input);
}

template <typename... Ts>
std::expected<parse_result, scan_error> parse_sources(std::string_view input, std::string_view format) {
    std::vector<std::string_view> format_parts;  // Части формата между {}
    std::vector<std::string_view> input_parts;
    size_t start = 0;
    while (true) {
        size_t open = format.find('{', start);
        if (open == std::string_view::npos) {
            break;
        }
        size_t close = format.find('}', open);
        if (close == std::string_view::npos) {
            break;
        }

        // Если между предыдущей } и текущей { есть текст,
        // проверяем его наличие во входной строке
        if (open > start) {
            std::string_view between = format.substr(start, open - start);
            auto pos = input.find(between);
            if (input.size() < between.size() || pos == std::string_view::npos) {
                return std::unexpected(scan_error{"Unformatted text in input and format string are different"});
            }
            if (start != 0) {
                input_parts.emplace_back(input.substr(0, pos));
            }

            input = input.substr(pos + between.size());
        }

        // Сохраняем спецификатор формата (то, что между {})
        format_parts.push_back(format.substr(open + 1, close - open - 1));
        start = close + 1;
    }

    // Проверяем оставшийся текст после последней }
    if (start < format.size()) {
        std::string_view remaining_format = format.substr(start);
        auto pos = input.find(remaining_format);
        if (input.size() < remaining_format.size() || pos == std::string_view::npos) {
            return std::unexpected(scan_error{"Unformatted text in input and format string are different"});
        }
        input_parts.emplace_back(input.substr(0, pos));
        input = input.substr(pos + remaining_format.size());
    } else {
        input_parts.emplace_back(input);
    }
    return std::pair{format_parts, input_parts};
}

}  // namespace stdx::details