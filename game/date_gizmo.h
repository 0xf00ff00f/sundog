#pragma once

#include "gui.h"
#include "julian_clock.h"

class DateGizmo : public ui::Column
{
public:
    explicit DateGizmo(ui::Gizmo *parent = nullptr);

    void setDate(JulianDate date);

private:
    ui::Text *m_timeText;
    ui::Text *m_dateText;
};
