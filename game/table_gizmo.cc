#include "table_gizmo.h"

#include <print>

TableGizmo::TableGizmo(std::size_t columns, ui::Gizmo *parent)
    : ui::Column(parent)
    , m_columnCount(columns)
    , m_columnStyles(columns)
{
    m_headerRow = appendChild<ui::Row>();

    m_headerSeparator = appendChild<ui::Rectangle>();
    m_headerSeparator->setFillBackground(true);
    m_headerSeparator->backgroundColor = glm::vec4{1.0f};

    m_scrollArea = appendChild<ui::ScrollArea>();

    m_dataRows = m_scrollArea->appendChild<ui::Column>();

    updateChildrenSizes();
}

void TableGizmo::setColumnWidth(std::size_t column, float width)
{
    if (column >= m_columnCount)
        return;
    if (m_columnStyles[column].width == width)
        return;
    m_columnStyles[column].width = width;
    updateChildrenSizes();
    // TODO update existing columns
}

void TableGizmo::setColumnAlign(std::size_t column, ui::Align align)
{
    if (column >= m_columnCount)
        return;
    if (m_columnStyles[column].align == align)
        return;
    m_columnStyles[column].align = align;
    // TODO update existing columns
}

void TableGizmo::clearRows()
{
    m_dataRows->clear();
}

void TableGizmo::updateChildrenSizes()
{
    auto totalWidth = std::ranges::fold_left(m_columnStyles, 0.0f,
                                             [](float width, const ColumnStyle &style) { return width + style.width; });
    totalWidth += m_headerRow->spacing() * (m_columnCount - 1);
    totalWidth += m_scrollArea->verticalScrollbarWidth();
    m_headerSeparator->setSize(totalWidth, 1.0f);
    m_scrollArea->setSize(totalWidth, 200.0f); // TODO: height
}

void TableGizmo::appendCell(ui::Row *row, std::size_t column, uint64_t value)
{
    appendCell(row, column, std::to_string(value));
}

void TableGizmo::appendCell(ui::Row *row, std::size_t column, std::string_view value)
{
    if (column >= m_columnCount)
        return;

    const auto &style = m_columnStyles[column];

    constexpr auto kFont = "DejaVuSans.ttf";

    auto *container = row->appendChild<Column>();
    container->setMargins(m_cellMargins);
    container->setMinimumWidth(style.width);

    auto *text = container->appendChild<ui::Text>();
    text->setAlign(style.align);
    text->setFont(Font{kFont, 20.0f, 0});
    text->setText(value);
    text->color = glm::vec4{1.0f};
}
