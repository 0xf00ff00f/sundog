#include "table_gizmo.h"

#include "style_settings.h"

#include <print>

using namespace ui;

namespace
{

std::locale thousandsSepLocale()
{
    struct ThousandsSepPunct : std::numpunct<char>
    {
        char do_thousands_sep() const override { return ','; }
        std::string do_grouping() const override { return "\3"; }
    };
    static std::locale locale{std::locale{}, new ThousandsSepPunct};
    return locale;
}

} // namespace

TableGizmoRow::TableGizmoRow(TableGizmo *table, Gizmo *parent)
    : Row(parent)
    , m_table(table)
{
    for (std::size_t i = 0; i < m_table->m_columnCount; ++i)
    {
        auto *container = appendChild<Column>();
        container->setMargins(table->m_cellMargins);

        auto *text = container->appendChild<Text>();
        text->setFont(g_styleSettings.normalFont);
        text->color = glm::vec4{1.0f};
    }
    updateColumnStyles();
    updateBackgroundColor(m_backgroundColor);

    m_table->columnStyleChanged.connect([this] { updateColumnStyles(); });
}

void TableGizmoRow::updateColumnStyles()
{
    for (std::size_t column = 0; auto *child : children())
    {
        assert(column < m_table->m_columnStyles.size());
        const auto &style = m_table->m_columnStyles[column];

        // column width
        static_cast<Column *>(child)->setMinimumWidth(style.width);

        // text alignment
        child->childAt(0)->setAlign(style.align);

        ++column;
    }
}

void TableGizmoRow::setHoveredColor(const glm::vec4 &color)
{
    m_hoveredColor = color;
}

void TableGizmoRow::setBackgroundColor(const glm::vec4 &color)
{
    m_hoveredColor = color;
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
    for (std::size_t i = 0; i < columnCount(); ++i)
        cellAt(i)->color = color;
}

void TableGizmoRow::setValue(std::size_t column, std::string_view value)
{
    if (auto *cell = cellAt(column))
        cell->setText(value);
}

void TableGizmoRow::setValue(std::size_t column, uint64_t value)
{
    setValue(column, std::format(thousandsSepLocale(), "{:L}", value));
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

void TableGizmoRow::handleMouseEnter()
{
    updateBackgroundColor(m_hoveredColor);
}

void TableGizmoRow::handleMouseLeave()
{
    updateBackgroundColor(m_backgroundColor);
}

void TableGizmoRow::updateBackgroundColor(const glm::vec4 &color)
{
    if (color.w == 0.0f)
    {
        setFillBackground(false);
    }
    else
    {
        setFillBackground(true);
        backgroundColor = color;
    }
}

TableGizmo::TableGizmo(std::size_t columns, Gizmo *parent)
    : Column(parent)
    , m_columnCount(columns)
    , m_columnStyles(columns)
{
    m_headerRow = appendChild<TableGizmoRow>(this);

    m_headerSeparator = appendChild<Rectangle>();
    m_headerSeparator->setFillBackground(true);
    m_headerSeparator->backgroundColor = glm::vec4{1.0f};

    m_scrollArea = appendChild<ScrollArea>();

    m_dataRows = m_scrollArea->appendChild<Column>();
    m_dataRows->resizedSignal.connect([this](const SizeF &size) {
        const auto totalWidth = size.width() + m_scrollArea->verticalScrollbarWidth();
        m_headerSeparator->setSize(totalWidth, 1.0f);
        m_scrollArea->setSize(totalWidth, 200.0f); // TODO: height
    });
}

void TableGizmo::setColumnWidth(std::size_t column, float width)
{
    if (column >= m_columnCount)
        return;
    if (m_columnStyles[column].width == width)
        return;
    m_columnStyles[column].width = width;
    columnStyleChanged();
}

void TableGizmo::setColumnAlign(std::size_t column, Align align)
{
    if (column >= m_columnCount)
        return;
    if (m_columnStyles[column].align == align)
        return;
    m_columnStyles[column].align = align;
    columnStyleChanged();
}

void TableGizmo::clearRows()
{
    m_dataRows->clear();
}
