#include "Hockey/UI/UIScreen.hpp"

namespace Hockey {

UIScreen::UIScreen(UIScreenId id) : m_Id(id) {}

UIScreenId UIScreen::Id() const {
    return m_Id;
}

bool UIScreen::IsVisible() const {
    return m_Visible;
}

void UIScreen::Show() {
    m_Visible = true;
}

void UIScreen::Hide() {
    m_Visible = false;
}

void UIScreen::Update(float) {}

} // namespace Hockey
