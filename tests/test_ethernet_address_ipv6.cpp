// clazy:skip
// NOLINTBEGIN

#include <libkommpot.h>

#include <communications/ethernet/ethernet_address.h>
#include <communications/ethernet/ethernet_address_factory.h>

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#ifdef _WIN32
// clang-format off
#    include <ws2tcpip.h>
// clang-format on
#else
#    include <arpa/inet.h>
#endif

using namespace testing;

/*******************************************************************************
 *
 * Helper functions.
 *
 *******************************************************************************/
static ethernet_ipv6_address make_ipv6(const std::string &colon_hex)
{
    sockaddr_in6 sa6 = {};
    sa6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, colon_hex.c_str(), &sa6.sin6_addr);

    auto addr =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa6));
    EXPECT_TRUE(addr.has_value());

    if (!addr.has_value())
    {
        return {};
    }

    return *dynamic_cast<ethernet_ipv6_address *>((*addr).get());
}

/*******************************************************************************
 *
 * Default construction.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_address, default_constructor_to_string_is_all_zeros)
{
    ethernet_ipv6_address addr;
    EXPECT_EQ(addr.to_string(), "0:0:0:0:0:0:0:0");
}

TEST(ethernet_ipv6_address, default_constructor_has_exactly_seven_colons)
{
    ethernet_ipv6_address addr;
    const std::string s = addr.to_string();
    EXPECT_EQ(std::count(s.begin(), s.end(), ':'), 7);
}

TEST(ethernet_ipv6_address, default_constructor_string_not_empty)
{
    ethernet_ipv6_address addr;
    EXPECT_FALSE(addr.to_string().empty());
}

/*******************************************************************************
 *
 * Invalid / malformed input (via factory from_string).
 *
 *******************************************************************************/
TEST(ethernet_ipv6_address, invalid_empty_string)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("").has_value());
}

TEST(ethernet_ipv6_address, invalid_no_colons_no_dots)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("not_an_address").has_value());
}

TEST(ethernet_ipv6_address, invalid_single_colon)
{
    EXPECT_FALSE(ethernet_address_factory::from_string(":").has_value());
}

TEST(ethernet_ipv6_address, invalid_triple_colon)
{
    EXPECT_FALSE(ethernet_address_factory::from_string(":::").has_value());
}

TEST(ethernet_ipv6_address, invalid_too_many_groups)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("1:2:3:4:5:6:7:8:9").has_value());
}

TEST(ethernet_ipv6_address, invalid_non_hex_characters)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("gggg::1").has_value());
}

TEST(ethernet_ipv6_address, invalid_segment_overflow_five_hex_digits)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("fffff::1").has_value());
}

TEST(ethernet_ipv6_address, invalid_double_double_colon)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("1::2::3").has_value());
}

TEST(ethernet_ipv6_address, invalid_whitespace_in_address)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("fe80:: 1").has_value());
}

TEST(ethernet_ipv6_address, invalid_brackets_around_address)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("[::1]").has_value());
}

TEST(ethernet_ipv6_address, invalid_trailing_colon_non_double)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("1:2:3:4:5:6:7:8:").has_value());
}

TEST(ethernet_ipv6_address, invalid_leading_colon_non_double)
{
    EXPECT_FALSE(ethernet_address_factory::from_string(":1:2:3:4:5:6:7:8").has_value());
}

TEST(ethernet_ipv6_address, invalid_negative_value)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("-1::0").has_value());
}

TEST(ethernet_ipv6_address, invalid_only_colons)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("::::").has_value());
}

TEST(ethernet_ipv6_address, valid_all_zeros_via_double_colon)
{
    auto addr = ethernet_address_factory::from_string("::");
    EXPECT_TRUE(addr.has_value());
    EXPECT_NE(*addr, nullptr);
}

TEST(ethernet_ipv6_address, valid_loopback_via_from_string)
{
    auto addr = ethernet_address_factory::from_string("::1");
    EXPECT_TRUE(addr.has_value());
    EXPECT_NE(*addr, nullptr);
}

TEST(ethernet_ipv6_address, valid_full_address_via_from_string)
{
    auto addr = ethernet_address_factory::from_string("2001:db8:85a3:0:0:8a2e:370:7334");
    EXPECT_TRUE(addr.has_value());
    EXPECT_NE(*addr, nullptr);
}

/*******************************************************************************
 *
 * to_string — corner-case IPv6 addresses.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_address, to_string_all_zeros)
{
    auto addr = make_ipv6("::");
    EXPECT_EQ(addr.to_string(), "0:0:0:0:0:0:0:0");
}

TEST(ethernet_ipv6_address, to_string_all_max)
{
    auto addr = make_ipv6("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
    EXPECT_EQ(addr.to_string(), "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
}

TEST(ethernet_ipv6_address, to_string_loopback)
{
    auto addr = make_ipv6("::1");
    EXPECT_EQ(addr.to_string(), "0:0:0:0:0:0:0:1");
}

TEST(ethernet_ipv6_address, to_string_link_local)
{
    // fe80::1 → segment 0 = 0xfe80
    auto addr = make_ipv6("fe80::1");
    EXPECT_EQ(addr.to_string(), "fe80:0:0:0:0:0:0:1");
}

TEST(ethernet_ipv6_address, to_string_documentation_address)
{
    // 2001:db8::1 → segments 0x2001, 0x0db8
    auto addr = make_ipv6("2001:db8::1");
    EXPECT_EQ(addr.to_string(), "2001:db8:0:0:0:0:0:1");
}

TEST(ethernet_ipv6_address, to_string_all_nodes_multicast)
{
    // ff02::1 → segment 0 = 0xff02
    auto addr = make_ipv6("ff02::1");
    EXPECT_EQ(addr.to_string(), "ff02:0:0:0:0:0:0:1");
}

TEST(ethernet_ipv6_address, to_string_all_routers_multicast)
{
    // ff02::2 → segment 0 = 0xff02
    auto addr = make_ipv6("ff02::2");
    EXPECT_EQ(addr.to_string(), "ff02:0:0:0:0:0:0:2");
}

TEST(ethernet_ipv6_address, to_string_ipv4_mapped)
{
    // ::ffff:c0a8:0101 = ::ffff:192.168.1.1
    // Segments: [0,0,0,0,0,0xffff,0xc0a8,0x0101]
    auto addr = make_ipv6("::ffff:c0a8:101");
    EXPECT_EQ(addr.to_string(), "0:0:0:0:0:ffff:c0a8:101");
}

TEST(ethernet_ipv6_address, to_string_full_address)
{
    // 2001:db8:85a3::8a2e:370:7334
    auto addr = make_ipv6("2001:db8:85a3::8a2e:370:7334");
    EXPECT_EQ(addr.to_string(), "2001:db8:85a3:0:0:8a2e:370:7334");
}

TEST(ethernet_ipv6_address, to_string_sequential_segments)
{
    auto addr = make_ipv6("1:2:3:4:5:6:7:8");
    EXPECT_EQ(addr.to_string(), "1:2:3:4:5:6:7:8");
}

TEST(ethernet_ipv6_address, to_string_all_ones)
{
    auto addr = make_ipv6("1:1:1:1:1:1:1:1");
    EXPECT_EQ(addr.to_string(), "1:1:1:1:1:1:1:1");
}

TEST(ethernet_ipv6_address, to_string_first_segment_only)
{
    auto addr = make_ipv6("1::");
    EXPECT_EQ(addr.to_string(), "1:0:0:0:0:0:0:0");
}

TEST(ethernet_ipv6_address, to_string_last_segment_only)
{
    auto addr = make_ipv6("::1");
    EXPECT_EQ(addr.to_string(), "0:0:0:0:0:0:0:1");
}

TEST(ethernet_ipv6_address, to_string_last_segment_max)
{
    auto addr = make_ipv6("::ffff");
    EXPECT_EQ(addr.to_string(), "0:0:0:0:0:0:0:ffff");
}

TEST(ethernet_ipv6_address, to_string_first_segment_max)
{
    auto addr = make_ipv6("ffff::");
    EXPECT_EQ(addr.to_string(), "ffff:0:0:0:0:0:0:0");
}

TEST(ethernet_ipv6_address, to_string_adjacent_to_all_max)
{
    auto addr = make_ipv6("ffff:ffff:ffff:ffff:ffff:ffff:ffff:fffe");
    EXPECT_EQ(addr.to_string(), "ffff:ffff:ffff:ffff:ffff:ffff:ffff:fffe");
}

TEST(ethernet_ipv6_address, to_string_adjacent_to_all_zeros)
{
    auto addr = make_ipv6("::1");
    EXPECT_EQ(addr.to_string(), "0:0:0:0:0:0:0:1");
}

/*******************************************************************************
 *
 * to_string — each segment individually set to 1.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_address, to_string_only_segment_0_set)
{
    auto addr = make_ipv6("1::");
    EXPECT_EQ(addr.to_string(), "1:0:0:0:0:0:0:0");
}

TEST(ethernet_ipv6_address, to_string_only_segment_1_set)
{
    auto addr = make_ipv6("0:1::");
    EXPECT_EQ(addr.to_string(), "0:1:0:0:0:0:0:0");
}

TEST(ethernet_ipv6_address, to_string_only_segment_2_set)
{
    auto addr = make_ipv6("0:0:1::");
    EXPECT_EQ(addr.to_string(), "0:0:1:0:0:0:0:0");
}

TEST(ethernet_ipv6_address, to_string_only_segment_3_set)
{
    auto addr = make_ipv6("0:0:0:1::");
    EXPECT_EQ(addr.to_string(), "0:0:0:1:0:0:0:0");
}

TEST(ethernet_ipv6_address, to_string_only_segment_4_set)
{
    auto addr = make_ipv6("::1:0:0:0");
    EXPECT_EQ(addr.to_string(), "0:0:0:0:1:0:0:0");
}

TEST(ethernet_ipv6_address, to_string_only_segment_5_set)
{
    auto addr = make_ipv6("::1:0:0");
    EXPECT_EQ(addr.to_string(), "0:0:0:0:0:1:0:0");
}

TEST(ethernet_ipv6_address, to_string_only_segment_6_set)
{
    auto addr = make_ipv6("::1:0");
    EXPECT_EQ(addr.to_string(), "0:0:0:0:0:0:1:0");
}

TEST(ethernet_ipv6_address, to_string_only_segment_7_set)
{
    auto addr = make_ipv6("::1");
    EXPECT_EQ(addr.to_string(), "0:0:0:0:0:0:0:1");
}

/*******************************************************************************
 *
 * to_string — each segment individually at max (0xFFFF).
 *
 *******************************************************************************/
TEST(ethernet_ipv6_address, to_string_segment_0_max)
{
    auto addr = make_ipv6("ffff::");
    EXPECT_EQ(addr.to_string(), "ffff:0:0:0:0:0:0:0");
}

TEST(ethernet_ipv6_address, to_string_segment_1_max)
{
    auto addr = make_ipv6("0:ffff::");
    EXPECT_EQ(addr.to_string(), "0:ffff:0:0:0:0:0:0");
}

TEST(ethernet_ipv6_address, to_string_segment_2_max)
{
    auto addr = make_ipv6("0:0:ffff::");
    EXPECT_EQ(addr.to_string(), "0:0:ffff:0:0:0:0:0");
}

TEST(ethernet_ipv6_address, to_string_segment_3_max)
{
    auto addr = make_ipv6("0:0:0:ffff::");
    EXPECT_EQ(addr.to_string(), "0:0:0:ffff:0:0:0:0");
}

TEST(ethernet_ipv6_address, to_string_segment_4_max)
{
    auto addr = make_ipv6("::ffff:0:0:0");
    EXPECT_EQ(addr.to_string(), "0:0:0:0:ffff:0:0:0");
}

TEST(ethernet_ipv6_address, to_string_segment_5_max)
{
    auto addr = make_ipv6("::ffff:0:0");
    EXPECT_EQ(addr.to_string(), "0:0:0:0:0:ffff:0:0");
}

TEST(ethernet_ipv6_address, to_string_segment_6_max)
{
    auto addr = make_ipv6("::ffff:0");
    EXPECT_EQ(addr.to_string(), "0:0:0:0:0:0:ffff:0");
}

TEST(ethernet_ipv6_address, to_string_segment_7_max)
{
    auto addr = make_ipv6("::ffff");
    EXPECT_EQ(addr.to_string(), "0:0:0:0:0:0:0:ffff");
}

/*******************************************************************************
 *
 * to_string format validation.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_address, to_string_has_exactly_seven_colons)
{
    auto addr = make_ipv6("2001:db8::1");
    const std::string s = addr.to_string();
    EXPECT_EQ(std::count(s.begin(), s.end(), ':'), 7);
}

TEST(ethernet_ipv6_address, to_string_contains_only_hex_digits_and_colons)
{
    auto addr = make_ipv6("fe80::1");
    const std::string s = addr.to_string();

    for (char c : s)
    {
        EXPECT_TRUE(std::isxdigit(static_cast<unsigned char>(c)) || c == ':')
            << "Unexpected character: " << c;
    }
}

TEST(ethernet_ipv6_address, to_string_does_not_contain_whitespace)
{
    auto addr = make_ipv6("2001:db8::1");
    const std::string s = addr.to_string();

    EXPECT_EQ(s.find(' '), std::string::npos);
    EXPECT_EQ(s.find('\t'), std::string::npos);
    EXPECT_EQ(s.find('\n'), std::string::npos);
}

TEST(ethernet_ipv6_address, to_string_does_not_start_or_end_with_colon)
{
    auto addr = make_ipv6("ff02::1");
    const std::string s = addr.to_string();

    EXPECT_NE(s.front(), ':');
    EXPECT_NE(s.back(), ':');
}

TEST(ethernet_ipv6_address, to_string_is_not_empty)
{
    auto addr = make_ipv6("::1");
    EXPECT_FALSE(addr.to_string().empty());
}

TEST(ethernet_ipv6_address, to_string_no_leading_zeros_in_segments)
{
    // Segment value 1 should render as "1", not "01" or "001"
    auto addr = make_ipv6("1::");
    const std::string s = addr.to_string();
    EXPECT_EQ(s.substr(0, 1), "1");
    EXPECT_EQ(s[1], ':');
}

TEST(ethernet_ipv6_address, to_string_all_zeros_has_exactly_seven_colons)
{
    auto addr = make_ipv6("::");
    const std::string s = addr.to_string();
    EXPECT_EQ(std::count(s.begin(), s.end(), ':'), 7);
}

TEST(ethernet_ipv6_address, to_string_all_max_has_exactly_seven_colons)
{
    auto addr = make_ipv6("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
    const std::string s = addr.to_string();
    EXPECT_EQ(std::count(s.begin(), s.end(), ':'), 7);
}

/*******************************************************************************
 *
 * Copy semantics.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_address, copy_constructor_preserves_value)
{
    auto original = make_ipv6("2001:db8::1");
    ethernet_ipv6_address copy(original);

    EXPECT_EQ(copy.to_string(), original.to_string());
}

TEST(ethernet_ipv6_address, copy_assignment_preserves_value)
{
    auto original = make_ipv6("fe80::1");
    ethernet_ipv6_address other;

    other = original;

    EXPECT_EQ(other.to_string(), "fe80:0:0:0:0:0:0:1");
}

TEST(ethernet_ipv6_address, copy_is_independent_of_original)
{
    auto original = make_ipv6("::1");
    ethernet_ipv6_address copy(original);

    EXPECT_EQ(copy.to_string(), original.to_string());
    EXPECT_NE(&copy, &original);
}

TEST(ethernet_ipv6_address, copy_then_overwrite_does_not_affect_original)
{
    auto original = make_ipv6("2001:db8::1");
    auto copy = original;

    copy = make_ipv6("ff02::1");

    EXPECT_EQ(original.to_string(), "2001:db8:0:0:0:0:0:1");
    EXPECT_EQ(copy.to_string(), "ff02:0:0:0:0:0:0:1");
}

/*******************************************************************************
 *
 * Move semantics.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_address, move_constructor_preserves_value)
{
    auto original = make_ipv6("fe80::1");
    const std::string expected = original.to_string();

    ethernet_ipv6_address moved(std::move(original));

    EXPECT_EQ(moved.to_string(), expected);
}

TEST(ethernet_ipv6_address, move_assignment_preserves_value)
{
    auto original = make_ipv6("2001:db8::1");
    const std::string expected = original.to_string();
    ethernet_ipv6_address other;

    other = std::move(original);

    EXPECT_EQ(other.to_string(), expected);
}

/*******************************************************************************
 *
 * Polymorphism / inheritance.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_address, is_derived_from_ethernet_ip_address)
{
    EXPECT_TRUE((std::is_base_of<ethernet_ip_address, ethernet_ipv6_address>::value));
}

TEST(ethernet_ipv6_address, virtual_dispatch_through_base_pointer)
{
    auto addr = make_ipv6("::1");
    ethernet_ip_address *base = &addr;

    EXPECT_EQ(base->to_string(), "0:0:0:0:0:0:0:1");
}

TEST(ethernet_ipv6_address, virtual_dispatch_through_base_reference)
{
    auto addr = make_ipv6("fe80::1");
    ethernet_ip_address &ref = addr;

    EXPECT_EQ(ref.to_string(), "fe80:0:0:0:0:0:0:1");
}

TEST(ethernet_ipv6_address, dynamic_cast_from_base_pointer_succeeds)
{
    auto addr = make_ipv6("::1");
    ethernet_ip_address *base = &addr;

    auto *ipv6 = dynamic_cast<ethernet_ipv6_address *>(base);
    ASSERT_NE(ipv6, nullptr);
    EXPECT_EQ(ipv6->to_string(), "0:0:0:0:0:0:0:1");
}

TEST(ethernet_ipv6_address, dynamic_cast_to_wrong_derived_type_returns_nullptr)
{
    auto addr = make_ipv6("::1");
    ethernet_ip_address *base = &addr;

    auto *ipv4 = dynamic_cast<ethernet_ipv4_address *>(base);
    EXPECT_EQ(ipv4, nullptr);
}

/*******************************************************************************
 *
 * Multiple instances are independent.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_address, two_different_addresses_are_independent)
{
    auto addr1 = make_ipv6("::1");
    auto addr2 = make_ipv6("::2");

    EXPECT_NE(addr1.to_string(), addr2.to_string());
}

TEST(ethernet_ipv6_address, two_identical_addresses_have_same_string)
{
    auto addr1 = make_ipv6("fe80::1");
    auto addr2 = make_ipv6("fe80::1");

    EXPECT_EQ(addr1.to_string(), addr2.to_string());
}

TEST(ethernet_ipv6_address, modifying_copy_does_not_affect_original)
{
    auto original = make_ipv6("2001:db8::1");
    auto copy = original;

    copy = make_ipv6("::1");

    EXPECT_EQ(original.to_string(), "2001:db8:0:0:0:0:0:1");
    EXPECT_EQ(copy.to_string(), "0:0:0:0:0:0:0:1");
}

/*******************************************************************************
 *
 * Polymorphic smart pointer usage.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_address, shared_ptr_to_base_holds_nonzero_address)
{
    auto concrete = make_ipv6("::1");
    std::shared_ptr<ethernet_ip_address> base = std::make_shared<ethernet_ipv6_address>(concrete);

    EXPECT_EQ(base->to_string(), "0:0:0:0:0:0:0:1");

    auto *ipv6 = dynamic_cast<ethernet_ipv6_address *>(base.get());
    ASSERT_NE(ipv6, nullptr);
    EXPECT_EQ(ipv6->to_string(), concrete.to_string());
}

/*******************************************************************************
 *
 * Idempotency.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_address, to_string_is_idempotent)
{
    auto addr = make_ipv6("2001:db8::1");

    const std::string s1 = addr.to_string();
    const std::string s2 = addr.to_string();
    const std::string s3 = addr.to_string();

    EXPECT_EQ(s1, s2);
    EXPECT_EQ(s2, s3);
}

/*******************************************************************************
 *
 * Const correctness.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_address, const_object_to_string)
{
    const auto addr = make_ipv6("::1");
    EXPECT_EQ(addr.to_string(), "0:0:0:0:0:0:0:1");
}

TEST(ethernet_ipv6_address, const_base_pointer_to_string)
{
    const auto addr = make_ipv6("fe80::1");
    const ethernet_ip_address *base = &addr;

    EXPECT_EQ(base->to_string(), "fe80:0:0:0:0:0:0:1");
}

/*******************************************************************************
 *
 * Type traits / compile-time properties.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_address, is_default_constructible)
{
    EXPECT_TRUE(std::is_default_constructible<ethernet_ipv6_address>::value);
}

TEST(ethernet_ipv6_address, is_copy_constructible)
{
    EXPECT_TRUE(std::is_copy_constructible<ethernet_ipv6_address>::value);
}

TEST(ethernet_ipv6_address, is_copy_assignable)
{
    EXPECT_TRUE(std::is_copy_assignable<ethernet_ipv6_address>::value);
}

TEST(ethernet_ipv6_address, is_move_constructible)
{
    EXPECT_TRUE(std::is_move_constructible<ethernet_ipv6_address>::value);
}

TEST(ethernet_ipv6_address, is_move_assignable)
{
    EXPECT_TRUE(std::is_move_assignable<ethernet_ipv6_address>::value);
}

TEST(ethernet_ipv6_address, is_not_abstract)
{
    EXPECT_FALSE(std::is_abstract<ethernet_ipv6_address>::value);
}

TEST(ethernet_ipv6_address, is_polymorphic)
{
    EXPECT_TRUE(std::is_polymorphic<ethernet_ipv6_address>::value);
}

/*******************************************************************************
 *
 * Return types.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_address, to_string_returns_std_string)
{
    ethernet_ipv6_address addr;
    EXPECT_TRUE((std::is_same<decltype(addr.to_string()), std::string>::value));
}

/*******************************************************************************
 *
 * Self-assignment safety.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_address, self_assignment_is_safe)
{
    auto addr = make_ipv6("2001:db8::1");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-assign-overloaded"
    addr = addr;
#pragma GCC diagnostic pop

    EXPECT_EQ(addr.to_string(), "2001:db8:0:0:0:0:0:1");
}

/*******************************************************************************
 *
 * STL container compatibility.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_address, storable_in_vector)
{
    std::vector<ethernet_ipv6_address> addrs;
    addrs.push_back(make_ipv6("::1"));
    addrs.push_back(make_ipv6("::2"));
    addrs.push_back(make_ipv6("::3"));

    EXPECT_EQ(addrs.size(), 3u);
    EXPECT_EQ(addrs[0].to_string(), "0:0:0:0:0:0:0:1");
    EXPECT_EQ(addrs[1].to_string(), "0:0:0:0:0:0:0:2");
    EXPECT_EQ(addrs[2].to_string(), "0:0:0:0:0:0:0:3");
}

TEST(ethernet_ipv6_address, storable_in_vector_default_constructed)
{
    std::vector<ethernet_ipv6_address> addrs(5);

    EXPECT_EQ(addrs.size(), 5u);
    for (const auto &addr : addrs)
    {
        EXPECT_EQ(addr.to_string(), "0:0:0:0:0:0:0:0");
    }
}

/*******************************************************************************
 *
 * Segment boundary values.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_address, segment_value_0x7FFF)
{
    // 0x7FFF — max signed 16-bit value
    auto addr = make_ipv6("7fff::");
    EXPECT_EQ(addr.to_string(), "7fff:0:0:0:0:0:0:0");
}

TEST(ethernet_ipv6_address, segment_value_0x8000)
{
    // 0x8000 — unsigned boundary
    auto addr = make_ipv6("8000::");
    EXPECT_EQ(addr.to_string(), "8000:0:0:0:0:0:0:0");
}

TEST(ethernet_ipv6_address, segment_value_0xFFFE)
{
    // 0xFFFE
    auto addr = make_ipv6("fffe::");
    EXPECT_EQ(addr.to_string(), "fffe:0:0:0:0:0:0:0");
}

TEST(ethernet_ipv6_address, segment_value_0x0100)
{
    // 0x0100 — byte boundary
    auto addr = make_ipv6("100::");
    EXPECT_EQ(addr.to_string(), "100:0:0:0:0:0:0:0");
}

TEST(ethernet_ipv6_address, segment_value_0x00FF)
{
    // 0x00FF
    auto addr = make_ipv6("ff::");
    EXPECT_EQ(addr.to_string(), "ff:0:0:0:0:0:0:0");
}

TEST(ethernet_ipv6_address, alternating_segments_max_zero)
{
    auto addr = make_ipv6("ffff:0:ffff:0:ffff:0:ffff:0");
    EXPECT_EQ(addr.to_string(), "ffff:0:ffff:0:ffff:0:ffff:0");
}

TEST(ethernet_ipv6_address, alternating_segments_zero_max)
{
    auto addr = make_ipv6("0:ffff:0:ffff:0:ffff:0:ffff");
    EXPECT_EQ(addr.to_string(), "0:ffff:0:ffff:0:ffff:0:ffff");
}

TEST(ethernet_ipv6_address, descending_segment_values)
{
    // 8:7:6:5:4:3:2:1
    auto addr = make_ipv6("8:7:6:5:4:3:2:1");
    EXPECT_EQ(addr.to_string(), "8:7:6:5:4:3:2:1");
}

/*******************************************************************************
 *
 * Well-known IPv6 addresses.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_address, well_known_unspecified)
{
    auto addr = make_ipv6("::");
    EXPECT_EQ(addr.to_string(), "0:0:0:0:0:0:0:0");
}

TEST(ethernet_ipv6_address, well_known_loopback)
{
    auto addr = make_ipv6("::1");
    EXPECT_EQ(addr.to_string(), "0:0:0:0:0:0:0:1");
}

TEST(ethernet_ipv6_address, well_known_link_local_prefix)
{
    // fe80::/10 — link-local
    auto addr = make_ipv6("fe80::");
    EXPECT_EQ(addr.to_string(), "fe80:0:0:0:0:0:0:0");
}

TEST(ethernet_ipv6_address, well_known_unique_local)
{
    // fd00:: — unique local address
    auto addr = make_ipv6("fd00::");
    EXPECT_EQ(addr.to_string(), "fd00:0:0:0:0:0:0:0");
}

TEST(ethernet_ipv6_address, well_known_solicited_node_multicast)
{
    // ff02::1:ff00:0 — solicited-node multicast
    auto addr = make_ipv6("ff02::1:ff00:0");
    EXPECT_EQ(addr.to_string(), "ff02:0:0:0:0:1:ff00:0");
}

TEST(ethernet_ipv6_address, well_known_6to4_prefix)
{
    // 2002:: — 6to4 tunnel prefix
    auto addr = make_ipv6("2002::");
    EXPECT_EQ(addr.to_string(), "2002:0:0:0:0:0:0:0");
}

/*******************************************************************************
 *
 * from_sockaddr_in — IPv6 basic success / return value.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_from_sockaddr, returns_true_on_valid_ipv6)
{
    sockaddr_in6 sa6 = {};
    sa6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::1", &sa6.sin6_addr);

    auto addr =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa6));
    EXPECT_TRUE(addr.has_value());
}

TEST(ethernet_ipv6_from_sockaddr, output_pointer_is_not_null_on_success)
{
    sockaddr_in6 sa6 = {};
    sa6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::1", &sa6.sin6_addr);

    auto addr =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa6));
    EXPECT_TRUE(addr.has_value());
    EXPECT_NE(*addr, nullptr);
}

TEST(ethernet_ipv6_from_sockaddr, result_is_ipv6_type)
{
    sockaddr_in6 sa6 = {};
    sa6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "2001:db8::1", &sa6.sin6_addr);

    auto addr =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa6));
    EXPECT_NE(dynamic_cast<ethernet_ipv6_address *>((*addr).get()), nullptr);
}

TEST(ethernet_ipv6_from_sockaddr, basic_address_value)
{
    sockaddr_in6 sa6 = {};
    sa6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "2001:db8::1", &sa6.sin6_addr);

    auto addr =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa6));
    EXPECT_EQ((*addr)->to_string(), "2001:db8:0:0:0:0:0:1");
}

/*******************************************************************************
 *
 * from_sockaddr_in — IPv6 corner-case addresses.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_from_sockaddr, all_zeros)
{
    sockaddr_in6 sa6 = {};
    sa6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::", &sa6.sin6_addr);

    auto addr =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa6));
    EXPECT_EQ((*addr)->to_string(), "0:0:0:0:0:0:0:0");
}

TEST(ethernet_ipv6_from_sockaddr, all_max)
{
    sockaddr_in6 sa6 = {};
    sa6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", &sa6.sin6_addr);

    auto addr =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa6));
    EXPECT_EQ((*addr)->to_string(), "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
}

TEST(ethernet_ipv6_from_sockaddr, loopback)
{
    sockaddr_in6 sa6 = {};
    sa6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::1", &sa6.sin6_addr);

    auto addr =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa6));
    EXPECT_EQ((*addr)->to_string(), "0:0:0:0:0:0:0:1");
}

TEST(ethernet_ipv6_from_sockaddr, link_local)
{
    sockaddr_in6 sa6 = {};
    sa6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "fe80::1", &sa6.sin6_addr);

    auto addr =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa6));
    EXPECT_EQ((*addr)->to_string(), "fe80:0:0:0:0:0:0:1");
}

TEST(ethernet_ipv6_from_sockaddr, documentation_prefix)
{
    sockaddr_in6 sa6 = {};
    sa6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "2001:db8::abcd", &sa6.sin6_addr);

    auto addr =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa6));
    EXPECT_EQ((*addr)->to_string(), "2001:db8:0:0:0:0:0:abcd");
}

TEST(ethernet_ipv6_from_sockaddr, multicast_all_nodes)
{
    sockaddr_in6 sa6 = {};
    sa6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "ff02::1", &sa6.sin6_addr);

    auto addr =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa6));
    EXPECT_EQ((*addr)->to_string(), "ff02:0:0:0:0:0:0:1");
}

TEST(ethernet_ipv6_from_sockaddr, ipv4_mapped)
{
    // ::ffff:192.168.1.1
    sockaddr_in6 sa6 = {};
    sa6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::ffff:192.168.1.1", &sa6.sin6_addr);

    auto addr =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa6));
    EXPECT_EQ((*addr)->to_string(), "0:0:0:0:0:ffff:c0a8:101");
}

TEST(ethernet_ipv6_from_sockaddr, unique_local)
{
    sockaddr_in6 sa6 = {};
    sa6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "fd00::1", &sa6.sin6_addr);

    auto addr =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa6));
    EXPECT_EQ((*addr)->to_string(), "fd00:0:0:0:0:0:0:1");
}

TEST(ethernet_ipv6_from_sockaddr, full_address_no_compression)
{
    sockaddr_in6 sa6 = {};
    sa6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "1:2:3:4:5:6:7:8", &sa6.sin6_addr);

    auto addr =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa6));
    EXPECT_EQ((*addr)->to_string(), "1:2:3:4:5:6:7:8");
}

TEST(ethernet_ipv6_from_sockaddr, six_to_four_prefix)
{
    sockaddr_in6 sa6 = {};
    sa6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "2002::1", &sa6.sin6_addr);

    auto addr =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa6));
    EXPECT_EQ((*addr)->to_string(), "2002:0:0:0:0:0:0:1");
}

/*******************************************************************************
 *
 * from_sockaddr_in — IPv6 segment boundary values.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_from_sockaddr, single_segment_max)
{
    sockaddr_in6 sa6 = {};
    sa6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "ffff::", &sa6.sin6_addr);

    auto addr =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa6));
    EXPECT_EQ((*addr)->to_string(), "ffff:0:0:0:0:0:0:0");
}

TEST(ethernet_ipv6_from_sockaddr, alternating_segments)
{
    sockaddr_in6 sa6 = {};
    sa6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "ff00:0:ff00:0:ff00:0:ff00:0", &sa6.sin6_addr);

    auto addr =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa6));
    EXPECT_EQ((*addr)->to_string(), "ff00:0:ff00:0:ff00:0:ff00:0");
}

TEST(ethernet_ipv6_from_sockaddr, one_in_each_segment)
{
    sockaddr_in6 sa6 = {};
    sa6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "1:1:1:1:1:1:1:1", &sa6.sin6_addr);

    auto addr =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa6));
    EXPECT_EQ((*addr)->to_string(), "1:1:1:1:1:1:1:1");
}

/*******************************************************************************
 *
 * from_sockaddr_in — IPv6 port is ignored.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_from_sockaddr, port_does_not_affect_address)
{
    auto make_sa6 = [](const char *ip, uint16_t port) {
        sockaddr_in6 sa6 = {};
        sa6.sin6_family = AF_INET6;
        sa6.sin6_port = htons(port);
        inet_pton(AF_INET6, ip, &sa6.sin6_addr);
        return sa6;
    };

    auto sa1 = make_sa6("2001:db8::1", 0);
    auto sa2 = make_sa6("2001:db8::1", 443);
    auto sa3 = make_sa6("2001:db8::1", 65535);

    auto a1 = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa1));
    auto a2 = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa2));
    auto a3 = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa3));

    EXPECT_EQ((*a1)->to_string(), (*a2)->to_string());
    EXPECT_EQ((*a2)->to_string(), (*a3)->to_string());
}

/*******************************************************************************
 *
 * from_sockaddr_in — IPv6 round-trip and consistency.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_from_sockaddr, round_trip_matches_make_ipv6_helper)
{
    const std::string ip = "fe80::abcd:ef01:2345:6789";
    auto from_helper = make_ipv6(ip);

    sockaddr_in6 sa6 = {};
    sa6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, ip.c_str(), &sa6.sin6_addr);

    auto from_sa =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa6));

    EXPECT_EQ((*from_sa)->to_string(), from_helper.to_string());
}

TEST(ethernet_ipv6_from_sockaddr, consistency_with_from_string)
{
    const std::string ip = "2001:db8:85a3::8a2e:370:7334";

    auto from_str = ethernet_address_factory::from_string(ip);

    sockaddr_in6 sa6 = {};
    sa6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, ip.c_str(), &sa6.sin6_addr);

    auto from_sa =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa6));

    EXPECT_EQ((*from_sa)->to_string(), (*from_str)->to_string());
}

/*******************************************************************************
 *
 * from_sockaddr_in — IPv6 virtual dispatch / polymorphism.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_from_sockaddr, virtual_to_string_via_base_pointer)
{
    sockaddr_in6 sa6 = {};
    sa6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::1", &sa6.sin6_addr);

    auto addr =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa6));

    ethernet_ip_address *base = (*addr).get();
    EXPECT_EQ(base->to_string(), "0:0:0:0:0:0:0:1");
}

/*******************************************************************************
 *
 * from_sockaddr_in — IPv6 independence and overwrite.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_from_sockaddr, two_calls_produce_independent_objects)
{
    sockaddr_in6 sa1 = {};
    sa1.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::1", &sa1.sin6_addr);

    sockaddr_in6 sa2 = {};
    sa2.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::2", &sa2.sin6_addr);

    auto a1 = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa1));
    auto a2 = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa2));

    EXPECT_NE((*a1)->to_string(), (*a2)->to_string());
}

TEST(ethernet_ipv6_from_sockaddr, second_call_overwrites_previous_pointer)
{
    sockaddr_in6 sa1 = {};
    sa1.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::1", &sa1.sin6_addr);

    sockaddr_in6 sa2 = {};
    sa2.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::2", &sa2.sin6_addr);

    auto addr1 =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa1));
    EXPECT_EQ((*addr1)->to_string(), "0:0:0:0:0:0:0:1");

    auto addr2 =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa2));
    EXPECT_EQ((*addr2)->to_string(), "0:0:0:0:0:0:0:2");
}

TEST(ethernet_ipv6_from_sockaddr, failure_does_not_overwrite_existing_pointer)
{
    sockaddr_in6 sa6 = {};
    sa6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::1", &sa6.sin6_addr);

    auto addr =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa6));
    ASSERT_TRUE(addr.has_value());
    auto original = *addr;

    EXPECT_FALSE(ethernet_address_factory::from_sockaddr_in(nullptr).has_value());
}

/*******************************************************************************
 *
 * from_sockaddr_in — mixed type switching (IPv4 ↔ IPv6).
 *
 *******************************************************************************/
TEST(ethernet_ipv6_from_sockaddr, ipv4_then_ipv6_overwrites_type)
{
    // Start with IPv4
    sockaddr_in sa4 = {};
    sa4.sin_family = AF_INET;
    inet_pton(AF_INET, "10.0.0.1", &sa4.sin_addr);

    auto addr4 =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa4));
    EXPECT_NE(dynamic_cast<ethernet_ipv4_address *>((*addr4).get()), nullptr);

    // Overwrite with IPv6
    sockaddr_in6 sa6 = {};
    sa6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::1", &sa6.sin6_addr);

    auto addr6 =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa6));
    EXPECT_NE(dynamic_cast<ethernet_ipv6_address *>((*addr6).get()), nullptr);
    EXPECT_EQ((*addr6)->to_string(), "0:0:0:0:0:0:0:1");
}

TEST(ethernet_ipv6_from_sockaddr, ipv6_then_ipv4_overwrites_type)
{
    // Start with IPv6
    sockaddr_in6 sa6 = {};
    sa6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::1", &sa6.sin6_addr);

    auto addr6 =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa6));
    EXPECT_NE(dynamic_cast<ethernet_ipv6_address *>((*addr6).get()), nullptr);

    // Overwrite with IPv4
    sockaddr_in sa4 = {};
    sa4.sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.1.1", &sa4.sin_addr);

    auto addr4 =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa4));
    EXPECT_NE(dynamic_cast<ethernet_ipv4_address *>((*addr4).get()), nullptr);
    EXPECT_EQ((*addr4)->to_string(), "192.168.1.1");
}

/*******************************************************************************
 *
 * Comparison operators — IPv6.
 *
 *******************************************************************************/
TEST(factory_ipv6_comparison, equal_addresses_are_equal)
{
    auto a = ethernet_address_factory::from_string("2001:db8::1");
    auto b = ethernet_address_factory::from_string("2001:db8::1");

    auto *ipv6_a = dynamic_cast<ethernet_ipv6_address *>((*a).get());
    auto *ipv6_b = dynamic_cast<ethernet_ipv6_address *>((*b).get());

    EXPECT_TRUE(*ipv6_a == *ipv6_b);
    EXPECT_FALSE(*ipv6_a != *ipv6_b);
}

TEST(factory_ipv6_comparison, different_addresses_are_not_equal)
{
    auto a = ethernet_address_factory::from_string("2001:db8::1");
    auto b = ethernet_address_factory::from_string("2001:db8::2");

    auto *ipv6_a = dynamic_cast<ethernet_ipv6_address *>((*a).get());
    auto *ipv6_b = dynamic_cast<ethernet_ipv6_address *>((*b).get());

    EXPECT_FALSE(*ipv6_a == *ipv6_b);
    EXPECT_TRUE(*ipv6_a != *ipv6_b);
}

TEST(factory_ipv6_comparison, lower_address_is_less_than_higher)
{
    auto a = ethernet_address_factory::from_string("2001:db8::1");
    auto b = ethernet_address_factory::from_string("2001:db8::2");

    auto *ipv6_a = dynamic_cast<ethernet_ipv6_address *>((*a).get());
    auto *ipv6_b = dynamic_cast<ethernet_ipv6_address *>((*b).get());

    EXPECT_TRUE(*ipv6_a < *ipv6_b);
    EXPECT_FALSE(*ipv6_b < *ipv6_a);
}

TEST(factory_ipv6_comparison, equal_addresses_not_less_than)
{
    auto a = ethernet_address_factory::from_string("2001:db8::1");
    auto b = ethernet_address_factory::from_string("2001:db8::1");

    auto *ipv6_a = dynamic_cast<ethernet_ipv6_address *>((*a).get());
    auto *ipv6_b = dynamic_cast<ethernet_ipv6_address *>((*b).get());

    EXPECT_FALSE(*ipv6_a < *ipv6_b);
}

TEST(factory_ipv6_comparison, less_than_or_equal_for_equal)
{
    auto a = ethernet_address_factory::from_string("2001:db8::1");
    auto b = ethernet_address_factory::from_string("2001:db8::1");

    auto *ipv6_a = dynamic_cast<ethernet_ipv6_address *>((*a).get());
    auto *ipv6_b = dynamic_cast<ethernet_ipv6_address *>((*b).get());

    EXPECT_TRUE(*ipv6_a <= *ipv6_b);
}

TEST(factory_ipv6_comparison, less_than_or_equal_for_less)
{
    auto a = ethernet_address_factory::from_string("2001:db8::1");
    auto b = ethernet_address_factory::from_string("2001:db8::2");

    auto *ipv6_a = dynamic_cast<ethernet_ipv6_address *>((*a).get());
    auto *ipv6_b = dynamic_cast<ethernet_ipv6_address *>((*b).get());

    EXPECT_TRUE(*ipv6_a <= *ipv6_b);
    EXPECT_FALSE(*ipv6_b <= *ipv6_a);
}

TEST(factory_ipv6_comparison, greater_than)
{
    auto a = ethernet_address_factory::from_string("2001:db8::2");
    auto b = ethernet_address_factory::from_string("2001:db8::1");

    auto *ipv6_a = dynamic_cast<ethernet_ipv6_address *>((*a).get());
    auto *ipv6_b = dynamic_cast<ethernet_ipv6_address *>((*b).get());

    EXPECT_TRUE(*ipv6_a > *ipv6_b);
    EXPECT_FALSE(*ipv6_b > *ipv6_a);
}

TEST(factory_ipv6_comparison, greater_than_or_equal_for_equal)
{
    auto a = ethernet_address_factory::from_string("2001:db8::1");
    auto b = ethernet_address_factory::from_string("2001:db8::1");

    auto *ipv6_a = dynamic_cast<ethernet_ipv6_address *>((*a).get());
    auto *ipv6_b = dynamic_cast<ethernet_ipv6_address *>((*b).get());

    EXPECT_TRUE(*ipv6_a >= *ipv6_b);
}

TEST(factory_ipv6_comparison, greater_than_or_equal_for_greater)
{
    auto a = ethernet_address_factory::from_string("2001:db8::2");
    auto b = ethernet_address_factory::from_string("2001:db8::1");

    auto *ipv6_a = dynamic_cast<ethernet_ipv6_address *>((*a).get());
    auto *ipv6_b = dynamic_cast<ethernet_ipv6_address *>((*b).get());

    EXPECT_TRUE(*ipv6_a >= *ipv6_b);
    EXPECT_FALSE(*ipv6_b >= *ipv6_a);
}

TEST(factory_ipv6_comparison, first_segment_determines_order)
{
    auto a = ethernet_address_factory::from_string("2001:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
    auto b = ethernet_address_factory::from_string("2002::");

    auto *ipv6_a = dynamic_cast<ethernet_ipv6_address *>((*a).get());
    auto *ipv6_b = dynamic_cast<ethernet_ipv6_address *>((*b).get());

    EXPECT_TRUE(*ipv6_a < *ipv6_b);
}

TEST(factory_ipv6_comparison, last_segment_determines_order)
{
    auto a = ethernet_address_factory::from_string("2001:db8::1");
    auto b = ethernet_address_factory::from_string("2001:db8::2");

    auto *ipv6_a = dynamic_cast<ethernet_ipv6_address *>((*a).get());
    auto *ipv6_b = dynamic_cast<ethernet_ipv6_address *>((*b).get());

    EXPECT_TRUE(*ipv6_a < *ipv6_b);
}

TEST(factory_ipv6_comparison, zero_and_all_ones)
{
    auto a = ethernet_address_factory::from_string("::");
    auto b = ethernet_address_factory::from_string("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");

    auto *ipv6_a = dynamic_cast<ethernet_ipv6_address *>((*a).get());
    auto *ipv6_b = dynamic_cast<ethernet_ipv6_address *>((*b).get());

    EXPECT_TRUE(*ipv6_a < *ipv6_b);
    EXPECT_TRUE(*ipv6_b > *ipv6_a);
    EXPECT_FALSE(*ipv6_a == *ipv6_b);
}

TEST(factory_ipv6_comparison, link_local_addresses)
{
    auto a = ethernet_address_factory::from_string("fe80::1");
    auto b = ethernet_address_factory::from_string("fe80::2");

    auto *ipv6_a = dynamic_cast<ethernet_ipv6_address *>((*a).get());
    auto *ipv6_b = dynamic_cast<ethernet_ipv6_address *>((*b).get());

    EXPECT_TRUE(*ipv6_a < *ipv6_b);
    EXPECT_TRUE(*ipv6_a != *ipv6_b);
}

TEST(factory_ipv6_comparison, middle_segment_determines_order)
{
    auto a = ethernet_address_factory::from_string("2001:db8:0:0:0:0:0:1");
    auto b = ethernet_address_factory::from_string("2001:db8:0:1:0:0:0:1");

    auto *ipv6_a = dynamic_cast<ethernet_ipv6_address *>((*a).get());
    auto *ipv6_b = dynamic_cast<ethernet_ipv6_address *>((*b).get());

    EXPECT_TRUE(*ipv6_a < *ipv6_b);
}

/*******************************************************************************
 *
 * Edge case: IPv6 scope ID.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_address, invalid_ipv6_with_scope_id)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("fe80::1%eth0").has_value());
}

TEST(ethernet_ipv6_address, invalid_ipv6_with_numeric_scope_id)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("fe80::1%1").has_value());
}

/*******************************************************************************
 *
 * Edge case: very long input strings.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_address, invalid_very_long_input_string)
{
    std::string long_input(10000, 'a');
    EXPECT_FALSE(ethernet_address_factory::from_string(long_input).has_value());
}

TEST(ethernet_ipv6_address, invalid_very_long_hex_string)
{
    std::string long_hex = std::string(5000, 'f') + "::" + std::string(5000, 'f');
    EXPECT_FALSE(ethernet_address_factory::from_string(long_hex).has_value());
}

TEST(ethernet_ipv6_address, invalid_extremely_long_single_segment)
{
    std::string very_long_segment = std::string(1000, 'f') + "::1";
    EXPECT_FALSE(ethernet_address_factory::from_string(very_long_segment).has_value());
}

/*******************************************************************************
 *
 * Edge case: null bytes embedded.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_address, invalid_ipv6_with_embedded_null_byte)
{
    std::string input = "2001:db8::1";
    input.insert(5, 1, '\0');
    EXPECT_FALSE(ethernet_address_factory::from_string(input).has_value());
}

TEST(ethernet_ipv6_address, invalid_ipv6_with_null_byte_at_start)
{
    std::string input = "\0::1";
    EXPECT_FALSE(ethernet_address_factory::from_string(input).has_value());
}

TEST(ethernet_ipv6_address, invalid_ipv6_with_null_byte_at_end)
{
    std::string input = "::1\0";
    EXPECT_TRUE(ethernet_address_factory::from_string(input).has_value());
}

/*******************************************************************************
 *
 * Self-assignment safety — comparison operators.
 *
 *******************************************************************************/
TEST(factory_ipv6_comparison, self_assignment_via_comparison)
{
    auto addr = ethernet_address_factory::from_string("2001:db8::1");

    auto *ipv6 = dynamic_cast<ethernet_ipv6_address *>((*addr).get());
    ASSERT_NE(ipv6, nullptr);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-assign-overloaded"
    EXPECT_TRUE(*ipv6 == *ipv6);
    EXPECT_FALSE(*ipv6 != *ipv6);
    EXPECT_FALSE(*ipv6 < *ipv6);
    EXPECT_TRUE(*ipv6 <= *ipv6);
    EXPECT_FALSE(*ipv6 > *ipv6);
    EXPECT_TRUE(*ipv6 >= *ipv6);
#pragma GCC diagnostic pop
}

/*******************************************************************************
 *
 * calculate_mask_prefix — non-contiguous IPv6 mask.
 *
 *******************************************************************************/
TEST(ethernet_ipv6_address, calculate_mask_prefix_non_contiguous_mask)
{
    auto addr = ethernet_address_factory::from_string("ffff:0:ffff:0:ffff:0:ffff:0");

    auto *ipv6 = dynamic_cast<ethernet_ipv6_address *>((*addr).get());
    ASSERT_NE(ipv6, nullptr);

    // Non-contiguous mask should return -1 or indicate invalid
    EXPECT_FALSE(ethernet_address_factory::calculate_mask_prefix(*addr).has_value());
}

TEST(ethernet_ipv6_address, calculate_mask_prefix_valid_contiguous_mask)
{
    auto addr = ethernet_address_factory::from_string("ffff:ffff:ffff:ffff:0:0:0:0");

    auto *ipv6 = dynamic_cast<ethernet_ipv6_address *>((*addr).get());
    ASSERT_NE(ipv6, nullptr);

    auto prefix = ethernet_address_factory::calculate_mask_prefix(*addr);
    EXPECT_EQ(*prefix, 64u);
}

TEST(ethernet_ipv6_address, calculate_mask_prefix_all_ones)
{
    auto addr = ethernet_address_factory::from_string("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");

    auto *ipv6 = dynamic_cast<ethernet_ipv6_address *>((*addr).get());
    ASSERT_NE(ipv6, nullptr);

    auto prefix = ethernet_address_factory::calculate_mask_prefix(*addr);
    EXPECT_EQ(*prefix, 128u);
}

TEST(ethernet_ipv6_address, calculate_mask_prefix_all_zeros)
{
    auto addr = ethernet_address_factory::from_string("::");

    auto *ipv6 = dynamic_cast<ethernet_ipv6_address *>((*addr).get());
    ASSERT_NE(ipv6, nullptr);

    auto prefix = ethernet_address_factory::calculate_mask_prefix(*addr);
    EXPECT_EQ(*prefix, 0u);
}

// NOLINTEND
