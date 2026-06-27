#pragma once

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <type_traits>

#define STATIC_REQUIRE(expr) { \
static_assert(expr);           \
REQUIRE(expr);                 \
}

#define STATIC_REQUIRE_FALSE(expr) { \
static_assert(!(expr));              \
REQUIRE_FALSE(expr);                 \
}

#define R_CHECK_EQ(L_expr, R_expr) {                             \
const auto L_run = (L_expr);                                     \
const auto R_run = (R_expr);                                     \
static_assert(std::is_same_v<decltype(L_run), decltype(R_run)>); \
CHECK_EQ(L_run, R_run);                                          \
}

#define RAC_CHECK_EQ(L_expr, R_expr) {                             \
static constexpr auto L_comp = (L_expr);                           \
static constexpr auto R_comp = (R_expr);                           \
static_assert(std::is_same_v<decltype(L_comp), decltype(R_comp)>); \
static_assert(L_comp == R_comp);                                   \
R_CHECK_EQ(L_expr, R_expr);                                        \
}

#define BASIC_R_SUBCASE(name, L_expr, R_expr) \
SUBCASE(name) { R_CHECK_EQ(L_expr, R_expr); }

#define BASIC_R_SUBCASE_2(name, L_expr1, R_expr1, L_expr2, R_expr2) \
SUBCASE(name) { R_CHECK_EQ(L_expr1, R_expr1); R_CHECK_EQ(L_expr2, R_expr2); }

#define BASIC_RAC_SUBCASE(name, L_expr, R_expr) \
SUBCASE(name) { RAC_CHECK_EQ(L_expr, R_expr); }

#define BASIC_RAC_SUBCASE_2(name, L_expr1, R_expr1, L_expr2, R_expr2) \
SUBCASE(name) { RAC_CHECK_EQ(L_expr1, R_expr1); RAC_CHECK_EQ(L_expr2, R_expr2); }
