#pragma once

#include <base/gui.h>

#include <utility>

class TableGizmo : public ui::Column
{
public:
    explicit TableGizmo(std::size_t columns, ui::Gizmo *parent);

    void setColumnWidth(std::size_t column, float width);
    void setColumnAlign(std::size_t column, ui::Align align);
    void setCellMargins(const ui::Margins &margins);

    template<typename... Args>
    void setHeader(Args &&...values)
    {
        initializeRow(m_headerRow, std::forward<Args>(values)...);
    }

    void clearRows();

    template<typename... Args>
    void appendRow(Args &&...values)
    {
        auto *row = m_dataRows->appendChild<ui::Row>();
        initializeRow(row, std::forward<Args>(values)...);
    }

private:
    template<typename... Args>
    void initializeRow(ui::Row *row, Args &&...values)
    {
        row->clear();
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (appendCell(row, Is, std::forward<Args>(values)), ...);
        }(std::index_sequence_for<Args...>{});
    }

    void appendCell(ui::Row *row, std::size_t column, std::string_view value);
    void appendCell(ui::Row *row, std::size_t column, uint64_t value);

    void updateChildrenSizes();

    std::size_t m_columnCount;
    ui::Row *m_headerRow{nullptr};
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
};
