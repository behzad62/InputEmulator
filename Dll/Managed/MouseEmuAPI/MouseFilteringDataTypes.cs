using System;
using System.Collections.Generic;
using System.Text;

namespace MouseEmuAPI
{
    [Flags]
    public enum FilterMode : ushort
    {
        MOUSE_NONE          = 0x0000,
        MOUSE_ALL           = 0xFFFF,

        LEFT_BUTTON_DOWN    = MouseButtonState.LEFT_BUTTON_DOWN,
        LEFT_BUTTON_UP      = MouseButtonState.LEFT_BUTTON_UP,
        LEFT_PRESS          = LEFT_BUTTON_UP | LEFT_BUTTON_UP,
        RIGHT_BUTTON_DOWN   = MouseButtonState.RIGHT_BUTTON_DOWN,
        RIGHT_BUTTON_UP     = MouseButtonState.RIGHT_BUTTON_UP,
        RIGHT_PRESS         = RIGHT_BUTTON_DOWN | RIGHT_BUTTON_UP,
        MIDDLE_BUTTON_DOWN  = MouseButtonState.MIDDLE_BUTTON_DOWN,
        MIDDLE_BUTTON_UP    = MouseButtonState.MIDDLE_BUTTON_UP,
        MIDDLE_PRESS        = MIDDLE_BUTTON_DOWN | MIDDLE_BUTTON_UP,

        BUTTON_1_DOWN       = LEFT_BUTTON_DOWN,
        BUTTON_1_UP         = LEFT_BUTTON_UP,
        BUTTON_1_PRESS      = BUTTON_1_DOWN | BUTTON_1_UP,
        BUTTON_2_DOWN       = RIGHT_BUTTON_DOWN,
        BUTTON_2_UP         = RIGHT_BUTTON_UP,
        BUTTON_2_PRESS      = BUTTON_2_DOWN | BUTTON_2_UP,
        BUTTON_3_DOWN       = MIDDLE_BUTTON_DOWN,
        BUTTON_3_UP         = MIDDLE_BUTTON_UP,
        BUTTON_3_PRESS      = BUTTON_3_DOWN | BUTTON_3_UP,

        BUTTON_4_DOWN       = MouseButtonState.BUTTON_4_DOWN,
        BUTTON_4_UP         = MouseButtonState.BUTTON_4_UP,
        BUTTON_4_PRESS      = BUTTON_4_DOWN | BUTTON_4_UP,
        BUTTON_5_DOWN       = MouseButtonState.BUTTON_5_DOWN,
        BUTTON_5_UP         = MouseButtonState.BUTTON_5_UP,
        BUTTON_5_PRESS      = BUTTON_5_DOWN | BUTTON_5_UP,

        MOUSE_WHEEL         = MouseButtonState.MOUSE_WHEEL,
        MOUSE_HWHEEL        = MouseButtonState.MOUSE_HWHEEL,

        MOUSE_MOVE          = 0x1000,
    }
}
