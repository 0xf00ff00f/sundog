#include "date_gizmo.h"

#include <print>

DateGizmo::DateGizmo(ui::Gizmo *parent)
    : ui::Column(parent)
{
    constexpr auto kFont = "DejaVuSans.ttf";

    m_timeText = appendChild<ui::Text>();
    m_timeText->setFont(Font{kFont, 32.0f, 0});
    m_timeText->color = glm::vec4{1.0f};
    m_timeText->setAlign(ui::Align::HorizontalCenter);

    m_dateText = appendChild<ui::Text>();
    m_dateText->setFont(Font{kFont, 24.0f, 0});
    m_dateText->color = glm::vec4{1.0f};
    m_dateText->setAlign(ui::Align::HorizontalCenter);
}

void DateGizmo::setDate(JulianDate date)
{
    constexpr auto kMonths =
        std::array{"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    const auto yearMonthDay = toYearMonthDay(date);
    const auto dateText =
        std::format("{} {} {}", static_cast<unsigned>(yearMonthDay.day()),
                    kMonths[static_cast<unsigned>(yearMonthDay.month()) - 1], static_cast<int>(yearMonthDay.year()));
    m_dateText->setText(dateText);

    double dummy{};
    const auto dayFraction = std::modf(date.time_since_epoch().count() - 0.5, &dummy);
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(JulianDate::duration{dayFraction});
    const auto hour = seconds.count() / (60 * 60);
    const auto minute = (seconds.count() / 60) % 60;
    const auto second = seconds.count() % 60;
    const auto timeText = std::format("{:02}:{:02}:{:02d}", hour, minute, second);
    m_timeText->setText(timeText);
}
