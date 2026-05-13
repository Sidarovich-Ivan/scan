#pragma once

#include <concepts>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace stdx::details {

struct scan_error {
    std::string message;
};

template <typename... Ts>
struct scan_result {
    std::tuple<Ts...> res;

    std::tuple<Ts...> &value() { return res; }
};

using parse_result = std::pair<std::vector<std::string_view>, std::vector<std::string_view>>;

template <typename T>
concept string_type = std::same_as<T, std::string> || std::same_as<T, std::string_view>;

template <typename T>
concept valid_type = !std::same_as<T, bool> && (std::integral<T> || std::floating_point<T> || string_type<T>);

template <typename T>
concept supported_type = !std::is_reference_v<T> && valid_type<std::remove_cv_t<T>>;

}  // namespace stdx::details
