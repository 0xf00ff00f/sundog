#include <base/gui.h>

#include <catch2/catch_test_macros.hpp>

using namespace ui;

TEST_CASE("column", "[column]")
{
    Column column;

    REQUIRE(column.childCount() == 0);
    REQUIRE(column.size() == SizeF{0.0f, 0.0f});

    auto *rc1 = column.appendChild<Rectangle>(20.0f, 30.0f);
    REQUIRE(column.childCount() == 1);
    REQUIRE(column.size() == SizeF{20.0f, 30.0f});

    rc1->setSize(30.0f, 80.0f);
    REQUIRE(column.size() == SizeF{30.0f, 80.0f});

    auto *rc2 = column.appendChild<Rectangle>(120.0f, 20.0);
    REQUIRE(column.childCount() == 2);
    REQUIRE(column.spacing() == 4.0f);
    REQUIRE(column.size() == SizeF{120.0f, 104.0f});

    column.setSpacing(20.0f);
    REQUIRE(column.spacing() == 20.0f);
    REQUIRE(column.size() == SizeF{120.0f, 120.0f});

    column.removeChildAt(0);
    REQUIRE(column.childCount() == 1);
    REQUIRE(column.size() == SizeF{120.0f, 20.0f});

    column.setMargins(Margins{5.0f, 10.0f, 15.0f, 20.0f});
    REQUIRE(column.size() == SizeF{135.0f, 55.0f});

    column.setMinimumWidth(150.0f);
    REQUIRE(column.size() == SizeF{150.0f, 55.0f});

    column.setMinimumWidth(10.0f);
    REQUIRE(column.size() == SizeF{135.0f, 55.0f});
}

TEST_CASE("row", "[row]")
{
    Row row;

    REQUIRE(row.childCount() == 0);
    REQUIRE(row.size() == SizeF{0.0f, 0.0f});

    auto *rc1 = row.appendChild<Rectangle>(30.0f, 50.0f);
    REQUIRE(row.childCount() == 1);
    REQUIRE(row.size() == SizeF{30.0f, 50.0f});

    auto *rc2 = row.appendChild<Rectangle>(80.0f, 70.0f);
    REQUIRE(row.childCount() == 2);
    REQUIRE(row.spacing() == 4.0f);
    REQUIRE(row.size() == SizeF{114.0f, 70.0f});

    row.setSpacing(10.0f);
    REQUIRE(row.spacing() == 10.0f);
    REQUIRE(row.size() == SizeF{120.0f, 70.0f});

    rc2->setSize(30.0f, 20.0f);
    REQUIRE(row.size() == SizeF{70.0f, 50.0f});

    row.removeChildAt(1);
    REQUIRE(row.childCount() == 1);
    REQUIRE(row.size() == SizeF{30.0f, 50.0f});

    row.setMargins(Margins{5.0f, 10.0f, 15.0f, 20.0f});
    REQUIRE(row.size() == SizeF{45.0f, 85.0f});

    row.setMinimumHeight(120.0f);
    REQUIRE(row.size() == SizeF{45.0f, 120.0f});

    row.setMinimumHeight(10.0f);
    REQUIRE(row.size() == SizeF{45.0f, 85.0f});
}

TEST_CASE("nested layout", "[nested-layout]")
{
    Row row;

    REQUIRE(row.childCount() == 0);
    REQUIRE(row.size() == SizeF{0.0f, 0.0f});

    auto *col = row.appendChild<Column>();
    REQUIRE(row.size() == SizeF{0.0f, 0.0f});
    REQUIRE(col->size() == SizeF{0.0f, 0.0f});

    auto *rc = col->appendChild<Rectangle>(50.0f, 80.0f);
    REQUIRE(col->size() == SizeF{50.0f, 80.0f});
    REQUIRE(row.size() == SizeF{50.0f, 80.0f});

    rc->setSize(120.0f, 120.0f);
    REQUIRE(col->size() == SizeF{120.0f, 120.0f});
    REQUIRE(row.size() == SizeF{120.0f, 120.0f});

    col->setMargins(Margins{5.0f, 10.0f, 15.0f, 20.0f});
    REQUIRE(row.size() == SizeF{135.0f, 155.0f});
}

TEST_CASE("gizmo positions", "[gizmo-positions]")
{
    Row row;

    REQUIRE(row.childCount() == 0);
    REQUIRE(row.globalPosition() == glm::vec2{0.0f});
    REQUIRE(row.margins() == Margins{0.0f, 0.0f, 0.0f, 0.0f});
    REQUIRE(row.spacing() == 4.0f);

    auto *rc1 = row.appendChild<Rectangle>(10.0f, 10.0f);
    REQUIRE(rc1->globalPosition() == glm::vec2{0.0f});

    row.setMargins(Margins{10.0f, 10.0f, 20.0f, 10.0f});
    REQUIRE(rc1->globalPosition() == glm::vec2{10.0f, 20.0f});

    auto *rc2 = row.appendChild<Rectangle>(10.0f, 10.0f);
    REQUIRE(rc2->globalPosition() == glm::vec2{24.0f, 20.0f});

    row.removeChildAt(0);
    REQUIRE(rc2->globalPosition() == glm::vec2{10.0f, 20.0f});
}

TEST_CASE("anchors", "[anchors]")
{
    Rectangle container{200.0f, 200.0f};

    auto *rc = container.appendChild<Rectangle>(50.0f, 50.0f);
    REQUIRE(rc->globalPosition() == glm::vec2{0.0f, 0.0f});

    rc->setAlign(Align::Bottom);
    REQUIRE(rc->globalPosition() == glm::vec2{0.0f, 150.0f});

    rc->setAlign(Align::VerticalCenter);
    REQUIRE(rc->globalPosition() == glm::vec2{0.0f, 75.0f});

    rc->setTop(0.0_px);
    REQUIRE(rc->globalPosition() == glm::vec2{0.0f, 0.0f});

    rc->setTop(10.0_px);
    REQUIRE(rc->globalPosition() == glm::vec2{0.0f, 10.0f});

    rc->setBottom(200.0_px);
    REQUIRE(rc->globalPosition() == glm::vec2{0.0f, 150.0f});

    rc->setBottom(100.0_pct);
    REQUIRE(rc->globalPosition() == glm::vec2{0.0f, 150.0f});

    rc->setBottom(80.0_px);
    REQUIRE(rc->globalPosition() == glm::vec2{0.0f, 30.0f});

    rc->setTop(0.0_px);
    rc->setLeft(0.0_px);
    REQUIRE(rc->globalPosition() == glm::vec2{0.0f, 0.0f});

    rc->setLeft(10.0_px);
    REQUIRE(rc->globalPosition() == glm::vec2{10.0f, 0.0f});

    rc->setRight(200.0_px);
    REQUIRE(rc->globalPosition() == glm::vec2{150.0f, 0.0f});

    rc->setRight(100.0_pct);
    REQUIRE(rc->globalPosition() == glm::vec2{150.0f, 0.0f});

    rc->setRight(80.0_px);
    REQUIRE(rc->globalPosition() == glm::vec2{30.0f, 0.0f});
}

TEST_CASE("layout anchors", "[layout-anchors]")
{
    Row row;
    row.setMinimumHeight(200.0f);

    auto *rc = row.appendChild<Rectangle>(50.0f, 50.0f);
    REQUIRE(rc->globalPosition() == glm::vec2{0.0f, 0.0f});

    rc->setAlign(Align::Bottom);
    REQUIRE(rc->globalPosition() == glm::vec2{0.0f, 150.0f});

    rc->setAlign(Align::VerticalCenter);
    REQUIRE(rc->globalPosition() == glm::vec2{0.0f, 75.0f});

    rc->setTop(0.0_px);
    REQUIRE(rc->globalPosition() == glm::vec2{0.0f, 0.0f});

    rc->setTop(10.0_px);
    REQUIRE(rc->globalPosition() == glm::vec2{0.0f, 10.0f});

    rc->setTop(180.0_px);
    REQUIRE(rc->globalPosition() == glm::vec2{0.0f, 150.0f});

    rc->setBottom(200.0_px);
    REQUIRE(rc->globalPosition() == glm::vec2{0.0f, 150.0f});

    rc->setBottom(100.0_pct);
    REQUIRE(rc->globalPosition() == glm::vec2{0.0f, 150.0f});

    rc->setBottom(80.0_px);
    REQUIRE(rc->globalPosition() == glm::vec2{0.0f, 30.0f});

    rc->setBottom(10.0_px);
    REQUIRE(rc->globalPosition() == glm::vec2{0.0f, 0.0f});

    row.setMargins(20.0f);
    REQUIRE(rc->globalPosition() == glm::vec2{20.0f, 20.0f});

    rc->setBottom(100.0_pct);
    REQUIRE(rc->globalPosition() == glm::vec2{20.0f, 130.0f});
}

TEST_CASE("visible", "[visible]")
{
    {
        Column column;

        REQUIRE(column.childCount() == 0);
        REQUIRE(column.size() == SizeF{0.0f, 0.0f});

        auto *rc1 = column.appendChild<Rectangle>(20.0f, 30.0f);
        REQUIRE(column.childCount() == 1);
        REQUIRE(column.size() == SizeF{20.0f, 30.0f});

        rc1->setVisible(false);
        REQUIRE(column.childCount() == 1);
        REQUIRE(column.size() == SizeF{0.0f, 0.0f});

        rc1->setVisible(true);
        REQUIRE(column.childCount() == 1);
        REQUIRE(column.size() == SizeF{20.0f, 30.0f});
    }

    {
        Row row;

        REQUIRE(row.childCount() == 0);
        REQUIRE(row.size() == SizeF{0.0f, 0.0f});

        auto *rc1 = row.appendChild<Rectangle>(20.0f, 30.0f);
        REQUIRE(row.childCount() == 1);
        REQUIRE(row.size() == SizeF{20.0f, 30.0f});

        rc1->setVisible(false);
        REQUIRE(row.childCount() == 1);
        REQUIRE(row.size() == SizeF{0.0f, 0.0f});

        rc1->setVisible(true);
        REQUIRE(row.childCount() == 1);
        REQUIRE(row.size() == SizeF{20.0f, 30.0f});
    }
}
