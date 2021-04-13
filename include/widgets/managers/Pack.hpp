#pragma once

#include <widgets/managers/Manager.hpp>
#include <util/Direction.hpp>

namespace Blame::Widgets {
    class Widget;
}

/*
 * Not sure what this does yet...
 * It contains a List of Widgets, a direction, and an x,y pair.
 */
namespace Blame::Widgets::Managers {
    class Pack : public Blame::Widgets::Managers::Manager {
    public:
        Blame::Util::Direction direction;

        int next_x;
        int next_y;

        std::vector<Blame::Widgets::Widget *> widgets;
    };
}
