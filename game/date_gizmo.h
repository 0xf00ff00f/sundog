#pragma once

#include "julian_clock.h"

#include <base/gui.h>

#include <muslots/muslots.h>

class Universe;

class DateGizmo : public ui::Column
{
public:
    explicit DateGizmo(Universe *universe, ui::Gizmo *parent = nullptr);
    ~DateGizmo() override;

private:
    void setDate(JulianDate date);

    ui::Text *m_timeText;
    ui::Text *m_dateText;
    muslots::Connection m_dateChangedConnection;
};
