#include "../tag/tag.hpp"
#include "../tag/time_tag.hpp"
#include "../string/String.hpp"

#include "gtest_helpers.hpp"
#include <gtest/gtest.h>

#include <unordered_map>
#include <tuple>

static_assert(SPRAWL_TAG("TEST")::EqualTo<SPRAWL_TAG("TEST")>(), "not equal");
static_assert(SPRAWL_TAG("TEST")::EqualTo("TEST"), "not equal");

static_assert(!SPRAWL_TAG("TEST")::EqualTo<SPRAWL_TAG("TESTA")>(), "invalidly equal");
static_assert(!SPRAWL_TAG("TEST")::EqualTo("TESTA"), "invalidly equal");
static_assert(!SPRAWL_TAG("TESTA")::EqualTo<SPRAWL_TAG("TEST")>(), "invalidly equal");
static_assert(!SPRAWL_TAG("TESTA")::EqualTo("TEST"), "invalidly equal");
static_assert(!SPRAWL_TAG("TEST")::EqualTo<SPRAWL_TAG("TES_")>(), "invalidly equal");
static_assert(!SPRAWL_TAG("TEST")::EqualTo("TES_"), "invalidly equal");

static_assert(SPRAWL_TAG("Hello")::Append<SPRAWL_TAG(" World!")>::EqualTo<SPRAWL_TAG("Hello World!")>(), "append failed");
static_assert(!SPRAWL_TAG("Hello")::Append<SPRAWL_TAG(" World!")>::EqualTo<SPRAWL_TAG("Hello House!")>(), "append failed");
static_assert(SPRAWL_TAG("Hello World")::AppendChar<'!'>::EqualTo<SPRAWL_TAG("Hello World!")>(), "append failed");
static_assert(SPRAWL_TAG("HeLlO")::Transform<sprawl::Upper>::EqualTo<SPRAWL_TAG("HELLO")>(), "Upper failed");
static_assert(SPRAWL_TAG("HeLlO")::Transform<sprawl::Lower>::EqualTo<SPRAWL_TAG("hello")>(), "Lower failed");
static_assert(SPRAWL_TAG("HeLlO")::Transform<sprawl::Capitalize>::EqualTo<SPRAWL_TAG("Hello")>(), "Capitalize failed");
static_assert(SPRAWL_TAG("HeLlO")::Transform<sprawl::SwapCase>::EqualTo<SPRAWL_TAG("hElLo")>(), "SwapCase failed");
static_assert(SPRAWL_TAG("hElLo WoRlD!")::Transform<sprawl::Title>::EqualTo<SPRAWL_TAG("Hello World!")>(), "Title failed");

static_assert(SPRAWL_TAG("Hello World!")::Substring<0, 5>::EqualTo<SPRAWL_TAG("Hello")>(), "Substring failed.");
static_assert(SPRAWL_TAG("Hello World!")::Substring<6, 6>::EqualTo<SPRAWL_TAG("World!")>(), "Substring failed.");
static_assert(SPRAWL_TAG("Hello World!")::Substring<0, 0>::EqualTo(""), "Substring failed");

static_assert(SPRAWL_TAG("Hello World!")::StartsWith<SPRAWL_TAG("Hello")>(), "StartsWith failed.");
static_assert(SPRAWL_TAG("Hello World!")::StartsWith("Hello"), "StartsWith failed.");
static_assert(SPRAWL_TAG("Hello World!")::EndsWith<SPRAWL_TAG("World!")>(), "EndsWith failed.");
static_assert(SPRAWL_TAG("Hello World!")::EndsWith("World!"), "EndsWith failed.");
static_assert(!SPRAWL_TAG("Hello World!")::StartsWith<SPRAWL_TAG("World!")>(), "StartsWith failed.");
static_assert(!SPRAWL_TAG("Hello World!")::StartsWith("World!"), "StartsWith failed.");
static_assert(!SPRAWL_TAG("Hello World!")::EndsWith<SPRAWL_TAG("Hello")>(), "EndsWith failed.");
static_assert(!SPRAWL_TAG("Hello World!")::EndsWith("Hello"), "EndsWith failed.");

static_assert(SPRAWL_TAG("Hello World!")::StartsWith<SPRAWL_TAG("World!"), 6>(), "StartsWith failed.");
static_assert(SPRAWL_TAG("Hello World!")::StartsWith("World!", 6), "StartsWith failed.");
static_assert(SPRAWL_TAG("Hello World!")::EndsWith<SPRAWL_TAG("Hello"), 0, 5>(), "EndsWith failed.");
static_assert(SPRAWL_TAG("Hello World!")::EndsWith("Hello", 0, 5), "EndsWith failed.");

static_assert(SPRAWL_TAG("Hello World!")::StartsWith<SPRAWL_TAG("o Wor"), 4, 9>(), "StartsWith failed.");
static_assert(SPRAWL_TAG("Hello World!")::StartsWith("o Wor", 4, 9), "StartsWith failed.");
static_assert(SPRAWL_TAG("Hello World!")::EndsWith<SPRAWL_TAG("o Wor"), 4, 9>(), "EndsWith failed.");
static_assert(SPRAWL_TAG("Hello World!")::EndsWith("o Wor", 4, 9), "EndsWith failed.");

static_assert(!SPRAWL_TAG("Hello World!")::StartsWith<SPRAWL_TAG("o Wor"), 5, 9>(), "StartsWith failed.");
static_assert(!SPRAWL_TAG("Hello World!")::StartsWith("o Wor", 5, 9), "StartsWith failed.");
static_assert(!SPRAWL_TAG("Hello World!")::EndsWith<SPRAWL_TAG("o Wor"), 5, 9>(), "EndsWith failed.");
static_assert(!SPRAWL_TAG("Hello World!")::EndsWith("o Wor", 5, 9), "EndsWith failed.");
static_assert(!SPRAWL_TAG("Hello World!")::StartsWith<SPRAWL_TAG("o Wor"), 4, 8>(), "StartsWith failed.");
static_assert(!SPRAWL_TAG("Hello World!")::StartsWith("o Wor", 4, 8), "StartsWith failed.");
static_assert(!SPRAWL_TAG("Hello World!")::EndsWith<SPRAWL_TAG("o Wor"), 4, 8>(), "EndsWith failed.");
static_assert(!SPRAWL_TAG("Hello World!")::EndsWith("o Wor", 4, 8), "EndsWith failed.");

static_assert(SPRAWL_TAG("Hello World!")::Find<SPRAWL_TAG("World")>() == 6, "Find failed");
static_assert(SPRAWL_TAG("Hello World!")::Find<SPRAWL_TAG("World!")>() == 6, "Find failed");
static_assert(SPRAWL_TAG("Hello World!")::Find<SPRAWL_TAG("W")>() == 6, "Find failed");
static_assert(SPRAWL_TAG("Hello World!")::Find<SPRAWL_TAG("Hello")>() == 0, "Find failed");
static_assert(SPRAWL_TAG("Hello World!")::Find<SPRAWL_TAG("H")>() == 0, "Find failed");
static_assert(SPRAWL_TAG("Hello World!")::Find<SPRAWL_TAG("!")>() == 11, "Find failed");
static_assert(SPRAWL_TAG("Hello World!")::Find<SPRAWL_TAG("Earth")>() == -1, "Find failed");
static_assert(SPRAWL_TAG("Hello Hello!")::Find<SPRAWL_TAG("Hello")>() == 0, "Find failed");
static_assert(SPRAWL_TAG("Hello Hello!")::Find<SPRAWL_TAG("Hello"), 1>() == 6, "Find failed");
static_assert(SPRAWL_TAG("Hello Hello!")::Find<SPRAWL_TAG("Hello"), 1, 10>() == -1, "Find failed");
static_assert(SPRAWL_TAG("Hello Hello!")::Find<SPRAWL_TAG("Hello"), 6, 11>() == 6, "Find failed");
static_assert(SPRAWL_TAG("Hello Hello!")::Find<SPRAWL_TAG("Hello"), 6>() == 6, "Find failed");
static_assert(SPRAWL_TAG("Hello Hello!")::Find<SPRAWL_TAG("Hello"), 6, 10>() == -1, "Find failed");
static_assert(SPRAWL_TAG("Hello World!")::Find<SPRAWL_TAG("!"), 6>() == 11, "Find failed");
static_assert(SPRAWL_TAG("Hello World!")::Find<SPRAWL_TAG("!"), 11>() == 11, "Find failed");
static_assert(SPRAWL_TAG("Hello World!")::Find<SPRAWL_TAG("!"), 6, 11>() == -1, "Find failed");
static_assert(SPRAWL_TAG("Hello World!")::Find<SPRAWL_TAG("!"), 12>() == -1, "Find failed");

static_assert(SPRAWL_TAG("Hello World!")::Find("World") == 6, "Find failed");
static_assert(SPRAWL_TAG("Hello World!")::Find("World!") == 6, "Find failed");
static_assert(SPRAWL_TAG("Hello World!")::Find("W") == 6, "Find failed");
static_assert(SPRAWL_TAG("Hello World!")::Find("Hello") == 0, "Find failed");
static_assert(SPRAWL_TAG("Hello World!")::Find("H") == 0, "Find failed");
static_assert(SPRAWL_TAG("Hello World!")::Find("!") == 11, "Find failed");
static_assert(SPRAWL_TAG("Hello World!")::Find("Earth") == -1, "Find failed");
static_assert(SPRAWL_TAG("Hello Hello!")::Find("Hello") == 0, "Find failed");
static_assert(SPRAWL_TAG("Hello Hello!")::Find("Hello", 1) == 6, "Find failed");
static_assert(SPRAWL_TAG("Hello Hello!")::Find("Hello", 1, 10) == -1, "Find failed");
static_assert(SPRAWL_TAG("Hello Hello!")::Find("Hello", 6, 11) == 6, "Find failed");
static_assert(SPRAWL_TAG("Hello Hello!")::Find("Hello", 6) == 6, "Find failed");
static_assert(SPRAWL_TAG("Hello Hello!")::Find("Hello", 6, 10) == -1, "Find failed");
static_assert(SPRAWL_TAG("Hello World!")::Find("!", 6) == 11, "Find failed");
static_assert(SPRAWL_TAG("Hello World!")::Find("!", 11) == 11, "Find failed");
static_assert(SPRAWL_TAG("Hello World!")::Find("!", 6, 11) == -1, "Find failed");
static_assert(SPRAWL_TAG("Hello World!")::Find("!", 12) == -1, "Find failed");

static_assert(SPRAWL_TAG("Hello World!")::RFind<SPRAWL_TAG("World")>() == 6, "RFind failed");
static_assert(SPRAWL_TAG("Hello World!")::RFind<SPRAWL_TAG("World!")>() == 6, "RFind failed");
static_assert(SPRAWL_TAG("Hello World!")::RFind<SPRAWL_TAG("W")>() == 6, "RFind failed");
static_assert(SPRAWL_TAG("Hello World!")::RFind<SPRAWL_TAG("Hello")>() == 0, "RFind failed");
static_assert(SPRAWL_TAG("Hello World!")::RFind<SPRAWL_TAG("H")>() == 0, "RFind failed");
static_assert(SPRAWL_TAG("Hello World!")::RFind<SPRAWL_TAG("!")>() == 11, "RFind failed");
static_assert(SPRAWL_TAG("Hello World!")::RFind<SPRAWL_TAG("Earth")>() == -1, "RFind failed");
static_assert(SPRAWL_TAG("Hello Hello!")::RFind<SPRAWL_TAG("Hello")>() == 6, "RFind failed");

static_assert(SPRAWL_TAG("Hello Hello!")::RFind<SPRAWL_TAG("Hello"), 0, 6>() == 0, "RFind failed");
static_assert(SPRAWL_TAG("Hello Hello!")::RFind<SPRAWL_TAG("Hello"), 1, 10>() == -1, "RFind failed");
static_assert(SPRAWL_TAG("Hello Hello!")::RFind<SPRAWL_TAG("Hello"), 0, 11>() == 6, "RFind failed");
static_assert(SPRAWL_TAG("Hello Hello!")::RFind<SPRAWL_TAG("Hello"), 0, 10>() == 0, "RFind failed");
static_assert(SPRAWL_TAG("Hello Hello!")::RFind<SPRAWL_TAG("Hello"), 0, 4>() == -1, "RFind failed");
static_assert(SPRAWL_TAG("Hello World!")::RFind<SPRAWL_TAG("H"), 0, 6>() == 0, "RFind failed");
static_assert(SPRAWL_TAG("Hello World!")::RFind<SPRAWL_TAG("H"), 0, 1>() == 0, "RFind failed");
static_assert(SPRAWL_TAG("Hello World!")::RFind<SPRAWL_TAG("H"), 0, 0>() == -1, "RFind failed");

static_assert(SPRAWL_TAG("Hello World!")::RFind("World") == 6, "RFind failed");
static_assert(SPRAWL_TAG("Hello World!")::RFind("World!") == 6, "RFind failed");
static_assert(SPRAWL_TAG("Hello World!")::RFind("W") == 6, "RFind failed");
static_assert(SPRAWL_TAG("Hello World!")::RFind("Hello") == 0, "RFind failed");
static_assert(SPRAWL_TAG("Hello World!")::RFind("H") == 0, "RFind failed");
static_assert(SPRAWL_TAG("Hello World!")::RFind("!") == 11, "RFind failed");
static_assert(SPRAWL_TAG("Hello World!")::RFind("Earth") == -1, "RFind failed");
static_assert(SPRAWL_TAG("Hello Hello!")::RFind("Hello") == 6, "RFind failed");

static_assert(SPRAWL_TAG("Hello Hello!")::RFind("Hello", 0, 6) == 0, "RFind failed");
static_assert(SPRAWL_TAG("Hello Hello!")::RFind("Hello", 1, 10) == -1, "RFind failed");
static_assert(SPRAWL_TAG("Hello Hello!")::RFind("Hello", 0, 11) == 6, "RFind failed");
static_assert(SPRAWL_TAG("Hello Hello!")::RFind("Hello", 0, 10) == 0, "RFind failed");
static_assert(SPRAWL_TAG("Hello Hello!")::RFind("Hello", 0, 4) == -1, "RFind failed");
static_assert(SPRAWL_TAG("Hello World!")::RFind("H", 0, 6) == 0, "RFind failed");
static_assert(SPRAWL_TAG("Hello World!")::RFind("H", 0, 1) == 0, "RFind failed");
static_assert(SPRAWL_TAG("Hello World!")::RFind("H", 0, 0) == -1, "RFind failed");

static_assert(SPRAWL_TAG("Hello World!")::Contains<SPRAWL_TAG("World")>(), "Contains failed");
static_assert(SPRAWL_TAG("Hello World!")::Contains<SPRAWL_TAG("World!")>(), "Contains failed");
static_assert(SPRAWL_TAG("Hello World!")::Contains<SPRAWL_TAG("W")>(), "Contains failed");
static_assert(SPRAWL_TAG("Hello World!")::Contains<SPRAWL_TAG("Hello")>(), "Contains failed");
static_assert(SPRAWL_TAG("Hello World!")::Contains<SPRAWL_TAG("H")>(), "Contains failed");
static_assert(SPRAWL_TAG("Hello World!")::Contains<SPRAWL_TAG("!")>(), "Contains failed");
static_assert(!SPRAWL_TAG("Hello World!")::Contains<SPRAWL_TAG("Earth")>(), "Contains failed");
static_assert(SPRAWL_TAG("Hello Hello!")::Contains<SPRAWL_TAG("Hello")>(), "Contains failed");

static_assert(SPRAWL_TAG("Hello World!")::Contains("World"), "Contains failed");
static_assert(SPRAWL_TAG("Hello World!")::Contains("World!"), "Contains failed");
static_assert(SPRAWL_TAG("Hello World!")::Contains("W"), "Contains failed");
static_assert(SPRAWL_TAG("Hello World!")::Contains("Hello"), "Contains failed");
static_assert(SPRAWL_TAG("Hello World!")::Contains("H"), "Contains failed");
static_assert(SPRAWL_TAG("Hello World!")::Contains("!"), "Contains failed");
static_assert(!SPRAWL_TAG("Hello World!")::Contains("Earth"), "Contains failed");
static_assert(SPRAWL_TAG("Hello Hello!")::Contains("Hello"), "Contains failed");

static_assert(SPRAWL_TAG("Hello World!")::Count<SPRAWL_TAG("Hello")>() == 1, "Count failed");
static_assert(SPRAWL_TAG("Hello World!")::Count<SPRAWL_TAG("World!")>() == 1, "Count failed");
static_assert(SPRAWL_TAG("Hello World!")::Count<SPRAWL_TAG("H")>() == 1, "Count failed");
static_assert(SPRAWL_TAG("Hello World!")::Count<SPRAWL_TAG("!")>() == 1, "Count failed");
static_assert(SPRAWL_TAG("Hello World!")::Count<SPRAWL_TAG("o")>() == 2, "Count failed");
static_assert(SPRAWL_TAG("Hello World!")::Count<SPRAWL_TAG("Hello"), 1>() == 0, "Count failed");
static_assert(SPRAWL_TAG("Hello World!")::Count<SPRAWL_TAG("World!"), 0, 10>() == 0, "Count failed");
static_assert(SPRAWL_TAG("HaHaHa")::Count<SPRAWL_TAG("HaHa")>() == 2, "Count failed");
static_assert(SPRAWL_TAG("HHHHHHHHHH")::Count<SPRAWL_TAG("H")>() == 10, "Count failed");
static_assert(SPRAWL_TAG("HHHHHHHHHH")::Count<SPRAWL_TAG("H"), 2, 4>() == 2, "Count failed");
static_assert(SPRAWL_TAG("Hello World!")::Count<SPRAWL_TAG("Goodbye")>() == 0, "Count failed");

static_assert(SPRAWL_TAG("Hello World!")::Count("Hello") == 1, "Count failed");
static_assert(SPRAWL_TAG("Hello World!")::Count("World!") == 1, "Count failed");
static_assert(SPRAWL_TAG("Hello World!")::Count("H") == 1, "Count failed");
static_assert(SPRAWL_TAG("Hello World!")::Count("!") == 1, "Count failed");
static_assert(SPRAWL_TAG("Hello World!")::Count("o") == 2, "Count failed");
static_assert(SPRAWL_TAG("Hello World!")::Count("Hello", 1) == 0, "Count failed");
static_assert(SPRAWL_TAG("Hello World!")::Count("World!", 0, 10) == 0, "Count failed");
static_assert(SPRAWL_TAG("HaHaHa")::Count("HaHa") == 2, "Count failed");
static_assert(SPRAWL_TAG("HHHHHHHHHH")::Count("H") == 10, "Count failed");
static_assert(SPRAWL_TAG("HHHHHHHHHH")::Count("H", 2, 4) == 2, "Count failed");
static_assert(SPRAWL_TAG("Hello World!")::Count("Goodbye") == 0, "Count failed");

static_assert(SPRAWL_TAG("helloworld")::IsAlnum(), "IsAlnum failed");
static_assert(SPRAWL_TAG("helloworld1")::IsAlnum(), "IsAlnum failed");
static_assert(SPRAWL_TAG("1234567890")::IsAlnum(), "IsAlnum failed");
static_assert(!SPRAWL_TAG("hello world")::IsAlnum(), "IsAlnum failed");
static_assert(SPRAWL_TAG("HelloWorld")::IsAlnum(), "IsAlnum failed");
static_assert(SPRAWL_TAG("helloworlD")::IsAlnum(), "IsAlnum failed");
static_assert(SPRAWL_TAG("HELLOWORLD")::IsAlnum(), "IsAlnum failed");
static_assert(SPRAWL_TAG("HELLOWORLD1")::IsAlnum(), "IsAlnum failed");
static_assert(!SPRAWL_TAG("HELLO WORLD")::IsAlnum(), "IsAlnum failed");
static_assert(SPRAWL_TAG("HelloWorld")::IsAlnum(), "IsAlnum failed");
static_assert(SPRAWL_TAG("HELLOWORLd")::IsAlnum(), "IsAlnum failed");
static_assert(!SPRAWL_TAG("")::IsAlnum(), "IsAlnum failed");

static_assert(SPRAWL_TAG("helloworld")::IsAlpha(), "IsAlpha failed");
static_assert(!SPRAWL_TAG("helloworld1")::IsAlpha(), "IsAlpha failed");
static_assert(!SPRAWL_TAG("1234567890")::IsAlpha(), "IsAlpha failed");
static_assert(!SPRAWL_TAG("hello world")::IsAlpha(), "IsAlpha failed");
static_assert(SPRAWL_TAG("HelloWorld")::IsAlpha(), "IsAlpha failed");
static_assert(SPRAWL_TAG("helloworlD")::IsAlpha(), "IsAlpha failed");
static_assert(SPRAWL_TAG("HELLOWORLD")::IsAlpha(), "IsAlpha failed");
static_assert(!SPRAWL_TAG("HELLOWORLD1")::IsAlpha(), "IsAlpha failed");
static_assert(!SPRAWL_TAG("HELLO WORLD")::IsAlpha(), "IsAlpha failed");
static_assert(SPRAWL_TAG("HelloWorld")::IsAlpha(), "IsAlpha failed");
static_assert(SPRAWL_TAG("HELLOWORLd")::IsAlpha(), "IsAlpha failed");
static_assert(!SPRAWL_TAG("")::IsAlpha(), "IsAlpha failed");

static_assert(SPRAWL_TAG("1234567890")::IsDigit(), "IsDigit failed");
static_assert(!SPRAWL_TAG("hello world")::IsDigit(), "IsDigit failed");
static_assert(!SPRAWL_TAG("HelloWorld")::IsDigit(), "IsDigit failed");
static_assert(!SPRAWL_TAG("helloworlD")::IsDigit(), "IsDigit failed");
static_assert(!SPRAWL_TAG("")::IsDigit(), "IsDigit failed");

static_assert(SPRAWL_TAG("helloworld")::IsLower(), "IsLower failed");
static_assert(!SPRAWL_TAG("helloworld1")::IsLower(), "IsLower failed");
static_assert(!SPRAWL_TAG("hello world")::IsLower(), "IsLower failed");
static_assert(!SPRAWL_TAG("HelloWorld")::IsLower(), "IsLower failed");
static_assert(!SPRAWL_TAG("helloworlD")::IsLower(), "IsLower failed");
static_assert(!SPRAWL_TAG("")::IsLower(), "IsLower failed");

static_assert(SPRAWL_TAG(" \t\n\r")::IsSpace(), "IsSpace failed");
static_assert(SPRAWL_TAG(" ")::IsSpace(), "IsSpace failed");
static_assert(!SPRAWL_TAG("Hello World")::IsSpace(), "IsSpace failed");
static_assert(!SPRAWL_TAG(" Hello ")::IsSpace(), "IsSpace failed");
static_assert(!SPRAWL_TAG("")::IsSpace(), "IsSpace failed");

static_assert(SPRAWL_TAG("HELLOWORLD")::IsUpper(), "IsUpper failed");
static_assert(!SPRAWL_TAG("HELLOWORLD1")::IsUpper(), "IsUpper failed");
static_assert(!SPRAWL_TAG("HELLO WORLD")::IsUpper(), "IsUpper failed");
static_assert(!SPRAWL_TAG("HelloWorld")::IsUpper(), "IsUpper failed");
static_assert(!SPRAWL_TAG("HELLOWORLd")::IsUpper(), "IsUpper failed");
static_assert(!SPRAWL_TAG("")::IsUpper(), "IsUpper failed");

static_assert(SPRAWL_TAG("Hello World!")::IsPrintable(), "IsPrintable failed");

static_assert(SPRAWL_TAG("Hello World!")::IsTitle(), "IsTitle failed");
static_assert(!SPRAWL_TAG("Hello world!")::IsTitle(), "IsTitle failed");
static_assert(!SPRAWL_TAG("hello World!")::IsTitle(), "IsTitle failed");
static_assert(!SPRAWL_TAG("Hello WorlD!")::IsTitle(), "IsTitle failed");

static_assert(SPRAWL_TAG("Hello World!")::Erase<5, 6>::EqualTo<SPRAWL_TAG("Hello!")>(), "Erase failed");
static_assert(SPRAWL_TAG("Hello World!")::Erase<11, 1>::EqualTo<SPRAWL_TAG("Hello World")>(), "Erase failed");
static_assert(SPRAWL_TAG("Hello World!")::Erase<1>::EqualTo<SPRAWL_TAG("H")>(), "Erase failed");
static_assert(SPRAWL_TAG("Hello World!")::Erase<0>::EqualTo<SPRAWL_TAG("")>(), "Erase failed");
static_assert(SPRAWL_TAG("Hello World!")::Erase<0, 6>::EqualTo<SPRAWL_TAG("World!")>(), "Erase failed");

static_assert(SPRAWL_TAG("Hello World!")::Substring<SPRAWL_TAG("Hello World!")::Find("World"), 5>::EqualTo("World"), "Find + Substring failed");

static_assert(SPRAWL_TAG("")::EqualTo<SPRAWL_TAG("")>(), "empty not equal");
static_assert(SPRAWL_TAG("")::Append<SPRAWL_TAG("Hello World!")>::EqualTo<SPRAWL_TAG("Hello World!")>(), "empty append failed");

static_assert(SPRAWL_TAG(" ")::Join<std::pair<SPRAWL_TAG("Hello"), SPRAWL_TAG("World!")>>::EqualTo<SPRAWL_TAG("Hello World!")>(), "join failed");
static_assert(SPRAWL_TAG("::")::Join<std::tuple<SPRAWL_TAG("sprawl"), SPRAWL_TAG("detail"), SPRAWL_TAG("join_test")>>::EqualTo<SPRAWL_TAG("sprawl::detail::join_test")>(), "join failed");
static_assert(SPRAWL_TAG(" ")::Join<sprawl::TypeList<SPRAWL_TAG("Hello"), SPRAWL_TAG("World!")>>::EqualTo<SPRAWL_TAG("Hello World!")>(), "join failed");
static_assert(SPRAWL_TAG("::")::Join<sprawl::TypeList<SPRAWL_TAG("sprawl"), SPRAWL_TAG("detail"), SPRAWL_TAG("join_test")>>::EqualTo<SPRAWL_TAG("sprawl::detail::join_test")>(), "join failed");

static_assert(SPRAWL_TAG("    Hello World!")::LStrip<>::EqualTo<SPRAWL_TAG("Hello World!")>(), "lstrip failed");
static_assert(SPRAWL_TAG("\n \t Hello World!")::LStrip<>::EqualTo<SPRAWL_TAG("Hello World!")>(), "lstrip failed");
static_assert(SPRAWL_TAG("")::LStrip<>::EqualTo<SPRAWL_TAG("")>(), "lstrip failed");
static_assert(SPRAWL_TAG("Hello World!    ")::RStrip<>::EqualTo<SPRAWL_TAG("Hello World!")>(), "rstrip failed");
static_assert(SPRAWL_TAG("Hello World!\n \t ")::RStrip<>::EqualTo<SPRAWL_TAG("Hello World!")>(), "rstrip failed");
static_assert(SPRAWL_TAG("")::RStrip<>::EqualTo<SPRAWL_TAG("")>(), "lstrip failed");
static_assert(SPRAWL_TAG("    Hello World!    ")::Strip<>::EqualTo<SPRAWL_TAG("Hello World!")>(), "strip failed");
static_assert(SPRAWL_TAG("\n \t Hello World!\n \t ")::Strip<>::EqualTo<SPRAWL_TAG("Hello World!")>(), "strip failed");
static_assert(SPRAWL_TAG("")::Strip<>::EqualTo<SPRAWL_TAG("")>(), "lstrip failed");

static_assert(SPRAWL_TAG("1234Hello World!")::LStrip<sprawl::IsDigit>::EqualTo<SPRAWL_TAG("Hello World!")>(), "lStrip failed");
static_assert(SPRAWL_TAG("abcdHello World!")::LStrip<sprawl::CharIn<'a', 'b', 'c', 'd'>>::EqualTo<SPRAWL_TAG("Hello World!")>(), "lStrip failed");
static_assert(SPRAWL_TAG("")::LStrip<sprawl::IsDigit>::EqualTo<SPRAWL_TAG("")>(), "lstrip failed");
static_assert(SPRAWL_TAG("Hello World!1234")::RStrip<sprawl::IsDigit>::EqualTo<SPRAWL_TAG("Hello World!")>(), "rStrip failed");
static_assert(SPRAWL_TAG("Hello World!abcd")::RStrip<sprawl::CharIn<'a', 'b', 'c', 'd'>>::EqualTo<SPRAWL_TAG("Hello World!")>(), "rStrip failed");
static_assert(SPRAWL_TAG("")::RStrip<sprawl::IsDigit>::EqualTo<SPRAWL_TAG("")>(), "lstrip failed");
static_assert(SPRAWL_TAG("1234Hello World!1234")::Strip<sprawl::IsDigit>::EqualTo<SPRAWL_TAG("Hello World!")>(), "Strip failed");
static_assert(SPRAWL_TAG("abcdHello World!abcd")::Strip<sprawl::CharIn<'a', 'b', 'c', 'd'>>::EqualTo<SPRAWL_TAG("Hello World!")>(), "Strip failed");
static_assert(SPRAWL_TAG("")::Strip<sprawl::IsDigit>::EqualTo<SPRAWL_TAG("")>(), "lstrip failed");

static_assert(SPRAWL_TAG("Hello Planet!")::Replace<SPRAWL_TAG("Planet"), SPRAWL_TAG("World")>::EqualTo<SPRAWL_TAG("Hello World!")>(), "replace failed");
static_assert(SPRAWL_TAG("Ha Ha Ha!")::Replace<SPRAWL_TAG("Ha"), SPRAWL_TAG("Ho")>::EqualTo<SPRAWL_TAG("Ho Ho Ho!")>(), "replace failed");
static_assert(SPRAWL_TAG("Ha Ha Ha!")::Replace<SPRAWL_TAG("Ha"), SPRAWL_TAG("Ho"), 2>::EqualTo<SPRAWL_TAG("Ho Ho Ha!")>(), "replace failed");
static_assert(SPRAWL_TAG("Ha Ha Ha!")::Replace<SPRAWL_TAG("Ha"), SPRAWL_TAG("Ho"), 1>::EqualTo<SPRAWL_TAG("Ho Ha Ha!")>(), "replace failed");
static_assert(SPRAWL_TAG("HaHaHa!")::Replace<SPRAWL_TAG("HaHa"), SPRAWL_TAG("HoHo")>::EqualTo<SPRAWL_TAG("HoHoHa!")>(), "replace failed");

static_assert(SPRAWL_TAG("Hello Planet!")::RReplace<SPRAWL_TAG("Planet"), SPRAWL_TAG("World")>::EqualTo<SPRAWL_TAG("Hello World!")>(), "RReplace failed");
static_assert(SPRAWL_TAG("Ha Ha Ha!")::RReplace<SPRAWL_TAG("Ha"), SPRAWL_TAG("Ho")>::EqualTo<SPRAWL_TAG("Ho Ho Ho!")>(), "RReplace failed");
static_assert(SPRAWL_TAG("Ha Ha Ha!")::RReplace<SPRAWL_TAG("Ha"), SPRAWL_TAG("Ho"), 2>::EqualTo<SPRAWL_TAG("Ha Ho Ho!")>(), "RReplace failed");
static_assert(SPRAWL_TAG("Ha Ha Ha!")::RReplace<SPRAWL_TAG("Ha"), SPRAWL_TAG("Ho"), 1>::EqualTo<SPRAWL_TAG("Ha Ha Ho!")>(), "RReplace failed");
static_assert(SPRAWL_TAG("HaHaHa!")::RReplace<SPRAWL_TAG("HaHa"), SPRAWL_TAG("HoHo")>::EqualTo<SPRAWL_TAG("HaHoHo!")>(), "RReplace failed");

static_assert(SPRAWL_TAG("Hello Planet!")::ReplaceAt<6, 6, SPRAWL_TAG("World")>::EqualTo<SPRAWL_TAG("Hello World!")>(), "ReplaceAt failed");

static_assert(std::is_same<sprawl::TypeList<int, bool, char>::Get<0>, int>::value, "TypeList fail");
static_assert(std::is_same<sprawl::TypeList<int>::Get<0>, int>::value, "TypeList fail");
static_assert(std::is_same<sprawl::TypeList<int, bool, char>::Get<1>, bool>::value, "TypeList fail");
static_assert(std::is_same<sprawl::TypeList<int, bool, char>::Get<2>, char>::value, "TypeList fail");
static_assert(sprawl::TypeList<int, bool, char>::size == 3, "TypeList fail");

static_assert(SPRAWL_TAG("Hello World!")::Partition<SPRAWL_TAG(" ")>::size == 3, "Partition fail");
static_assert(SPRAWL_TAG("Hello World!")::Partition<SPRAWL_TAG(" ")>::Get<0>::EqualTo("Hello"), "Partition fail");
static_assert(SPRAWL_TAG("Hello World!")::Partition<SPRAWL_TAG(" ")>::Get<1>::EqualTo(" "), "Partition fail");
static_assert(SPRAWL_TAG("Hello World!")::Partition<SPRAWL_TAG(" ")>::Get<2>::EqualTo("World!"), "Partition fail");

static_assert(SPRAWL_TAG("Hello Again World!")::Partition<SPRAWL_TAG(" ")>::size == 3, "Partition fail");
static_assert(SPRAWL_TAG("Hello Again World!")::Partition<SPRAWL_TAG(" ")>::Get<0>::EqualTo("Hello"), "Partition fail");
static_assert(SPRAWL_TAG("Hello Again World!")::Partition<SPRAWL_TAG(" ")>::Get<1>::EqualTo(" "), "Partition fail");
static_assert(SPRAWL_TAG("Hello Again World!")::Partition<SPRAWL_TAG(" ")>::Get<2>::EqualTo("Again World!"), "Partition fail");

static_assert(SPRAWL_TAG("HelloWorld!")::Partition<SPRAWL_TAG(" ")>::size == 3, "Partition fail");
static_assert(SPRAWL_TAG("HelloWorld!")::Partition<SPRAWL_TAG(" ")>::Get<0>::EqualTo("HelloWorld!"), "Partition fail");
static_assert(SPRAWL_TAG("HelloWorld!")::Partition<SPRAWL_TAG(" ")>::Get<1>::EqualTo(""), "Partition fail");
static_assert(SPRAWL_TAG("HelloWorld!")::Partition<SPRAWL_TAG(" ")>::Get<2>::EqualTo(""), "Partition fail");

static_assert(SPRAWL_TAG("Hello World!")::RPartition<SPRAWL_TAG(" ")>::size == 3, "RPartition fail");
static_assert(SPRAWL_TAG("Hello World!")::RPartition<SPRAWL_TAG(" ")>::Get<0>::EqualTo("Hello"), "RPartition fail");
static_assert(SPRAWL_TAG("Hello World!")::RPartition<SPRAWL_TAG(" ")>::Get<1>::EqualTo(" "), "RPartition fail");
static_assert(SPRAWL_TAG("Hello World!")::RPartition<SPRAWL_TAG(" ")>::Get<2>::EqualTo("World!"), "RPartition fail");

static_assert(SPRAWL_TAG("Hello Again World!")::RPartition<SPRAWL_TAG(" ")>::size == 3, "RPartition fail");
static_assert(SPRAWL_TAG("Hello Again World!")::RPartition<SPRAWL_TAG(" ")>::Get<0>::EqualTo("Hello Again"), "RPartition fail");
static_assert(SPRAWL_TAG("Hello Again World!")::RPartition<SPRAWL_TAG(" ")>::Get<1>::EqualTo(" "), "RPartition fail");
static_assert(SPRAWL_TAG("Hello Again World!")::RPartition<SPRAWL_TAG(" ")>::Get<2>::EqualTo("World!"), "RPartition fail");

static_assert(SPRAWL_TAG("HelloWorld!")::RPartition<SPRAWL_TAG(" ")>::size == 3, "RPartition fail");
static_assert(SPRAWL_TAG("HelloWorld!")::RPartition<SPRAWL_TAG(" ")>::Get<0>::EqualTo("HelloWorld!"), "RPartition fail");
static_assert(SPRAWL_TAG("HelloWorld!")::RPartition<SPRAWL_TAG(" ")>::Get<1>::EqualTo(""), "RPartition fail");
static_assert(SPRAWL_TAG("HelloWorld!")::RPartition<SPRAWL_TAG(" ")>::Get<2>::EqualTo(""), "RPartition fail");

static_assert(SPRAWL_TAG("Hello World!")::Split<SPRAWL_TAG(" ")>::size == 2, "Split fail");
static_assert(SPRAWL_TAG("Hello World!")::Split<SPRAWL_TAG(" ")>::Get<0>::EqualTo("Hello"), "Split fail");
static_assert(SPRAWL_TAG("Hello World!")::Split<SPRAWL_TAG(" ")>::Get<1>::EqualTo("World!"), "Split fail");

static_assert(SPRAWL_TAG("Hello Again World!")::Split<SPRAWL_TAG(" ")>::size == 3, "Split fail");
static_assert(SPRAWL_TAG("Hello Again World!")::Split<SPRAWL_TAG(" ")>::Get<0>::EqualTo("Hello"), "Split fail");
static_assert(SPRAWL_TAG("Hello Again World!")::Split<SPRAWL_TAG(" ")>::Get<1>::EqualTo("Again"), "Split fail");
static_assert(SPRAWL_TAG("Hello Again World!")::Split<SPRAWL_TAG(" ")>::Get<2>::EqualTo("World!"), "Split fail");

static_assert(SPRAWL_TAG("Hello Again World!")::Split<SPRAWL_TAG(" "), 1>::size == 2, "Split fail");
static_assert(SPRAWL_TAG("Hello Again World!")::Split<SPRAWL_TAG(" "), 1>::Get<0>::EqualTo("Hello"), "Split fail");
static_assert(SPRAWL_TAG("Hello Again World!")::Split<SPRAWL_TAG(" "), 1>::Get<1>::EqualTo("Again World!"), "Split fail");

static_assert(SPRAWL_TAG("HelloWorld!")::Split<SPRAWL_TAG(" ")>::size == 1, "Split fail");
static_assert(SPRAWL_TAG("HelloWorld!")::Split<SPRAWL_TAG(" ")>::Get<0>::EqualTo("HelloWorld!"), "Split fail");

static_assert(SPRAWL_TAG("Hello World!")::RSplit<SPRAWL_TAG(" ")>::size == 2, "RSplit fail");
static_assert(SPRAWL_TAG("Hello World!")::RSplit<SPRAWL_TAG(" ")>::Get<0>::EqualTo("Hello"), "RSplit fail");
static_assert(SPRAWL_TAG("Hello World!")::RSplit<SPRAWL_TAG(" ")>::Get<1>::EqualTo("World!"), "RSplit fail");

static_assert(SPRAWL_TAG("Hello Again World!")::RSplit<SPRAWL_TAG(" ")>::size == 3, "RSplit fail");
static_assert(SPRAWL_TAG("Hello Again World!")::RSplit<SPRAWL_TAG(" ")>::Get<0>::EqualTo("Hello"), "RSplit fail");
static_assert(SPRAWL_TAG("Hello Again World!")::RSplit<SPRAWL_TAG(" ")>::Get<1>::EqualTo("Again"), "RSplit fail");
static_assert(SPRAWL_TAG("Hello Again World!")::RSplit<SPRAWL_TAG(" ")>::Get<2>::EqualTo("World!"), "RSplit fail");

static_assert(SPRAWL_TAG("Hello Again World!")::RSplit<SPRAWL_TAG(" "), 1>::size == 2, "RSplit fail");
static_assert(SPRAWL_TAG("Hello Again World!")::RSplit<SPRAWL_TAG(" "), 1>::Get<0>::EqualTo("Hello Again"), "RSplit fail");
static_assert(SPRAWL_TAG("Hello Again World!")::RSplit<SPRAWL_TAG(" "), 1>::Get<1>::EqualTo("World!"), "RSplit fail");

static_assert(SPRAWL_TAG("HelloWorld!")::RSplit<SPRAWL_TAG(" ")>::size == 1, "RSplit fail");
static_assert(SPRAWL_TAG("HelloWorld!")::RSplit<SPRAWL_TAG(" ")>::Get<0>::EqualTo("HelloWorld!"), "RSplit fail");

static_assert(SPRAWL_TAG("Hello World!")::CharAt<0>() == 'H', "CharAt fail");
static_assert(SPRAWL_TAG("Hello World!")::CharAt<1>() == 'e', "CharAt fail");
static_assert(SPRAWL_TAG("Hello World!")::CharAt<11>() == '!', "CharAt fail");

static_assert(SPRAWL_TAG("Hello World!")::Insert<5, SPRAWL_TAG(" Again")>::EqualTo("Hello Again World!"), "Insert failed");
static_assert(SPRAWL_TAG("World!")::Insert<0, SPRAWL_TAG("Hello ")>::EqualTo("Hello World!"), "Insert failed");
static_assert(SPRAWL_TAG("Hello")::Insert<5, SPRAWL_TAG(" World!")>::EqualTo("Hello World!"), "Insert failed");

static_assert(SPRAWL_TAG(" \tHello World!\t ")::FindFirstOf(" \n\t\v\f\r") == 0, "FindFirstOf failed");
static_assert(SPRAWL_TAG("\t Hello World! \t")::FindFirstOf(" \n\t\v\f\r") == 0, "FindFirstOf failed");
static_assert(SPRAWL_TAG("\t Hello World! \t")::FindFirstOf(" \n\t\v\f\r", 2) == 7, "FindFirstOf failed");
static_assert(SPRAWL_TAG("\t Hello World! \t")::FindFirstOf(" \n\t\v\f\r", 2, 6) == -1, "FindFirstOf failed");
static_assert(SPRAWL_TAG("HelloWorld!")::FindFirstOf(" \n\t\v\f\r") == -1, "FindFirstOf failed");

static_assert(SPRAWL_TAG(" \tHello World!\t ")::FindLastOf(" \n\t\v\f\r") == 15, "FindLastOf failed");
static_assert(SPRAWL_TAG("\t Hello World! \t")::FindLastOf(" \n\t\v\f\r") == 15, "FindLastOf failed");
static_assert(SPRAWL_TAG("\t Hello World! \t")::FindLastOf(" \n\t\v\f\r", 0, 8) == 7, "FindLastOf failed");
static_assert(SPRAWL_TAG("\t Hello World! \t")::FindLastOf(" \n\t\v\f\r", 2, 6) == -1, "FindLastOf failed");
static_assert(SPRAWL_TAG("HelloWorld!")::FindLastOf(" \n\t\v\f\r") == -1, "FindLastOf failed");

static_assert(SPRAWL_TAG(" \tHello World!\t ")::FindFirstNotOf(" \n\t\v\f\r") == 2, "FindFirstNotOf failed");
static_assert(SPRAWL_TAG("\t Hello World! \t")::FindFirstNotOf(" \n\t\v\f\r") == 2, "FindFirstNotOf failed");
static_assert(SPRAWL_TAG("\t Hello World! \t")::FindFirstNotOf(" \n\t\v\f\r", 7) == 8, "FindFirstNotOf failed");
static_assert(SPRAWL_TAG("\t Hello World! \t")::FindFirstNotOf(" \n\t\v\f\r", 0, 2) == -1, "FindFirstNotOf failed");
static_assert(SPRAWL_TAG("\t \n\t \t")::FindFirstNotOf(" \n\t\v\f\r") == -1, "FindFirstNotOf failed");

static_assert(SPRAWL_TAG(" \tHello World!\t ")::FindLastNotOf(" \n\t\v\f\r") == 13, "FindLastNotOf failed");
static_assert(SPRAWL_TAG("\t Hello World! \t")::FindLastNotOf(" \n\t\v\f\r") == 13, "FindLastNotOf failed");
static_assert(SPRAWL_TAG("\t Hello World! \t")::FindLastNotOf(" \n\t\v\f\r", 0, 8) == 6, "FindLastNotOf failed");
static_assert(SPRAWL_TAG("\t Hello World! \t")::FindLastNotOf(" \n\t\v\f\r", 14) == -1, "FindLastNotOf failed");
static_assert(SPRAWL_TAG("\t \n\t \t")::FindLastNotOf(" \n\t\v\f\r") == -1, "FindLastNotOf failed");

static_assert(SPRAWL_TAG(" \tHello World!\t ")::FindFirstOf<sprawl::IsWhitespace>() == 0, "FindFirstOf failed");
static_assert(SPRAWL_TAG("\t Hello World! \t")::FindFirstOf<sprawl::IsWhitespace>() == 0, "FindFirstOf failed");
static_assert(SPRAWL_TAG("\t Hello World! \t")::FindFirstOf<sprawl::IsWhitespace, 2>() == 7, "FindFirstOf failed");
static_assert(SPRAWL_TAG("\t Hello World! \t")::FindFirstOf<sprawl::IsWhitespace, 2, 6>() == -1, "FindFirstOf failed");
static_assert(SPRAWL_TAG("HelloWorld!")::FindFirstOf<sprawl::IsWhitespace>() == -1, "FindFirstOf failed");

static_assert(SPRAWL_TAG(" \tHello World!\t ")::FindLastOf<sprawl::IsWhitespace>() == 15, "FindLastOf failed");
static_assert(SPRAWL_TAG("\t Hello World! \t")::FindLastOf<sprawl::IsWhitespace>() == 15, "FindLastOf failed");
static_assert(SPRAWL_TAG("\t Hello World! \t")::FindLastOf<sprawl::IsWhitespace, 0, 8>() == 7, "FindLastOf failed");
static_assert(SPRAWL_TAG("\t Hello World! \t")::FindLastOf<sprawl::IsWhitespace, 2, 6>() == -1, "FindLastOf failed");
static_assert(SPRAWL_TAG("HelloWorld!")::FindLastOf<sprawl::IsWhitespace>() == -1, "FindLastOf failed");

static_assert(SPRAWL_TAG(" \tHello World!\t ")::FindFirstNotOf<sprawl::IsWhitespace>() == 2, "FindFirstNotOf failed");
static_assert(SPRAWL_TAG("\t Hello World! \t")::FindFirstNotOf<sprawl::IsWhitespace>() == 2, "FindFirstNotOf failed");
static_assert(SPRAWL_TAG("\t Hello World! \t")::FindFirstNotOf<sprawl::IsWhitespace, 7>() == 8, "FindFirstNotOf failed");
static_assert(SPRAWL_TAG("\t Hello World! \t")::FindFirstNotOf<sprawl::IsWhitespace, 0, 2>() == -1, "FindFirstNotOf failed");
static_assert(SPRAWL_TAG("\t \n\t \t")::FindFirstNotOf<sprawl::IsWhitespace>() == -1, "FindFirstNotOf failed");

static_assert(SPRAWL_TAG(" \tHello World!\t ")::FindLastNotOf<sprawl::IsWhitespace>() == 13, "FindLastNotOf failed");
static_assert(SPRAWL_TAG("\t Hello World! \t")::FindLastNotOf<sprawl::IsWhitespace>() == 13, "FindLastNotOf failed");
static_assert(SPRAWL_TAG("\t Hello World! \t")::FindLastNotOf<sprawl::IsWhitespace, 0, 8>() == 6, "FindLastNotOf failed");
static_assert(SPRAWL_TAG("\t Hello World! \t")::FindLastNotOf<sprawl::IsWhitespace, 14>() == -1, "FindLastNotOf failed");
static_assert(SPRAWL_TAG("\t \n\t \t")::FindLastNotOf<sprawl::IsWhitespace>() == -1, "FindLastNotOf failed");

static_assert(SPRAWL_TAG(" \tHello World!\t ")::ReplaceAnyOf<sprawl::IsWhitespace, SPRAWL_TAG("{WS}")>::EqualTo("{WS}{WS}Hello{WS}World!{WS}{WS}"), "ReplaceAnyOf failed");
static_assert(SPRAWL_TAG(" \tHello World!\t ")::ReplaceAnyOf<sprawl::IsWhitespace, SPRAWL_TAG("-")>::EqualTo("--Hello-World!--"), "ReplaceAnyOf failed");
static_assert(SPRAWL_TAG(" \tHello World!\t ")::ReplaceAnyOf<sprawl::IsWhitespace, SPRAWL_TAG("-"), 3>::EqualTo("--Hello-World!\t "), "ReplaceAnyOf failed");
static_assert(SPRAWL_TAG(" \tHello World!\t ")::ReplaceAnyOf<sprawl::IsWhitespace, SPRAWL_TAG("")>::EqualTo("HelloWorld!"), "ReplaceAnyOf failed");
static_assert(SPRAWL_TAG(" \tHello World!\t ")::ReplaceAnyNotOf<sprawl::IsWhitespace, SPRAWL_TAG("{CH}")>::EqualTo(" \t{CH}{CH}{CH}{CH}{CH} {CH}{CH}{CH}{CH}{CH}{CH}\t "), "ReplaceAnyNotOf failed");
static_assert(SPRAWL_TAG(" \tHello World!\t ")::ReplaceAnyNotOf<sprawl::IsWhitespace, SPRAWL_TAG("-")>::EqualTo(" \t----- ------\t "), "ReplaceAnyNotOf failed");
static_assert(SPRAWL_TAG(" \tHello World!\t ")::ReplaceAnyNotOf<sprawl::IsWhitespace, SPRAWL_TAG("-"), 3>::EqualTo(" \t---lo World!\t "), "ReplaceAnyNotOf failed");
static_assert(SPRAWL_TAG(" \tHello World!\t ")::ReplaceAnyNotOf<sprawl::IsWhitespace, SPRAWL_TAG("")>::EqualTo(" \t \t "), "ReplaceAnyNotOf failed");

static_assert(SPRAWL_TAG(" \tHello World!\t ")::RReplaceAnyOf<sprawl::IsWhitespace, SPRAWL_TAG("{WS}")>::EqualTo("{WS}{WS}Hello{WS}World!{WS}{WS}"), "RReplaceAnyOf failed");
static_assert(SPRAWL_TAG(" \tHello World!\t ")::RReplaceAnyOf<sprawl::IsWhitespace, SPRAWL_TAG("-")>::EqualTo("--Hello-World!--"), "RReplaceAnyOf failed");
static_assert(SPRAWL_TAG(" \tHello World!\t ")::RReplaceAnyOf<sprawl::IsWhitespace, SPRAWL_TAG("-"), 3>::EqualTo(" \tHello-World!--"), "RReplaceAnyOf failed");
static_assert(SPRAWL_TAG(" \tHello World!\t ")::RReplaceAnyOf<sprawl::IsWhitespace, SPRAWL_TAG("")>::EqualTo("HelloWorld!"), "RReplaceAnyOf failed");
static_assert(SPRAWL_TAG(" \tHello World!\t ")::RReplaceAnyNotOf<sprawl::IsWhitespace, SPRAWL_TAG("{CH}")>::EqualTo(" \t{CH}{CH}{CH}{CH}{CH} {CH}{CH}{CH}{CH}{CH}{CH}\t "), "RReplaceAnyNotOf failed");
static_assert(SPRAWL_TAG(" \tHello World!\t ")::RReplaceAnyNotOf<sprawl::IsWhitespace, SPRAWL_TAG("-")>::EqualTo(" \t----- ------\t "), "RReplaceAnyNotOf failed");
static_assert(SPRAWL_TAG(" \tHello World!\t ")::RReplaceAnyNotOf<sprawl::IsWhitespace, SPRAWL_TAG("-"), 3>::EqualTo(" \tHello Wor---\t "), "RReplaceAnyNotOf failed");
static_assert(SPRAWL_TAG(" \tHello World!\t ")::RReplaceAnyNotOf<sprawl::IsWhitespace, SPRAWL_TAG("")>::EqualTo(" \t \t "), "RReplaceAnyNotOf failed");

static_assert(SPRAWL_TAG("A\nB\tC D")::SplitAnyOf<sprawl::IsWhitespace>::size == 4, "SplitAnyOf failed");
static_assert(SPRAWL_TAG("A\nB\tC D")::SplitAnyOf<sprawl::IsWhitespace>::Get<0>::EqualTo("A"), "SplitAnyOf failed");
static_assert(SPRAWL_TAG("A\nB\tC D")::SplitAnyOf<sprawl::IsWhitespace>::Get<1>::EqualTo("B"), "SplitAnyOf failed");
static_assert(SPRAWL_TAG("A\nB\tC D")::SplitAnyOf<sprawl::IsWhitespace>::Get<2>::EqualTo("C"), "SplitAnyOf failed");
static_assert(SPRAWL_TAG("A\nB\tC D")::SplitAnyOf<sprawl::IsWhitespace>::Get<3>::EqualTo("D"), "SplitAnyOf failed");

static_assert(SPRAWL_TAG("A\nB\tC D")::SplitAnyOf<sprawl::IsWhitespace, 1>::size == 2, "SplitAnyOf failed");
static_assert(SPRAWL_TAG("A\nB\tC D")::SplitAnyOf<sprawl::IsWhitespace, 1>::Get<0>::EqualTo("A"), "SplitAnyOf failed");
static_assert(SPRAWL_TAG("A\nB\tC D")::SplitAnyOf<sprawl::IsWhitespace, 1>::Get<1>::EqualTo("B\tC D"), "SplitAnyOf failed");

static_assert(SPRAWL_TAG("A\nB\tC D")::SplitAnyNotOf<sprawl::IsWhitespace>::size == 5, "SplitAnyNotOf failed");
static_assert(SPRAWL_TAG("A\nB\tC D")::SplitAnyNotOf<sprawl::IsWhitespace>::Get<0>::EqualTo(""), "SplitAnyNotOf failed");
static_assert(SPRAWL_TAG("A\nB\tC D")::SplitAnyNotOf<sprawl::IsWhitespace>::Get<1>::EqualTo("\n"), "SplitAnyNotOf failed");
static_assert(SPRAWL_TAG("A\nB\tC D")::SplitAnyNotOf<sprawl::IsWhitespace>::Get<2>::EqualTo("\t"), "SplitAnyNotOf failed");
static_assert(SPRAWL_TAG("A\nB\tC D")::SplitAnyNotOf<sprawl::IsWhitespace>::Get<3>::EqualTo(" "), "SplitAnyNotOf failed");
static_assert(SPRAWL_TAG("A\nB\tC D")::SplitAnyNotOf<sprawl::IsWhitespace>::Get<4>::EqualTo(""), "SplitAnyNotOf failed");

static_assert(SPRAWL_TAG("A\nB\tC D")::SplitAnyNotOf<sprawl::IsWhitespace, 1>::size == 2, "SplitAnyNotOf failed");
static_assert(SPRAWL_TAG("A\nB\tC D")::SplitAnyNotOf<sprawl::IsWhitespace, 1>::Get<0>::EqualTo(""), "SplitAnyNotOf failed");
static_assert(SPRAWL_TAG("A\nB\tC D")::SplitAnyNotOf<sprawl::IsWhitespace, 1>::Get<1>::EqualTo("\nB\tC D"), "SplitAnyNotOf failed");

static_assert(SPRAWL_TAG("A\nB\tC D")::RSplitAnyOf<sprawl::IsWhitespace>::size == 4, "RSplitAnyOf failed");
static_assert(SPRAWL_TAG("A\nB\tC D")::RSplitAnyOf<sprawl::IsWhitespace>::Get<0>::EqualTo("A"), "RSplitAnyOf failed");
static_assert(SPRAWL_TAG("A\nB\tC D")::RSplitAnyOf<sprawl::IsWhitespace>::Get<1>::EqualTo("B"), "RSplitAnyOf failed");
static_assert(SPRAWL_TAG("A\nB\tC D")::RSplitAnyOf<sprawl::IsWhitespace>::Get<2>::EqualTo("C"), "RSplitAnyOf failed");
static_assert(SPRAWL_TAG("A\nB\tC D")::RSplitAnyOf<sprawl::IsWhitespace>::Get<3>::EqualTo("D"), "RSplitAnyOf failed");

static_assert(SPRAWL_TAG("A\nB\tC D")::RSplitAnyOf<sprawl::IsWhitespace, 1>::size == 2, "RSplitAnyOf failed");
static_assert(SPRAWL_TAG("A\nB\tC D")::RSplitAnyOf<sprawl::IsWhitespace, 1>::Get<0>::EqualTo("A\nB\tC"), "RSplitAnyOf failed");
static_assert(SPRAWL_TAG("A\nB\tC D")::RSplitAnyOf<sprawl::IsWhitespace, 1>::Get<1>::EqualTo("D"), "RSplitAnyOf failed");

static_assert(SPRAWL_TAG("A\nB\tC D")::RSplitAnyNotOf<sprawl::IsWhitespace>::size == 5, "RSplitAnyNotOf failed");
static_assert(SPRAWL_TAG("A\nB\tC D")::RSplitAnyNotOf<sprawl::IsWhitespace>::Get<0>::EqualTo(""), "RSplitAnyNotOf failed");
static_assert(SPRAWL_TAG("A\nB\tC D")::RSplitAnyNotOf<sprawl::IsWhitespace>::Get<1>::EqualTo("\n"), "RSplitAnyNotOf failed");
static_assert(SPRAWL_TAG("A\nB\tC D")::RSplitAnyNotOf<sprawl::IsWhitespace>::Get<2>::EqualTo("\t"), "RSplitAnyNotOf failed");
static_assert(SPRAWL_TAG("A\nB\tC D")::RSplitAnyNotOf<sprawl::IsWhitespace>::Get<3>::EqualTo(" "), "RSplitAnyNotOf failed");
static_assert(SPRAWL_TAG("A\nB\tC D")::RSplitAnyNotOf<sprawl::IsWhitespace>::Get<4>::EqualTo(""), "RSplitAnyNotOf failed");

static_assert(SPRAWL_TAG("A\nB\tC D")::RSplitAnyNotOf<sprawl::IsWhitespace, 1>::size == 2, "RSplitAnyNotOf failed");
static_assert(SPRAWL_TAG("A\nB\tC D")::RSplitAnyNotOf<sprawl::IsWhitespace, 1>::Get<0>::EqualTo("A\nB\tC "), "RSplitAnyNotOf failed");
static_assert(SPRAWL_TAG("A\nB\tC D")::RSplitAnyNotOf<sprawl::IsWhitespace, 1>::Get<1>::EqualTo(""), "RSplitAnyNotOf failed");


static_assert(SPRAWL_TAG("A\nB\rC\n\rD")::SplitLines<>::size == 4, "SplitLines failed");
static_assert(SPRAWL_TAG("A\nB\rC\n\rD")::SplitLines<>::Get<0>::EqualTo("A"), "SplitLines failed");
static_assert(SPRAWL_TAG("A\nB\rC\n\rD")::SplitLines<>::Get<1>::EqualTo("B"), "SplitLines failed");
static_assert(SPRAWL_TAG("A\nB\rC\n\rD")::SplitLines<>::Get<2>::EqualTo("C"), "SplitLines failed");
static_assert(SPRAWL_TAG("A\nB\rC\n\rD")::SplitLines<>::Get<3>::EqualTo("D"), "SplitLines failed");

static_assert(SPRAWL_TAG("A\nB\rC\n\rD\r\n")::SplitLines<>::size == 4, "SplitLines failed");
static_assert(SPRAWL_TAG("A\nB\rC\n\rD\r\n")::SplitLines<>::Get<0>::EqualTo("A"), "SplitLines failed");
static_assert(SPRAWL_TAG("A\nB\rC\n\rD\r\n")::SplitLines<>::Get<1>::EqualTo("B"), "SplitLines failed");
static_assert(SPRAWL_TAG("A\nB\rC\n\rD\r\n")::SplitLines<>::Get<2>::EqualTo("C"), "SplitLines failed");
static_assert(SPRAWL_TAG("A\nB\rC\n\rD\r\n")::SplitLines<>::Get<3>::EqualTo("D"), "SplitLines failed");

static_assert(SPRAWL_TAG("A\nB\rC\n\rD")::SplitLines<true>::size == 4, "SplitLines failed");
static_assert(SPRAWL_TAG("A\nB\rC\n\rD")::SplitLines<true>::Get<0>::EqualTo("A\n"), "SplitLines failed");
static_assert(SPRAWL_TAG("A\nB\rC\n\rD")::SplitLines<true>::Get<1>::EqualTo("B\r"), "SplitLines failed");
static_assert(SPRAWL_TAG("A\nB\rC\n\rD")::SplitLines<true>::Get<2>::EqualTo("C\n\r"), "SplitLines failed");
static_assert(SPRAWL_TAG("A\nB\rC\n\rD")::SplitLines<true>::Get<3>::EqualTo("D"), "SplitLines failed");

static_assert(SPRAWL_TAG("A\nB\rC\n\rD\r\n")::SplitLines<true>::size == 4, "SplitLines failed");
static_assert(SPRAWL_TAG("A\nB\rC\n\rD\r\n")::SplitLines<true>::Get<0>::EqualTo("A\n"), "SplitLines failed");
static_assert(SPRAWL_TAG("A\nB\rC\n\rD\r\n")::SplitLines<true>::Get<1>::EqualTo("B\r"), "SplitLines failed");
static_assert(SPRAWL_TAG("A\nB\rC\n\rD\r\n")::SplitLines<true>::Get<2>::EqualTo("C\n\r"), "SplitLines failed");
static_assert(SPRAWL_TAG("A\nB\rC\n\rD\r\n")::SplitLines<true>::Get<3>::EqualTo("D\r\n"), "SplitLines failed");

static_assert(SPRAWL_TAG("0")::As<int64_t>() == 0, "As<int64_t> failed");
static_assert(SPRAWL_TAG("100")::As<int64_t>() == 100, "As<int64_t> failed");
static_assert(SPRAWL_TAG("1024")::As<int64_t>() == 1024, "As<int64_t> failed");
static_assert(SPRAWL_TAG("-15")::As<int64_t>() == -15, "As<int64_t> failed");
static_assert(SPRAWL_TAG("1234j4321")::As<int64_t>() == 1234, "As<int64_t> failed");
static_assert(SPRAWL_TAG("h1234")::As<int64_t>() == 0, "As<int64_t> failed");
static_assert(SPRAWL_TAG("9223372036854775807")::As<int64_t>() == 9223372036854775807ll, "As<int64_t> failed");
static constexpr int64_t min_int64 = int64_t(9223372036854775808ull);
static_assert(SPRAWL_TAG("9223372036854775808")::As<int64_t>() == min_int64, "As<int64_t> failed");

static_assert(SPRAWL_TAG("0")::As<double>() == 0.0, "As<double> failed");
static_assert(SPRAWL_TAG("100")::As<double>() == 100.0, "As<double> failed");
static_assert(SPRAWL_TAG("1024")::As<double>() == 1024.0, "As<double> failed");
static_assert(SPRAWL_TAG("-15")::As<double>() == -15.0, "As<double> failed");
static_assert(SPRAWL_TAG("1234j4321")::As<double>() == 1234.0, "As<double> failed");
static_assert(SPRAWL_TAG("h1234")::As<double>() == 0.0, "As<double> failed");
static_assert(SPRAWL_TAG("1.1")::As<double>() == 1.1, "As<double> failed");
static_assert(SPRAWL_TAG("1.2345")::As<double>() == 1.2345, "As<double> failed");
static_assert(SPRAWL_TAG("-5.4321")::As<double>() == -5.4321, "As<double> failed");
static_assert(SPRAWL_TAG(".2")::As<double>() == 0.2, "As<double> failed");
static_assert(SPRAWL_TAG("-.2")::As<double>() == -0.2, "As<double> failed");
static_assert(SPRAWL_TAG("1.")::As<double>() == 1.0, "As<double> failed");

static_assert(SPRAWL_TAG("33")::As<char>() == '!', "As<char> failed");

static_assert(SPRAWL_TAG("TRUE")::As<bool>(), "As<bool> failed");
static_assert(SPRAWL_TAG("true")::As<bool>(), "As<bool> failed");
static_assert(SPRAWL_TAG("True")::As<bool>(), "As<bool> failed");
static_assert(SPRAWL_TAG("truE")::As<bool>(), "As<bool> failed");
static_assert(SPRAWL_TAG("1")::As<bool>(), "As<bool> failed");
static_assert(SPRAWL_TAG("3")::As<bool>(), "As<bool> failed");
static_assert(SPRAWL_TAG("5.5")::As<bool>(), "As<bool> failed");
static_assert(SPRAWL_TAG(".6")::As<bool>(), "As<bool> failed");
static_assert(!SPRAWL_TAG("false")::As<bool>(), "As<bool> failed");
static_assert(!SPRAWL_TAG("cheese")::As<bool>(), "As<bool> failed");
static_assert(!SPRAWL_TAG("0")::As<bool>(), "As<bool> failed");

static_assert(SPRAWL_TAG("0")::As<uint64_t>() == 0, "As<uint64_t> failed");
static_assert(SPRAWL_TAG("100")::As<uint64_t>() == 100, "As<uint64_t> failed");
static_assert(SPRAWL_TAG("1024")::As<uint64_t>() == 1024, "As<uint64_t> failed");
static_assert(SPRAWL_TAG("-15")::As<uint64_t>() == uint64_t(-15), "As<uint64_t> failed");
static_assert(SPRAWL_TAG("1234j4321")::As<uint64_t>() == 1234, "As<uint64_t> failed");
static_assert(SPRAWL_TAG("h1234")::As<uint64_t>() == 0, "As<uint64_t> failed");
static_assert(SPRAWL_TAG("9223372036854775807")::As<uint64_t>() == 9223372036854775807ull, "As<uint64_t> failed");
static_assert(SPRAWL_TAG("9223372036854775808")::As<uint64_t>() == 9223372036854775808ull, "As<uint64_t> failed");

//Ensure early-out once a non-int is found. If early-out fails then the below code will cause overflows.
static_assert(SPRAWL_TAG("1h123459223372036854775808")::As<uint64_t>() == 1, "As<uint64_t> failed");
static_assert(SPRAWL_TAG("1h123459223372036854775808")::As<double>() == 1, "As<uint64_t> failed");

static_assert(sprawl::COMPILE_DATE_TIME_TAG::Replace<SPRAWL_TAG("  "), SPRAWL_TAG(" ")>::Replace<SPRAWL_TAG(":"), SPRAWL_TAG(" ")>::Split<SPRAWL_TAG(" ")>::size == 6, "Date parse failed");

static_assert(SPRAWL_TAG("Hello World!")::FindFirstInList<sprawl::TypeList<SPRAWL_TAG("Hello"), SPRAWL_TAG("World!")>>() == 0, "FindFirstInList failed");
static_assert(SPRAWL_TAG("Hello World!")::FindFirstInList<std::tuple<SPRAWL_TAG("Hello"), SPRAWL_TAG("World!")>>() == 0, "FindFirstInList failed");
static_assert(SPRAWL_TAG("Hello World!")::FindFirstInList<sprawl::TypeList<SPRAWL_TAG("Goodbye"), SPRAWL_TAG("World!")>>() == 6, "FindFirstInList failed");
static_assert(SPRAWL_TAG("Hello World!")::FindFirstInList<std::tuple<SPRAWL_TAG("Goodbye"), SPRAWL_TAG("World!")>>() == 6, "FindFirstInList failed");
static_assert(SPRAWL_TAG("Hello World!")::FindLastInList<sprawl::TypeList<SPRAWL_TAG("Hello"), SPRAWL_TAG("World!")>>() == 6, "FindLastInList failed");
static_assert(SPRAWL_TAG("Hello World!")::FindLastInList<std::tuple<SPRAWL_TAG("Hello"), SPRAWL_TAG("World!")>>() == 6, "FindLastInList failed");
static_assert(SPRAWL_TAG("Hello World!")::FindLastInList<sprawl::TypeList<SPRAWL_TAG("Hello"), SPRAWL_TAG("Universe!")>>() == 0, "FindLastInList failed");
static_assert(SPRAWL_TAG("Hello World!")::FindLastInList<std::tuple<SPRAWL_TAG("Hello"), SPRAWL_TAG("Universe!")>>() == 0, "FindLastInList failed");

static_assert(SPRAWL_TAG("Hello World!")::ReplaceAnyInList<std::tuple<SPRAWL_TAG("Hello"), SPRAWL_TAG("World")>, SPRAWL_TAG("Foo")>::EqualTo("Foo Foo!"), "ReplaceAnyInList failed");
static_assert(SPRAWL_TAG("Hello World!")::ReplaceAnyInList<std::tuple<SPRAWL_TAG("Hello"), SPRAWL_TAG("World")>, SPRAWL_TAG("Foo"), 1>::EqualTo("Foo World!"), "ReplaceAnyInList failed");
static_assert(SPRAWL_TAG("Hello World!")::ReplaceAnyInList<std::tuple<SPRAWL_TAG("World"), SPRAWL_TAG("Hello")>, SPRAWL_TAG("Foo"), 1>::EqualTo("Foo World!"), "ReplaceAnyInList failed");

static_assert(SPRAWL_TAG("Hello World!")::RReplaceAnyInList<std::tuple<SPRAWL_TAG("Hello"), SPRAWL_TAG("World")>, SPRAWL_TAG("Foo")>::EqualTo("Foo Foo!"), "RReplaceAnyInList failed");
static_assert(SPRAWL_TAG("Hello World!")::RReplaceAnyInList<std::tuple<SPRAWL_TAG("Hello"), SPRAWL_TAG("World")>, SPRAWL_TAG("Foo"), 1>::EqualTo("Hello Foo!"), "RReplaceAnyInList failed");
static_assert(SPRAWL_TAG("Hello World!")::RReplaceAnyInList<std::tuple<SPRAWL_TAG("World"), SPRAWL_TAG("Hello")>, SPRAWL_TAG("Foo"), 1>::EqualTo("Hello Foo!"), "RReplaceAnyInList failed");

static_assert(SPRAWL_TAG("HAfooHAbarHA!")::SplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>>::size == 3, "SplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHA!")::SplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>>::Get<0>::EqualTo("HA"), "SplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHA!")::SplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>>::Get<1>::EqualTo("HA"), "SplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHA!")::SplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>>::Get<2>::EqualTo("HA!"), "SplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHAfooHA!")::SplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>>::size == 4, "SplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHAfooHA!")::SplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>>::Get<0>::EqualTo("HA"), "SplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHAfooHA!")::SplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>>::Get<1>::EqualTo("HA"), "SplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHAfooHA!")::SplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>>::Get<2>::EqualTo("HA"), "SplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHAfooHA!")::SplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>>::Get<3>::EqualTo("HA!"), "SplitAnyInList failed");

static_assert(SPRAWL_TAG("HAfooHAbarHA!")::SplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>, 1>::size == 2, "SplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHA!")::SplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>, 1>::Get<0>::EqualTo("HA"), "SplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHA!")::SplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>, 1>::Get<1>::EqualTo("HAbarHA!"), "SplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHAfooHA!")::SplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>, 2>::size == 3, "SplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHAfooHA!")::SplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>, 2>::Get<0>::EqualTo("HA"), "SplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHAfooHA!")::SplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>, 2>::Get<1>::EqualTo("HA"), "SplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHAfooHA!")::SplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>, 2>::Get<2>::EqualTo("HAfooHA!"), "SplitAnyInList failed");

static_assert(SPRAWL_TAG("HAfooHAbarHA!")::SplitAnyInList<sprawl::TypeList<SPRAWL_TAG("bar"), SPRAWL_TAG("foo")>, 1>::size == 2, "SplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHA!")::SplitAnyInList<sprawl::TypeList<SPRAWL_TAG("bar"), SPRAWL_TAG("foo")>, 1>::Get<0>::EqualTo("HA"), "SplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHA!")::SplitAnyInList<sprawl::TypeList<SPRAWL_TAG("bar"), SPRAWL_TAG("foo")>, 1>::Get<1>::EqualTo("HAbarHA!"), "SplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHAfooHA!")::SplitAnyInList<sprawl::TypeList<SPRAWL_TAG("bar"), SPRAWL_TAG("foo")>, 2>::size == 3, "SplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHAfooHA!")::SplitAnyInList<sprawl::TypeList<SPRAWL_TAG("bar"), SPRAWL_TAG("foo")>, 2>::Get<0>::EqualTo("HA"), "SplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHAfooHA!")::SplitAnyInList<sprawl::TypeList<SPRAWL_TAG("bar"), SPRAWL_TAG("foo")>, 2>::Get<1>::EqualTo("HA"), "SplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHAfooHA!")::SplitAnyInList<sprawl::TypeList<SPRAWL_TAG("bar"), SPRAWL_TAG("foo")>, 2>::Get<2>::EqualTo("HAfooHA!"), "SplitAnyInList failed");

static_assert(SPRAWL_TAG("HAfooHAbarHA!")::RSplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>>::size == 3, "RSplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHA!")::RSplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>>::Get<0>::EqualTo("HA"), "RSplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHA!")::RSplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>>::Get<1>::EqualTo("HA"), "RSplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHA!")::RSplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>>::Get<2>::EqualTo("HA!"), "RSplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHAfooHA!")::RSplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>>::size == 4, "RSplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHAfooHA!")::RSplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>>::Get<0>::EqualTo("HA"), "RSplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHAfooHA!")::RSplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>>::Get<1>::EqualTo("HA"), "RSplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHAfooHA!")::RSplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>>::Get<2>::EqualTo("HA"), "RSplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHAfooHA!")::RSplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>>::Get<3>::EqualTo("HA!"), "RSplitAnyInList failed");

static_assert(SPRAWL_TAG("HAfooHAbarHA!")::RSplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>, 1>::size == 2, "RSplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHA!")::RSplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>, 1>::Get<0>::EqualTo("HAfooHA"), "RSplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHA!")::RSplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>, 1>::Get<1>::EqualTo("HA!"), "RSplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHAfooHA!")::RSplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>, 2>::size == 3, "RSplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHAfooHA!")::RSplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>, 2>::Get<0>::EqualTo("HAfooHA"), "RSplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHAfooHA!")::RSplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>, 2>::Get<1>::EqualTo("HA"), "RSplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHAfooHA!")::RSplitAnyInList<sprawl::TypeList<SPRAWL_TAG("foo"), SPRAWL_TAG("bar")>, 2>::Get<2>::EqualTo("HA!"), "RSplitAnyInList failed");

static_assert(SPRAWL_TAG("HAfooHAbarHA!")::RSplitAnyInList<sprawl::TypeList<SPRAWL_TAG("bar"), SPRAWL_TAG("foo")>, 1>::size == 2, "RSplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHA!")::RSplitAnyInList<sprawl::TypeList<SPRAWL_TAG("bar"), SPRAWL_TAG("foo")>, 1>::Get<0>::EqualTo("HAfooHA"), "RSplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHA!")::RSplitAnyInList<sprawl::TypeList<SPRAWL_TAG("bar"), SPRAWL_TAG("foo")>, 1>::Get<1>::EqualTo("HA!"), "RSplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHAfooHA!")::RSplitAnyInList<sprawl::TypeList<SPRAWL_TAG("bar"), SPRAWL_TAG("foo")>, 2>::size == 3, "RSplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHAfooHA!")::RSplitAnyInList<sprawl::TypeList<SPRAWL_TAG("bar"), SPRAWL_TAG("foo")>, 2>::Get<0>::EqualTo("HAfooHA"), "RSplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHAfooHA!")::RSplitAnyInList<sprawl::TypeList<SPRAWL_TAG("bar"), SPRAWL_TAG("foo")>, 2>::Get<1>::EqualTo("HA"), "RSplitAnyInList failed");
static_assert(SPRAWL_TAG("HAfooHAbarHAfooHA!")::RSplitAnyInList<sprawl::TypeList<SPRAWL_TAG("bar"), SPRAWL_TAG("foo")>, 2>::Get<2>::EqualTo("HA!"), "RSplitAnyInList failed");

template<typename T>
struct nested
{
	struct foo
	{

	};
};
static_assert(sprawl::TypeName<int>::EqualTo("int"), "TypeName failed");
static_assert(sprawl::TypeName<std::tuple<bool, int>>::EqualTo("std::tuple<bool, int>"), "TypeName failed");
static_assert(sprawl::TypeName<nested<int>::foo>::EqualTo("nested<int>::foo"), "TypeName failed");
static_assert(sprawl::TypeName<std::tuple<bool, std::tuple<bool, int>>>::EqualTo("std::tuple<bool, std::tuple<bool, int>>"), "TypeName failed");

static_assert(sprawl::TagFrom<int>::FromValue<0>::EqualTo("0"), "FromInt failed");
static_assert(sprawl::TagFrom<int>::FromValue<-1>::EqualTo("-1"), "FromInt failed");
static_assert(sprawl::TagFrom<int>::FromValue<1>::EqualTo("1"), "FromInt failed");
static_assert(sprawl::TagFrom<int>::FromValue<12345>::EqualTo("12345"), "FromInt failed");
static_assert(sprawl::TagFrom<int>::FromValue<-12345>::EqualTo("-12345"), "FromInt failed");

static_assert(sprawl::TagFrom<int>::FromValue<0x0, 16>::EqualTo("0"), "FromInt failed");
static_assert(sprawl::TagFrom<int>::FromValue<-0x1, 16>::EqualTo("-1"), "FromInt failed");
static_assert(sprawl::TagFrom<int>::FromValue<0x1, 16>::EqualTo("1"), "FromInt failed");
static_assert(sprawl::TagFrom<int>::FromValue<0x90abc, 16>::EqualTo("90ABC"), "FromInt failed");
static_assert(sprawl::TagFrom<int>::FromValue<-0x90abc, 16>::EqualTo("-90ABC"), "FromInt failed");

static_assert(sprawl::TagFrom<unsigned int>::FromValue<0>::EqualTo("0"), "FromInt failed");
static_assert(sprawl::TagFrom<unsigned int>::FromValue<1>::EqualTo("1"), "FromInt failed");
static_assert(sprawl::TagFrom<unsigned int>::FromValue<12345>::EqualTo("12345"), "FromInt failed");

static_assert(sprawl::TagFrom<unsigned int>::FromValue<0x0, 16>::EqualTo("0"), "FromInt failed");
static_assert(sprawl::TagFrom<unsigned int>::FromValue<0x1, 16>::EqualTo("1"), "FromInt failed");
static_assert(sprawl::TagFrom<unsigned int>::FromValue<0x90abc, 16>::EqualTo("90ABC"), "FromInt failed");

static_assert(sprawl::TagFrom<bool>::FromValue<true>::EqualTo("true"), "FromBool failed");
static_assert(sprawl::TagFrom<bool>::FromValue<false>::EqualTo("false"), "FromBool failed");

static_assert(SPRAWL_TAG("1234567890")::Slice<2, 5>::EqualTo("345"), "Slice failed");
static_assert(SPRAWL_TAG("1234567890")::Slice<0, 10, 2>::EqualTo("13579"), "Slice failed");
static_assert(SPRAWL_TAG("1234567890")::Slice<0, 11, 2>::EqualTo("13579"), "Slice failed");
static_assert(SPRAWL_TAG("1234567890")::Slice<0, 10, -1>::EqualTo(""), "Slice failed");
static_assert(SPRAWL_TAG("1234567890")::Slice<0, -1, -1>::EqualTo("1"), "Slice failed");
static_assert(SPRAWL_TAG("1234567890")::Slice<9, -1, -1>::EqualTo("0987654321"), "Slice failed");
static_assert(SPRAWL_TAG("1234567890")::Slice<9, -1, -2>::EqualTo("08642"), "Slice failed");
static_assert(SPRAWL_TAG("1234567890")::Slice<10, -1, -2>::EqualTo("08642"), "Slice failed");
static_assert(SPRAWL_TAG("1234567890")::Slice<7, 3, -1>::EqualTo("8765"), "Slice failed");

TEST(TagTest, Murmur3HashValuesEqual)
{
	typedef SPRAWL_TAG("123456781") t1;
	typedef SPRAWL_TAG("1234567812345678") t2;
	typedef SPRAWL_TAG("123456781234567") t3;
	typedef SPRAWL_TAG("123456781234") t4;
	ASSERT_EQ(sprawl::murmur3::Hash("123456781", sizeof("123456781") - 1, 0), t1::Hash());
	ASSERT_EQ(sprawl::murmur3::Hash("1234567812345678", sizeof("1234567812345678") - 1, 0), t2::Hash<sprawl::Murmur3<0>>());
	ASSERT_EQ(sprawl::murmur3::Hash("123456781234567", sizeof("123456781234567") - 1, 0), t3::Hash<sprawl::Murmur3<0>>());
	ASSERT_EQ(sprawl::murmur3::Hash("123456781234", sizeof("123456781234") - 1, 0), t4::Hash<sprawl::Murmur3<0>>());
}

TEST(TagTest, TestTagName)
{
	ASSERT_EQ(sprawl::String(SPRAWL_TAG("ABCDEFG")::name), sprawl::String("ABCDEFG"));
}
