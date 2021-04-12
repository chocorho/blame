#include <widgets/Scroll.hpp>

#include <cmath>
#include <functional>

#include <styles/StyleScaleHorizontal.hpp>
#include <styles/StyleScaleVertical.hpp>

/* Constructor.
 * 
 * Generates a new Scroll widget, storing a pointer to its parent Widget and of course the underlying Console foundation.
 * Sets the height and width based on the orientation and stores all of them.
 * 
 * I'm not a fan of the overshadowed variables, but aside from that, this is pretty reasonable.
 */
Blame::Widgets::Scroll::Scroll(Blame::Console *console, Blame::Widgets::Widget *parent, Blame::Util::Orientation orientation, std::vector<Blame::Widgets::Widget *> widgets) : Widget(console, parent) {
    this->orientation = orientation;
    this->widgets = widgets;

    switch (this->orientation) {
        case Blame::Util::Orientation::HORIZONTAL:
            this->width = 8;
            this->height = 3;

            this->symbol_handle = "█";

            this->style = Blame::Styles::StyleScaleHorizontal();
            break;

        case Blame::Util::Orientation::VERTICAL:
            this->width = 1;
            this->height = 8;

            this->symbol_handle = "█";

            this->style = Blame::Styles::StyleScaleVertical();
            break;
    }

    this->handle_current = 0;

    this->handle_min = 0;
    this->handle_max = 8;

    this->colour_handle = Blame::Util::EscapeCodes::foregroundBlue();
}

/* This method is intended to UPDATE the raw_grid.
 * It really just "refreshes" this particular widget's graphical display.
 * 
 */
void Blame::Widgets::Scroll::redraw() {
    Widget::redraw();

    switch (this->orientation) {
        case Blame::Util::Orientation::HORIZONTAL:
            /* From my read of things, this for-loop checks each individual CELL
             * of the grid and, if possible, updates it.
             * 
             * I'm not yet clear on the purpose of the if-block.
             * 
             * It makes sure that the client area's LEFT index (presumably
             * a lower bound on the x-index) PLUS the "current" variable (which
             * is modified only in the move method!!!) PLUS `i` (which is just
             * our iterator traversing each integer less than the
             * handle_size) is no larger than the console width definition.
             * 
             * But here's the problem. This index very well might be larger
             * than the width, no? Judging from the variable name,
             * the width is NOT an upper-index; rather it is simply a value
             * meant to store the actual number of spaces comprising the 
             * widget (in this case a scroll bar). The width could be 30,
             * and the upper INDEX of the scroll bar could be farrr far more
             * than 30, and that would be okay. So why are we doing the
             * comparison this way?
             * 
             * 
             * */
            for (auto i = 0; i < this->handle_size; i++) {
                if (this->client_area.left + this->current + i < this->console->width) {
                    this->console->raw_grid[this->client_area.top][this->client_area.left + this->current + i] =
                            this->getCurrentColour(this->style.colours.background_content)
                            + this->colour_handle
                            + this->symbol_handle;
                }
            }
            break;

        case Blame::Util::Orientation::VERTICAL:
            /*
             * makes sure that the client area's TOP index (a lower
             * bound on the y-index) PLUS the "current" variable (which is
             * modified only in the move method!!!) PLUS `i` (which is just
             * our iterator traversing each integer less than the
             * handle_size) is no larger than the console height definition.
             * 
             * Same questions as above.
             */
            for (auto i = 0; i < this->handle_size; i++) {
                if (this->client_area.top + this->current + i < this->console->height) {
                    this->console->raw_grid[this->client_area.top + this->current + i][this->client_area.left] =
                            this->getCurrentColour(this->style.colours.background_content) + this->colour_handle + this->symbol_handle;
                }
            }
            break;
    } 
    this->is_redrawn.exchange(true);
}

void Blame::Widgets::Scroll::move(Blame::Util::Direction direction) {
    /* A raw pointer equality comparison to determine if thise MOVEMENT request
     * actually applies to this widget (i.e. whether it is currently in focus).
     * This is reasonable enough and suggests that the widgets' move() method
     * is going to be called in quick succession, even for some widgets that
     * are not currently active.
     */
    if (this != this->console->focused_widget)
        return;

    /* Okay, this one is interesting. It appears to be a way of generalizing
     * the "current cursor position"
     * between both orientation cases (Vertical and Horizontal).
     * This actually makes sense, because a Scroll-button is one-dimensional
     * and can be EITHER x-aligned or y-aligned. Nonetheless, I can't help
     * but wonder if there is a better way to implement this code (e.g.,
     * have an outer-level if- or switch-block that partitions based on
     * Orientation, instead of hastily checking it in the nested if-conditions).
     * 
     * The implication, then, is that the user may modify view_area_offset_y
     * IF AND ONLY IF the current widget is in VERTICAL orientation.
     * They may modify view_area_offset_x if and only if the current widget is
     * IF AND ONLY IF the current widget is in HORIZONTAL orientation.
     * 
     * This is counter-intuitive to me, because it appears that a widget HAS
     * a view_area_offset_x and a view_area_offset_y.
     */
    switch (direction) {
        case Blame::Util::Direction::UP:
            if (this->handle_current - 1 > this->handle_min - 1 && this->orientation == Blame::Util::Orientation::VERTICAL) {
                this->handle_current--;

                for (auto i : this->widgets) {
                    i->view_area_offset_y--;
                }
            }
            break;

        case Blame::Util::Direction::DOWN:
            if (this->handle_current + 1 < this->handle_max - 2 && this->orientation == Blame::Util::Orientation::VERTICAL) {
                this->handle_current++;

                for (auto i : this->widgets) {
                    i->view_area_offset_y++;
                }
            }
            break;

        case Blame::Util::Direction::LEFT:
            if (this->handle_current - 1 > this->handle_min - 1 && this->orientation == Blame::Util::Orientation::HORIZONTAL) {
                this->handle_current--;

                for (auto i : this->widgets) {
                    i->view_area_offset_x--;
                }
            }
            break;

        case Blame::Util::Direction::RIGHT:
            if (this->handle_current + 1 < this->handle_max && this->orientation == Blame::Util::Orientation::HORIZONTAL) {
                this->handle_current++;

                for (auto i : this->widgets) {
                    i->view_area_offset_x++;
                }
            }
            break;
    }

    /* This is in dire need of clearer documentation.
     * I believe it is updating the PLACEMENT of the scroll bar,
     * based on how far through the file (or document being displayed) we are.
     * This is based upon the ratio of handle_max to handle_size, which I
     * interpret as basically a conversion between the size of the SCROLL BAR
     * to the number of lines in the document being displayed (right?).
     * But what is handle_max? Surely it's not the max size OF the scroll bar.
     * Perhaps it's referring to a maximum INDEX? Perhaps a max line count
     * of the document being displayed, or the max index of the view_area?
     * 
     * Why does it divide handle_max (8 by default) by handle_size (not set in
     * this file)?
     * Even more confusing, why would this be multiplied by the elusive
     * handle_current which is zero by default?
     * 
     * Moral of the story:
     * `current` is set to zero at first, but over time, as the handle_current
     * value gets modified by *successive* move invocations, the `current`
     * variable can finally grow larger. But it definitely appears to be bounded
     * by zero. If that's not true, then some variables like handle_current might
     * be negative, and then I have an even more fundamental misunderstanding.
     */
    this->current = (this->handle_max / this->handle_size) * this->handle_current;

    /*
     * Finally, call the (static?) superclass method for move().
     * I'm not yet sure why.
     */
    Widget::move(direction);
}
