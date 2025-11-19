#include <catch2/catch_test_macros.hpp>

#include <rect.h>

#include <print>

TEST_CASE("rect basic", "[rect-basic]")
{
    RectF r1{glm::vec2{10.0f, 20.0f}, SizeF{30.0f, 50.0f}};

    REQUIRE(r1.width() == 30.0f);
    REQUIRE(r1.height() == 50.0f);

    REQUIRE(r1.left() == 10.0f);
    REQUIRE(r1.right() == 40.0f);
    REQUIRE(r1.top() == 20.0f);
    REQUIRE(r1.bottom() == 70.0f);
}

TEST_CASE("rect intersection", "[rect-intersection]")
{
    // intersecting rect
    {
        const RectF r1{glm::vec2{10.0f, 20.0f}, SizeF{30.0f, 50.0f}};
        const RectF r2{glm::vec2{30.0f, 40.0f}, SizeF{80.0f, 60.0f}};
        const RectF r3 = r1 & r2;

        REQUIRE(r3.left() == 30.0f);
        REQUIRE(r3.right() == 40.0f);
        REQUIRE(r3.top() == 40.0f);
        REQUIRE(r3.bottom() == 70.0f);

        REQUIRE(!r3.size().isNull());
        REQUIRE(!r3.isNull());
    }

    // disjoint rect
    {
        const RectF r1{glm::vec2{10.0f, 20.0f}, SizeF{30.0f, 50.0f}};
        const RectF r2{glm::vec2{50.0f, 40.0f}, SizeF{80.0f, 60.0f}};
        const RectF r3 = r1 & r2;

        REQUIRE(r3.width() == 0.0f);
        REQUIRE(r3.size().isNull());
        REQUIRE(r3.isNull());
    }
}
