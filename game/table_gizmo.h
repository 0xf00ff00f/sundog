#pragma once

#include <base/gui.h>

#include <any>
#include <utility>

class TableGizmo;

class TableGizmoRow : public ui::Row
{
public:
    explicit TableGizmoRow(TableGizmo *table, ui::Gizmo *parent = nullptr);

    void setValue(std::size_t column, std::string_view value);
    void setValue(std::size_t column, uint64_t value);
    void setIndent(std::size_t column, float indent);
    void setTextColor(std::size_t column, const glm::vec4 &color);

    void setHoveredColor(const glm::vec4 &color);
    void setColor(const glm::vec4 &color);
    void setSelectedColor(const glm::vec4 &color);

    void setTextColor(const glm::vec4 &color);
    void setSelectedTextColor(const glm::vec4 &color);

    void setSelectable(bool selectable);
    bool isSelectable() const { return m_selectable; }

    void setSelected(bool selected);
    bool isSelected() const { return m_selected; }

    void setData(std::any data);
    std::any data() const { return m_data; }

    void handleHoverEnter() override;
    void handleHoverLeave() override;

    bool handleMousePress(const glm::vec2 &pos) override;
    void handleMouseRelease(const glm::vec2 &pos) override;

    muslots::Signal<> clickedSignal;

private:
    void updateColumnStyles();

    std::size_t columnCount() const;
    void updateColors(bool hovered = false);
    ui::Text *cellAt(std::size_t column);

    TableGizmo *m_table{nullptr};
    glm::vec4 m_color{0.0f};
    glm::vec4 m_hoveredColor{1.0f, 1.0f, 1.0f, 0.25f};
    glm::vec4 m_selectedColor{1.0f, 1.0f, 1.0f, 0.5f};
    glm::vec4 m_textColor{1.0f};
    glm::vec4 m_selectedTextColor{1.0f};
    bool m_selectable{false};
    bool m_selected{false};
    std::any m_data;
};

class TableGizmo : public ui::Column
{
public:
    explicit TableGizmo(std::size_t columns, ui::Gizmo *parent = nullptr);

    void setColumnWidth(std::size_t column, float width);
    void setColumnAlign(std::size_t column, ui::Align align);
    void setCellMargins(const ui::Margins &margins);

    template<typename... Args>
    void setHeader(Args &&...values)
    {
        initializeRow(m_headerRow, std::forward<Args>(values)...);
    }

    TableGizmoRow *headerRow() const { return m_headerRow; }

    void clearRows();

    template<typename... Args>
    TableGizmoRow *appendRow(Args &&...values)
    {
        auto *row = m_dataRows->appendChild<TableGizmoRow>(this);
        initializeRow(row, std::forward<Args>(values)...);
        row->clickedSignal.connect([this, row] { selectRow(row); });
        return row;
    }

    muslots::Signal<> columnStyleChangedSignal;
    muslots::Signal<> cellMarginsChangedSignal;
    muslots::Signal<const TableGizmoRow *> rowSelectedSignal;

private:
    template<typename... Args>
    void initializeRow(TableGizmoRow *row, Args &&...values)
    {
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (row->setValue(Is, std::forward<Args>(values)), ...);
        }(std::index_sequence_for<Args...>{});
    }

    void selectRow(TableGizmoRow *row);

    std::size_t m_columnCount;
    TableGizmoRow *m_headerRow{nullptr};
    ui::Rectangle *m_headerSeparator{nullptr};
    ui::ScrollArea *m_scrollArea{nullptr};
    ui::Column *m_dataRows{nullptr};
    struct ColumnStyle
    {
        float width{120.0f};
        ui::Align align{ui::Align::Left | ui::Align::VerticalCenter};
    };
    std::vector<ColumnStyle> m_columnStyles;
    ui::Margins m_cellMargins{2.0f, 2.0f, 2.0f, 2.0f};
    TableGizmoRow *m_selectedRow{nullptr};

    friend class TableGizmoRow;
};
