#pragma once

#include <vector>

namespace UCI {
    // always-open menu that draws the icons from indicator.swf
    class IconMenu : public RE::IMenu {
    public:
        static constexpr std::string_view MENU_NAME = "UnsearchedCorpsesIndicator";

        static void Register();
        static void Show();

        IconMenu();

        void AdvanceMovie(float a_interval, std::uint32_t a_currentTime) override;

    private:
        static RE::IMenu* Create() { return new IconMenu(); }

        void Update();

        std::vector<RE::GFxValue> _icons;
    };
}
