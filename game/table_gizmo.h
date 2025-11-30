#pragma once

#include <base/gui.h>

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
    void setBackgroundColor(const glm::vec4 &color);
    void setTextColor(const glm::vec4 &color);

    void handleMouseEnter() override;
    void handleMouseLeave() override;

private:
    void updateColumnStyles();

    std::size_t columnCount() const;
    void updateBackgroundColor(const glm::vec4 &color);
    ui::Text *cellAt(std::size_t column);

    TableGizmo *m_table{nullptr};
    bool m_hoverable{false};
    glm::vec4 m_hoveredColor{1.0f, 1.0f, 1.0f, 0.25f};
    glm::vec4 m_backgroundColor{0.0f};
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
        return row;
    }

    muslots::Signal<> columnStyleChanged;

private:
    template<typename... Args>
    void initializeRow(TableGizmoRow *row, Args &&...values)
    {
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (row->setValue(Is, std::forward<Args>(values)), ...);
        }(std::index_sequence_for<Args...>{});
    }

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
    ui::Margins m_cellMargins;

    friend class TableGizmoRow;
};
