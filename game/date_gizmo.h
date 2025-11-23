#pragma once

#include "julian_clock.h"

#include <base/gui.h>

class DateGizmo : public ui::Column
{
public:
    explicit DateGizmo(ui::Gizmo *parent = nullptr);

    void setDate(JulianDate date);

private:
    ui::Text *m_timeText;
    ui::Text *m_dateText;
};
