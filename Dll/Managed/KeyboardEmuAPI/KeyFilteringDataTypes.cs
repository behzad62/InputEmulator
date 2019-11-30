using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace KeyboardEmuAPI
{
    public enum FilterMode : ushort
    {
        /// <summary>
        /// Filter nothing
        /// </summary>
        KEY_NONE = 0x0000,
        /// <summary>
        /// Filter all keys matches given predicate flag
        /// </summary>
        KEY_FLAGS = 0x0001,
        /// <summary>
        /// Filter all keys matches given predicate flag and scan code pairs
        /// </summary>
        KEY_FLAG_AND_SCANCODE = 0x0002,
        /// <summary>
        /// Filter all keys
        /// </summary>
        KEY_ALL = 0xFFFF,
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct KeyFilterData
    {
        /// <summary>
        /// The predicate flag that will be used to filter inputs
        /// </summary>
        public KeyboardKeyFlag KeyFlagPredicates;
        /// <summary>
        /// Scan code of keys which will be filtered
        /// </summary>
        public ushort ScanCode;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct KeyFiltering
    {
        /// <summary>
        /// FILTER_MODE
        /// </summary>
        public FilterMode FilterMode;
        /// <summary>
        /// Either No. of input 'KeyFilterData' or a key Flag predicate; depends on the filter mode
        /// </summary>
        public ushort FlagOrCount;
        /// <summary>
        /// Filter data entries
        /// </summary>
        public KeyFilterData[] FilterData;

        internal byte[] GetBytes()
        {
            int inputSize = 2 * sizeof(ushort);
            if (this.FilterMode == FilterMode.KEY_FLAG_AND_SCANCODE)
            {
                if (this.FilterData != null)
                {
                    if (this.FlagOrCount > this.FilterData.Length)
                        this.FlagOrCount = (ushort)this.FilterData.Length;
                }
                else this.FlagOrCount = 0;
                inputSize += this.FlagOrCount * Marshal.SizeOf(typeof(KeyFilterData));
            }

            byte[] buffer = new byte[inputSize];
            int writeIndex = 0;
            var bytes = BitConverter.GetBytes((ushort)this.FilterMode);
            Array.Copy(bytes, 0, buffer, writeIndex, bytes.Length);
            writeIndex += bytes.Length;
            bytes = BitConverter.GetBytes(this.FlagOrCount);
            Array.Copy(bytes, 0, buffer, writeIndex, bytes.Length);
            writeIndex += bytes.Length;
            if (this.FilterMode == FilterMode.KEY_FLAG_AND_SCANCODE)
                for (int i = 0; i < this.FlagOrCount; i++)
                {
                    bytes = BitConverter.GetBytes((ushort)this.FilterData[i].KeyFlagPredicates);
                    Array.Copy(bytes, 0, buffer, writeIndex, bytes.Length);
                    writeIndex += bytes.Length;
                    bytes = BitConverter.GetBytes(this.FilterData[i].ScanCode);
                    Array.Copy(bytes, 0, buffer, writeIndex, bytes.Length);
                    writeIndex += bytes.Length;
                }

            return buffer;
        }

        internal void RemoveRedundant()
        {
            if (FilterMode == FilterMode.KEY_FLAG_AND_SCANCODE && this.FilterData != null && this.FilterData.Length != 0)
            {
                this.FilterData = this.FilterData.Distinct(new KeyFilterDataComparer()).ToArray();
                this.FlagOrCount = (ushort)FilterData.Length;
            }
        }


        internal static KeyFiltering FromBuffer(byte[] buffer)
        {
            KeyFiltering filterRequest = new KeyFiltering();
            int bytesRead = 0;
            int bytesRemained = buffer.Length;
            if (bytesRemained >= sizeof(ushort))
            {
                filterRequest.FilterMode = (FilterMode)BitConverter.ToUInt16(buffer, bytesRead);
                bytesRead += sizeof(ushort);
                bytesRemained -= sizeof(ushort);
                if (bytesRemained >= sizeof(ushort))
                {
                    filterRequest.FlagOrCount = BitConverter.ToUInt16(buffer, bytesRead);
                    bytesRead += sizeof(ushort);
                    bytesRemained -= sizeof(ushort);
                    if (filterRequest.FilterMode == FilterMode.KEY_FLAG_AND_SCANCODE && bytesRemained >= filterRequest.FlagOrCount * 2 * sizeof(ushort))
                    {
                        filterRequest.FilterData = new KeyFilterData[filterRequest.FlagOrCount];
                        for (int i = 0; i < filterRequest.FlagOrCount; i++)
                        {
                            filterRequest.FilterData[i].KeyFlagPredicates = (KeyboardKeyFlag)BitConverter.ToUInt16(buffer, bytesRead);
                            bytesRead += sizeof(ushort);
                            filterRequest.FilterData[i].ScanCode = BitConverter.ToUInt16(buffer, bytesRead);
                            bytesRead += sizeof(ushort);
                        }
                    }
                }
            }

            return filterRequest;
        }
    }

    class KeyFilterDataComparer : EqualityComparer<KeyFilterData>
    {
        public override bool Equals(KeyFilterData x, KeyFilterData y)
        {
            return x.ScanCode == y.ScanCode && x.KeyFlagPredicates == y.KeyFlagPredicates;
        }

        public override int GetHashCode(KeyFilterData obj)
        {
            return obj.GetHashCode();
        }
    }


    [Flags]
    public enum KeyboardKeyFlag : ushort
    {
        KEY_NONE = 0x0000,
        KEY_ALL = 0xFFFF,
        KEY_DOWN = KeyboardKeyState.KEY_UP,
        KEY_UP = KeyboardKeyState.KEY_UP << 1,
        KEY_PRESS = KEY_UP | KEY_DOWN,
        KEY_E0 = KeyboardKeyState.KEY_E0 << 1,
        KEY_E1 = KeyboardKeyState.KEY_E1 << 1,
        KEY_TERMSRV_SET_LED = KeyboardKeyState.KEY_TERMSRV_SET_LED << 1,
        KEY_TERMSRV_SHADOW = KeyboardKeyState.KEY_TERMSRV_SHADOW << 1,
        KEY_TERMSRV_VKPACKET = KeyboardKeyState.KEY_TERMSRV_VKPACKET << 1
    };
}
