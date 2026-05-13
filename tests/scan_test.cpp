#include <cstdint>
#include <gtest/gtest.h>
#include <limits>
#include <string>
#include <string_view>
#include <tuple>

#include "scan.hpp"

template <typename T, typename Cb>
void call_with_cv(Cb &&cb) {
    cb.template operator()<T>();
    cb.template operator()<const T>();

    if constexpr (std::is_scalar_v<T>) {
        cb.template operator()<volatile T>();
        cb.template operator()<const volatile T>();
    }
}

template <typename... Ts, typename Cb>
void for_each_type(Cb &&cb) {
    (call_with_cv<Ts>(std::forward<Cb>(cb)), ...);
}

TEST(ScanTest, basic_multi_test_with_converison_specifiers) {

    auto result = stdx::scan<int, unsigned, const std::string, float, double, std::string_view>(
        "_ 1 _ 2 _ str1 _ 3.0 _ 4.0 _ str2 _", "_ {%d} _ {%u} _ {%s} _ {%f} _ {%f} _ {%s} _");

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(std::get<0>(result->value()), 1);
    ASSERT_EQ(std::get<1>(result->value()), 2);
    ASSERT_EQ(std::get<2>(result->value()), "str1");
    ASSERT_EQ(std::get<3>(result->value()), 3.0f);
    ASSERT_EQ(std::get<4>(result->value()), 4.0f);
    ASSERT_EQ(std::get<5>(result->value()), "str2");
}

TEST(ScanTest, basic_multi_test_without_converison_specifiers) {

    auto result = stdx::scan<int, unsigned, const std::string, float, double, std::string_view>(
        "_ 1 _ 2 _ str1 _ 3.0 _ 4.0 _ str2 _", "_ {} _ {} _ {} _ {} _ {} _ {} _");

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(std::get<0>(result->value()), 1);
    ASSERT_EQ(std::get<1>(result->value()), 2);
    ASSERT_EQ(std::get<2>(result->value()), "str1");
    ASSERT_EQ(std::get<3>(result->value()), 3.0f);
    ASSERT_EQ(std::get<4>(result->value()), 4.0f);
    ASSERT_EQ(std::get<5>(result->value()), "str2");
}

TEST(ScanTest, get_valid_floating_point) {

    for_each_type<float, double>([]<typename T>() {
        T standard = 1.01;

        auto result = stdx::scan<T>("_ 1.01 _", "_ {} _");

        ASSERT_TRUE(result.has_value());

        T final_val = std::get<0>(result->value());

        ASSERT_EQ(final_val, standard);
    });
}

TEST(ScanTest, get_valid_string) {

    for_each_type<std::string, std::string_view>([]<typename T>() {
        auto result = stdx::scan<T>("_ standard _", "_ {} _");

        ASSERT_TRUE(result.has_value());
        ASSERT_EQ(std::get<0>(result->value()), "standard");
    });
}

TEST(ScanTest, get_valid_integral_and_unsigned) {

    for_each_type<int, unsigned, int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t>(
        []<typename T>() {
            T standard = 123;

            auto result = stdx::scan<T>("_ 123 _", "_ {} _");

            ASSERT_TRUE(result.has_value());

            T final_val = std::get<0>(result->value());

            ASSERT_EQ(final_val, standard);
        });
}

TEST(ScanTest, incorrect_value_floating_point) {

    for_each_type<float, double>([]<typename T>() {
        auto result_str = stdx::scan<T>("_ str _", "_ {} _");

        ASSERT_FALSE(result_str.has_value());
        ASSERT_EQ(result_str.error().message, "Invalid floating point number.");

        std::string src;

        if constexpr (std::is_same_v<T, float>)
            src = "_ 1e1000 _";
        else
            src = "_ 1e10000 _";

        auto result_max = stdx::scan<T>(src, "_ {} _");

        ASSERT_FALSE(result_max.has_value());
        ASSERT_EQ(result_max.error().message, "Floating point value out of range.");

        auto result_unexpected_val = stdx::scan<T>("_ 1.0abc _", "_ {} _");

        ASSERT_FALSE(result_unexpected_val.has_value());
        ASSERT_EQ(result_unexpected_val.error().message, "Unexpected trailing characters.");
    });
}

TEST(ScanTest, incorrect_value_integral_and_unsigned) {

    for_each_type<int, unsigned, int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t>(
        []<typename T>() {
            auto result_str = stdx::scan<T>("_ str _", "_ {} _");

            ASSERT_FALSE(result_str.has_value());
            ASSERT_EQ(result_str.error().message, "Invalid integer.");

            T max_val = std::numeric_limits<T>::max();
            T tail = max_val % 10;
            T head = max_val / 10;

            std::string max = std::to_string(head) + std::to_string(tail + 1);
            std::string src = "_ " + max + " _";
            auto result_max = stdx::scan<T>(src, "_ {} _");

            ASSERT_FALSE(result_max.has_value());
            ASSERT_EQ(result_max.error().message, "Integer out of range.");

            auto result_unexpected_val = stdx::scan<T>("_ 123abc _", "_ {} _");

            ASSERT_FALSE(result_unexpected_val.has_value());
            ASSERT_EQ(result_unexpected_val.error().message, "Unexpected trailing characters.");
        });
}

TEST(ScanTest, format_mismatch_with_the_specifier_floating_point) {

    for_each_type<float, double>([]<typename T>() {
        auto result_u = stdx::scan<T>("_ 1.0 _", "_ {%u} _");

        ASSERT_FALSE(result_u.has_value());
        ASSERT_EQ(result_u.error().message, "The format does not match float num type.");

        auto result_d = stdx::scan<T>("_ 1.0 _", "_ {%d} _");

        ASSERT_FALSE(result_d.has_value());
        ASSERT_EQ(result_d.error().message, "The format does not match float num type.");

        auto result_s = stdx::scan<T>("_ 1.0 _", "_ {%s} _");

        ASSERT_FALSE(result_s.has_value());
        ASSERT_EQ(result_s.error().message, "The format does not match float num type.");
    });
}

TEST(ScanTest, format_mismatch_with_the_specifier_string) {

    for_each_type<std::string, std::string_view>([]<typename T>() {
        auto result_u = stdx::scan<T>("_ str _", "_ {%u} _");

        ASSERT_FALSE(result_u.has_value());
        ASSERT_EQ(result_u.error().message, "The format does not match string type.");

        auto result_d = stdx::scan<T>("_ str _", "_ {%d} _");

        ASSERT_FALSE(result_d.has_value());
        ASSERT_EQ(result_d.error().message, "The format does not match string type.");

        auto result_f = stdx::scan<T>("_ str _", "_ {%f} _");

        ASSERT_FALSE(result_f.has_value());
        ASSERT_EQ(result_f.error().message, "The format does not match string type.");
    });
}

TEST(ScanTest, format_mismatch_with_the_specifier_unsigned) {

    for_each_type<unsigned, uint8_t, uint16_t, uint32_t, uint64_t>([]<typename T>() {
        auto result_s = stdx::scan<T>("_ 1 _", "_ {%s} _");

        ASSERT_FALSE(result_s.has_value());
        ASSERT_EQ(result_s.error().message, "The format does not match unsigned type.");

        auto result_d = stdx::scan<T>("_ 1 _", "_ {%d} _");

        ASSERT_FALSE(result_d.has_value());
        ASSERT_EQ(result_d.error().message, "The format does not match unsigned type.");

        auto result_f = stdx::scan<T>("_ 1 _", "_ {%f} _");

        ASSERT_FALSE(result_f.has_value());
        ASSERT_EQ(result_f.error().message, "The format does not match unsigned type.");
    });
}

TEST(ScanTest, format_mismatch_with_the_specifier_integral) {

    for_each_type<int, int8_t, int16_t, int32_t, int64_t>([]<typename T>() {
        auto result_s = stdx::scan<T>("_ 1 _", "_ {%s} _");

        ASSERT_FALSE(result_s.has_value());
        ASSERT_EQ(result_s.error().message, "The format does not match integral type.");

        auto result_u = stdx::scan<T>("_ 1 _", "_ {%u} _");

        ASSERT_FALSE(result_u.has_value());
        ASSERT_EQ(result_u.error().message, "The format does not match integral type.");

        auto result_f = stdx::scan<T>("_ 1 _", "_ {%f} _");

        ASSERT_FALSE(result_f.has_value());
        ASSERT_EQ(result_f.error().message, "The format does not match integral type.");
    });
}

TEST(ScanTest, text_and_fmt_are_different) {

    auto result = stdx::scan<std::string>("val", "_ {} _");

    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error().message, "Unformatted text in input and format string are different");
}

TEST(ScanTest, diff_number_types_and_tempaltaes) {
    auto result_more = stdx::scan<int, int, int>("_ 1 _", "_ {} _");

    ASSERT_FALSE(result_more.has_value());
    ASSERT_EQ(result_more.error().message, "The number of types and the string template do not match.");

    auto result_less = stdx::scan<int>("_ 1 1 1 _", "_ {} {} {} _");

    ASSERT_FALSE(result_less.has_value());
    ASSERT_EQ(result_less.error().message, "The number of types and the string template do not match.");
}