#include "table_gizmo.h"

#include "style_settings.h"
#include "util.h"

#include <print>

using namespace ui;

TableGizmoRow::TableGizmoRow(TableGizmo *table, Gizmo *parent)
    : Row(parent)
    , m_table(table)
{
    setSpacing(0);

    for (std::size_t i = 0; i < m_table->m_columnCount; ++i)
    {
        auto *container = appendChild<Column>();
        auto *text = container->appendChild<Text>();
        text->setFont(table->m_font);
        text->color = glm::vec4{1.0f};
    }
    updateColumnStyles();
    updateColors();

    m_columnStyleChangedConnection = m_table->columnStyleChangedSignal.connect([this] { updateColumnStyles(); });
    m_cellMarginsChangedConnection = m_table->cellMarginsChangedSignal.connect([this] { updateColumnStyles(); });
}

TableGizmoRow::~TableGizmoRow()
{
    m_columnStyleChangedConnection.disconnect();
    m_cellMarginsChangedConnection.disconnect();
}

void TableGizmoRow::updateColumnStyles()
{
    for (std::size_t column = 0; auto *child : children())
    {
        auto *container = static_cast<Column *>(child);

        assert(column < m_table->m_columnStyles.size());
        const auto &style = m_table->m_columnStyles[column];

        // margins
        container->setMargins(m_table->m_cellMargins);

        // column width
        container->setMinimumWidth(style.width);

        // text alignment
        child->childAt(0)->setAlign(style.align);

        ++column;
    }
}

void TableGizmoRow::setColor(const glm::vec4 &color)
{
    m_color = color;
    updateColors();
}

void TableGizmoRow::setHoveredColor(const glm::vec4 &color)
{
    m_hoveredColor = color;
    updateColors();
}

void TableGizmoRow::setSelectedColor(const glm::vec4 &color)
{
    m_selectedColor = color;
    updateColors();
}

void TableGizmoRow::setSelectable(bool selectable)
{
    if (selectable == m_selectable)
        return;
    m_selectable = selectable;
    updateColors();
}

void TableGizmoRow::setSelected(bool selected)
{
    if (selected == m_selected)
        return;
    m_selected = selected;
    updateColors();
}

std::size_t TableGizmoRow::columnCount() const
{
    return m_table->m_columnCount;
}

Text *TableGizmoRow::cellAt(std::size_t column)
{
    return static_cast<Text *>(childAt(column)->childAt(0));
}

void TableGizmoRow::setTextColor(const glm::vec4 &color)
{
    if (color == m_textColor)
        return;
    m_textColor = color;
    updateColors();
}

void TableGizmoRow::setSelectedTextColor(const glm::vec4 &color)
{
    if (color == m_selectedTextColor)
        return;
    m_selectedTextColor = color;
    updateColors();
}

void TableGizmoRow::setValue(std::size_t column, std::string_view value)
{
    if (auto *cell = cellAt(column))
        cell->setText(value);
}

void TableGizmoRow::setValue(std::size_t column, uint64_t value)
{
    setValue(column, formatCredits(value));
}

void TableGizmoRow::setIndent(std::size_t column, float indent)
{
    if (auto *cell = cellAt(column))
        cell->setLeft(Length::pixels(indent));
}

void TableGizmoRow::setTextColor(std::size_t column, const glm::vec4 &color)
{
    if (auto *cell = cellAt(column))
        cell->color = color;
}

void TableGizmoRow::handleHoverEnter()
{
    updateColors(true);
}

void TableGizmoRow::handleHoverLeave()
{
    updateColors(false);
}

bool TableGizmoRow::handleMousePress(const glm::vec2 &pos)
{
    return m_selectable && rect().contains(pos);
}

void TableGizmoRow::handleMouseRelease(const glm::vec2 &pos)
{
    if (rect().contains(pos))
        clickedSignal();
}

void TableGizmoRow::updateColors(bool hovered)
{
    const auto color = [this, hovered]() -> glm::vec4 {
        if (m_selectable && m_selected)
            return m_selectedColor;
        if (hovered)
            return m_hoveredColor;
        return m_color;
    }();
    if (color.w == 0.0f)
    {
        setFillBackground(false);
    }
    else
    {
        setFillBackground(true);
        backgroundColor = color;
    }

    const auto textColor = [this] {
        if (m_selectable && m_selected)
            return m_selectedTextColor;
        return m_textColor;
    }();
    for (std::size_t i = 0; i < columnCount(); ++i)
        cellAt(i)->color = textColor;
}

void TableGizmoRow::setData(std::any data)
{
    m_data = data;
}

TableGizmo::TableGizmo(std::size_t columns, Gizmo *parent)
    : Column(parent)
    , m_columnCount(columns)
    , m_columnStyles(columns)
    , m_font(g_styleSettings.normalFont)
{
    m_headerRow = appendChild<TableGizmoRow>(this);

    m_headerSeparator = appendChild<Rectangle>();
    m_headerSeparator->setFillBackground(true);
    m_headerSeparator->backgroundColor = glm::vec4{1.0f};

    m_scrollArea = appendChild<ScrollArea>();

    m_dataRows = m_scrollArea->appendChild<Column>();
    m_dataRows->setSpacing(0.0f);
    m_dataRows->resizedSignal.connect([this](const SizeF &) { updateSizes(); });
}

void TableGizmo::setHeaderSeparatorColor(const glm::vec4 &color)
{
    m_headerSeparator->backgroundColor = color;
}

void TableGizmo::setVisibleRowCount(std::size_t count)
{
    if (count == m_visibleRowCount)
        return;
    m_visibleRowCount = count;
    updateSizes();
}

void TableGizmo::setColumnWidth(std::size_t column, float width)
{
    if (column >= m_columnCount)
        return;
    if (m_columnStyles[column].width == width)
        return;
    m_columnStyles[column].width = width;
    columnStyleChangedSignal();
}

void TableGizmo::setColumnAlign(std::size_t column, Align align)
{
    if (column >= m_columnCount)
        return;
    if (m_columnStyles[column].align == align)
        return;
    m_columnStyles[column].align = align;
    columnStyleChangedSignal();
}

void TableGizmo::setCellMargins(const ui::Margins &margins)
{
    if (margins == m_cellMargins)
        return;
    m_cellMargins = margins;
    cellMarginsChangedSignal();
}

void TableGizmo::clearRows()
{
    m_dataRows->clear();
    m_selectedRow = nullptr;
}

void TableGizmo::selectRow(TableGizmoRow *row)
{
    if (row == m_selectedRow)
        return;
    if (m_selectedRow)
        m_selectedRow->setSelected(false);
    if (row->isSelectable())
    {
        row->setSelected(true);
        m_selectedRow = row;
    }
    rowSelectedSignal(row);
}

void TableGizmo::updateSizes()
{
    const auto dataRowsSize = m_dataRows->size();
    auto totalWidth = dataRowsSize.width();
    if (m_scrollArea->verticalScrollbarVisible())
        totalWidth += m_scrollArea->verticalScrollbarWidth();

    m_headerSeparator->setSize(totalWidth, 1.0f);

    const auto rowHeight = m_cellMargins.top + m_cellMargins.bottom + m_font.pixelHeight;
    auto scrollAreaHeight = rowHeight * m_visibleRowCount;
    if (m_visibleRowCount > 0)
        scrollAreaHeight += (m_visibleRowCount - 1) * m_dataRows->spacing();
    m_scrollArea->setSize(totalWidth, scrollAreaHeight);
}

std::size_t TableGizmo::rowCount() const
{
    return m_dataRows->childCount();
}

TableGizmoRow *TableGizmo::rowAt(std::size_t index) const
{
    return static_cast<TableGizmoRow *>(m_dataRows->childAt(index));
}
