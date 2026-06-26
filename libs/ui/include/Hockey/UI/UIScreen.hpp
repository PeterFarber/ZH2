#pragma once

#include "Hockey/UI/ClientFlow.hpp"

namespace Hockey {

class UIScreen {
public:
    explicit UIScreen(UIScreenId id);
    virtual ~UIScreen() = default;

    UIScreenId Id() const;
    bool IsVisible() const;

    virtual void Show();
    virtual void Hide();
    virtual void Update(float deltaSeconds);

private:
    UIScreenId m_Id;
    bool m_Visible = false;
};

} // namespace Hockey
