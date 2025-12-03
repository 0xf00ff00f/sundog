#pragma once

#include <base/gui.h>

class ButtonGizmo : public ui::Rectangle
{
public:
    explicit ButtonGizmo(std::string_view text, ui::Gizmo *parent = nullptr);
    explicit ButtonGizmo(ui::Gizmo *parent = nullptr);

    void setText(std::string_view text);
    std::string_view text() const;

    bool handleMousePress(const glm::vec2 &pos) override;
    void handleMouseRelease(const glm::vec2 &pos) override;
    void handleHoverEnter() override;
    void handleHoverLeave() override;

    muslots::Signal<> clickedSignal;

private:
    ui::Text *m_text{nullptr};
};
