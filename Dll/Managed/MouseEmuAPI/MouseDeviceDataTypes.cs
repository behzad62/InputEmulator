using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace MouseEmuAPI
{
    [StructLayout(LayoutKind.Sequential)]
    public struct MouseDevicesQuery
    {
        /// <summary>
        /// Currently active device Id.
        /// </summary>
        public ushort ActiveDeviceId;
        /// <summary>
        /// Number of existing Mouse devices
        /// </summary>
        public ushort NumberOfDevices;
    }

    public enum MouseButtonState : ushort
    {
        LEFT_BUTTON_DOWN    = 0x001,
        LEFT_BUTTON_UP      = 0x002,
        LEFT_BUTTON_PRESS   = LEFT_BUTTON_DOWN | LEFT_BUTTON_UP,
        RIGHT_BUTTON_DOWN   = 0x004,
        RIGHT_BUTTON_UP     = 0x008,
        RIGHT_BUTTON_PRESS  = RIGHT_BUTTON_DOWN | RIGHT_BUTTON_UP,
        MIDDLE_BUTTON_DOWN  = 0x010,
        MIDDLE_BUTTON_UP    = 0x020,
        MIDDLE_BUTTON_PRESS = MIDDLE_BUTTON_DOWN | MIDDLE_BUTTON_UP,

        BUTTON_1_DOWN       = LEFT_BUTTON_DOWN,
        BUTTON_1_UP         = LEFT_BUTTON_UP,
        BUTTON_1_PRESS      = BUTTON_1_DOWN | BUTTON_1_UP,
        BUTTON_2_DOWN       = RIGHT_BUTTON_DOWN,
        BUTTON_2_UP         = RIGHT_BUTTON_UP,
        BUTTON_2_PRESS      = BUTTON_2_DOWN | BUTTON_2_UP,
        BUTTON_3_DOWN       = MIDDLE_BUTTON_DOWN,
        BUTTON_3_UP         = MIDDLE_BUTTON_UP,
        BUTTON_3_PRESS      = BUTTON_3_DOWN | BUTTON_3_UP,

        BUTTON_4_DOWN       = 0x040,
        BUTTON_4_UP         = 0x080,
        BUTTON_4_PRESS      = BUTTON_4_DOWN | BUTTON_4_UP,
        BUTTON_5_DOWN       = 0x100,
        BUTTON_5_UP         = 0x200,
        BUTTON_5_PRESS      = BUTTON_5_DOWN | BUTTON_5_UP,

        MOUSE_WHEEL         = 0x400,
        MOUSE_HWHEEL        = 0x800
    };

    [StructLayout(LayoutKind.Sequential, Pack = 2)]
    public struct Mouse_Input_Data
    {

        /// <summary>
        /// Unit number.  E.g., for \Device\PointerPort0  the unit is '0',
        /// for \Device\PointerPort1 the unit is '1', and so on.
        /// </summary>
        public ushort UnitId;

        /// <summary>
        /// Specifies a bitwise OR of one or more of the mouse indicator flags.
        /// </summary>
        public MouseFlag Flags;

        /// <summary>
        /// The transition state of the mouse buttons.
        /// </summary>
        public ButtonsUnion Buttons;
        /// <summary>
        /// Specifies the raw state of the mouse buttons. The Win32 subsystem does not use this member.
        /// </summary>
        public uint RawButtons;

        /// <summary>
        /// Specifies the signed relative or absolute motion in the x direction.
        /// </summary>
        public int LastX;

        /// <summary>
        /// Specifies the signed relative or absolute motion in the y direction.
        /// </summary>
        public int LastY;

        /// <summary>
        /// Device-specific additional information for the event.
        /// </summary>
        public uint ExtraInformation;

    }

    [StructLayout(LayoutKind.Explicit)]
    public struct ButtonsUnion
    {
        /// <summary>
        /// Specifies both ButtonFlags and ButtonData values.
        /// </summary>
        [FieldOffset(0)] public uint Buttons;
        /// <summary>
        /// Specifies the transition state of the mouse buttons.
        /// </summary>
        [FieldOffset(0)] public MouseButtonState ButtonFlags;
        /// <summary>
        /// Specifies mouse wheel data, if MOUSE_WHEEL is set in ButtonFlags.
        /// </summary>
        [FieldOffset(2)] public ushort ButtonData;
    }

    [Flags]
    public enum MouseFlag : ushort
    {
        /// <summary>
        /// The LastX and LastY are set relative to the previous location.
        /// </summary>
        MOUSE_MOVE_RELATIVE = 0x000,
        /// <summary>
        /// MOUSE_MOVE_ABSOLUTE	The LastX and LastY values are set to absolute values.
        /// </summary>
        MOUSE_MOVE_ABSOLUTE = 0x001,
        /// <summary>
        /// The mouse coordinates are mapped to the virtual desktop.
        /// </summary>
        MOUSE_VIRTUAL_DESKTOP = 0x002,
        /// <summary>
        /// The mouse attributes have changed. The other data in the structure is not used.
        /// </summary>
        MOUSE_ATTRIBUTES_CHANGED = 0x004,
        /// <summary>
        /// Windows Vista and later) WM_MOUSEMOVE notification messages will not be coalesced. By default, these messages are coalesced.
        /// </summary>
        MOVE_NOCOALESCE = 0x008,
    };


    [StructLayout(LayoutKind.Sequential)]
    public struct MOUSE_ATTRIBUTES
    {

        /// <summary>
        /// Mouse ID value.  Used to distinguish between mouse types.
        /// </summary>
        public ushort MouseIdentifier;

        /// <summary>
        /// Number of buttons located on the mouse.
        /// </summary>
        public ushort NumberOfButtons;

        /// <summary>
        /// Specifies the rate at which the hardware reports mouse input
        /// (reports per second).  This may not be applicable for every mouse device.
        /// </summary>
        public ushort SampleRate;

        /// <summary>
        /// Length of the readahead buffer, in bytes.
        /// </summary>
        public uint InputDataQueueLength;

    }
}
