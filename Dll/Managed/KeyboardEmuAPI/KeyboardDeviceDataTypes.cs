using System.Runtime.InteropServices;

namespace KeyboardEmuAPI
{
    [StructLayout(LayoutKind.Sequential)]
    public struct KeyboardDevicesQuery
    {
        /// <summary>
        /// Currently active device Id.
        /// </summary>
        public ushort ActiveDeviceId;
        /// <summary>
        /// Number of existing keyboard devices
        /// </summary>
        public ushort NumberOfDevices;
    }


    public enum KeyboardKeyState : ushort
    {
        KEY_DOWN = 0x00,
        KEY_UP = 0x01,
        KEY_E0 = 0x02,
        KEY_E1 = 0x04,
        KEY_TERMSRV_SET_LED = 0x08,
        KEY_TERMSRV_SHADOW = 0x10,
        KEY_TERMSRV_VKPACKET = 0x20
    };

    [StructLayout(LayoutKind.Sequential, Pack = 2)]
    public struct KEYBOARD_INPUT_DATA
    {

        /// <summary>
        /// Unit number.  E.g., for \Device\KeyboardPort0 the unit is '0',
        /// for \Device\KeyboardPort1 the unit is '1', and so on.
        /// </summary>
        public ushort UnitId;

        /// <summary>
        /// The "make" scan code (key depression).
        /// </summary>
        public ushort MakeCode;

        /// <summary>
        /// The flags field indicates a "break" (key release) and other
        /// miscellaneous scan code information defined below.
        /// </summary>
        public KeyboardKeyState Flags;

        public ushort Reserved;

        /// <summary>
        /// Device-specific additional information for the event.
        /// </summary>
        public uint ExtraInformation;

    }

    [StructLayout(LayoutKind.Sequential)]
    public struct KEYBOARD_ID
    {
        public char Type;       // Keyboard type
        public char Subtype;    // Keyboard subtype (OEM-dependent value)
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct KEYBOARD_TYPEMATIC_PARAMETERS
    {

        //
        // Unit identifier.  Specifies the device unit for which this
        // request is intended.
        //

        public ushort UnitId;

        //
        // Typematic rate, in repeats per second.
        //

        public ushort Rate;

        //
        // Typematic delay, in milliseconds.
        //

        public ushort Delay;

    }

    [StructLayout(LayoutKind.Sequential)]
    public struct KEYBOARD_ATTRIBUTES
    {

        /// <summary>
        /// Keyboard ID value.  Used to distinguish between keyboard types.
        /// </summary>
        public KEYBOARD_ID KeyboardIdentifier;

        /// <summary>
        /// Scan code mode.
        /// </summary>
        public ushort KeyboardMode;

        /// <summary>
        /// Number of function keys located on the keyboard.
        /// </summary>
        public ushort NumberOfFunctionKeys;

        /// <summary>
        /// Number of LEDs located on the keyboard.
        /// </summary>
        public ushort NumberOfIndicators;

        /// <summary>
        /// Total number of keys located on the keyboard.
        /// </summary>
        public ushort NumberOfKeysTotal;

        /// <summary>
        /// Length of the typeahead buffer, in bytes.
        /// </summary>
        public uint InputDataQueueLength;

        /// <summary>
        /// Minimum allowable values of keyboard typematic rate and delay.
        /// </summary>
        public KEYBOARD_TYPEMATIC_PARAMETERS KeyRepeatMinimum;

        /// <summary>
        /// Maximum allowable values of keyboard typematic rate and delay.
        /// </summary>
        public KEYBOARD_TYPEMATIC_PARAMETERS KeyRepeatMaximum;

    }
}
