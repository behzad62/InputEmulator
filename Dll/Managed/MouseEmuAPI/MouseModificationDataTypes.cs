using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace MouseEmuAPI
{
    [StructLayout(LayoutKind.Sequential)]
    public struct ButtonModifyData
    {
        /// <summary>
        /// Change button state from
        /// </summary>
        public MouseButtonState FromState;
        /// <summary>
        /// To this button state
        /// </summary>
        public MouseButtonState ToState;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 2)]
    public struct MouseModification
    {
        /// <summary>
        /// Number of modify data entries
        /// </summary>
        public ushort ModifyCount;
        /// <summary>
        /// Modify data entries
        /// </summary>
        public ButtonModifyData[] ModifyData;

        internal byte[] GetBytes()
        {
            int inputSize = sizeof(ushort);
            if (this.ModifyData != null)
            {
                if (this.ModifyCount > this.ModifyData.Length)
                    this.ModifyCount = (ushort)this.ModifyData.Length;
                inputSize += this.ModifyCount * Marshal.SizeOf(typeof(ButtonModifyData));
            }
            else
                this.ModifyCount = 0;

            byte[] buffer = new byte[inputSize];
            int writeIndex = 0;
            var bytes = BitConverter.GetBytes(this.ModifyCount);
            Array.Copy(bytes, 0, buffer, writeIndex, bytes.Length);
            writeIndex += bytes.Length;
            for (int i = 0; i < this.ModifyCount; i++)
            {
                bytes = BitConverter.GetBytes((ushort)this.ModifyData[i].FromState);
                Array.Copy(bytes, 0, buffer, writeIndex, bytes.Length);
                writeIndex += bytes.Length;
                bytes = BitConverter.GetBytes((ushort)this.ModifyData[i].ToState);
                Array.Copy(bytes, 0, buffer, writeIndex, bytes.Length);
                writeIndex += bytes.Length;
            }


            return buffer;
        }

        internal void RemoveRedundant()
        {
            if (this.ModifyData != null && this.ModifyData.Length != 0)
            {
                this.ModifyData = this.ModifyData.Where(d => d.FromState != d.ToState).Distinct(new ButtonModifyDataComparer()).ToArray();
                this.ModifyCount = (ushort)ModifyData.Length;
            }
        }

        internal static MouseModification FromBuffer(byte[] buffer)
        {
            MouseModification modifyRequest = new MouseModification();
            int bytesRead = 0;
            int bytesRemained = buffer.Length;
            if (bytesRemained >= sizeof(ushort))
            {
                modifyRequest.ModifyCount = BitConverter.ToUInt16(buffer, bytesRead);
                bytesRead += sizeof(ushort);
                bytesRemained -= sizeof(ushort);
                if (bytesRemained >= modifyRequest.ModifyCount * 2 * sizeof(ushort))
                {
                    modifyRequest.ModifyData = new ButtonModifyData[modifyRequest.ModifyCount];
                    for (int i = 0; i < modifyRequest.ModifyCount; i++)
                    {
                        modifyRequest.ModifyData[i].FromState = (MouseButtonState)BitConverter.ToUInt16(buffer, bytesRead);
                        bytesRead += sizeof(ushort);
                        modifyRequest.ModifyData[i].ToState = (MouseButtonState)BitConverter.ToUInt16(buffer, bytesRead);
                        bytesRead += sizeof(ushort);
                    }
                }
            }

            return modifyRequest;
        }

    }

    class ButtonModifyDataComparer : EqualityComparer<ButtonModifyData>
    {
        public override bool Equals(ButtonModifyData x, ButtonModifyData y)
        {
            return x.FromState == y.FromState && x.ToState == y.ToState;
        }

        public override int GetHashCode(ButtonModifyData obj)
        {
            return obj.GetHashCode();
        }
    }
}
