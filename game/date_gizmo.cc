#include "date_gizmo.h"

#include "universe.h"

#include <print>

DateGizmo::DateGizmo(Universe *universe, ui::Gizmo *parent)
    : ui::Column(parent)
    , m_dateChangedConnection(universe->dateChangedSignal.connect([this](JulianDate date) { setDate(date); }))
{
    setMargins(12.0f);

    constexpr auto kFont = "DejaVuSans.ttf";

    m_timeText = appendChild<ui::Text>();
    m_timeText->setFont(Font{kFont, 32.0f, 0});
    m_timeText->color = glm::vec4{1.0f};
    m_timeText->setAlign(ui::Align::HorizontalCenter);

    m_dateText = appendChild<ui::Text>();
    m_dateText->setFont(Font{kFont, 24.0f, 0});
    m_dateText->color = glm::vec4{1.0f};
    m_dateText->setAlign(ui::Align::HorizontalCenter);

    setDate(universe->date());
}

DateGizmo::~DateGizmo()
{
    m_dateChangedConnection.disconnect();
}

void DateGizmo::setDate(JulianDate date)
{
    m_dateText->setText(std::format("{:D}", date));
    m_timeText->setText(std::format("{:T}", date));
}

void DateGizmo::paintContents(Painter *painter, const glm::vec2 &pos, int depth) const
{
    const auto rect = RectF{pos, size()};
    painter->setColor(glm::vec4{0.0f, 0.0f, 0.0f, 0.75f});
    painter->fillRoundedRect(rect, 8.0f, depth);
}
