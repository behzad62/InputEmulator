using System;
using System.Linq;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace KeyboardEmuAPI
{
    [StructLayout(LayoutKind.Sequential)]
    public struct KeyModifyData
    {
        /// <summary>
        /// If input flag matches one of this flag bits
        /// </summary>
        public KeyboardKeyFlag KeyStatePredicates;
        /// <summary>
        /// Change scan code from
        /// </summary>
        public ushort FromScanCode;
        /// <summary>
        /// To this scan code
        /// </summary>
        public ushort ToScanCode;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 2)]
    public struct KeyModification
    {
        /// <summary>
        /// Number of modify data entries
        /// </summary>
        public ushort ModifyCount;
        /// <summary>
        /// Modify data entries
        /// </summary>
        public KeyModifyData[] ModifyData;

        internal byte[] GetBytes()
        {
            int inputSize = sizeof(ushort);
            if (this.ModifyData != null)
            {
                if (this.ModifyCount > this.ModifyData.Length)
                    this.ModifyCount = (ushort)this.ModifyData.Length;
                inputSize += this.ModifyCount * Marshal.SizeOf(typeof(KeyModifyData));
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
                bytes = BitConverter.GetBytes((ushort)this.ModifyData[i].KeyStatePredicates);
                Array.Copy(bytes, 0, buffer, writeIndex, bytes.Length);
                writeIndex += bytes.Length;
                bytes = BitConverter.GetBytes(this.ModifyData[i].FromScanCode);
                Array.Copy(bytes, 0, buffer, writeIndex, bytes.Length);
                writeIndex += bytes.Length;
                bytes = BitConverter.GetBytes(this.ModifyData[i].ToScanCode);
                Array.Copy(bytes, 0, buffer, writeIndex, bytes.Length);
                writeIndex += bytes.Length;
            }


            return buffer;
        }

        internal void RemoveRedundant()
        {
            if (this.ModifyData != null && this.ModifyData.Length != 0)
            {
                this.ModifyData = this.ModifyData.Where(d => d.FromScanCode != d.ToScanCode).Distinct(new KeyModifyDataComparer()).ToArray();
                this.ModifyCount = (ushort)ModifyData.Length;
            }
        }

        internal static KeyModification FromBuffer(byte[] buffer)
        {
            KeyModification modifyRequest = new KeyModification();
            int bytesRead = 0;
            int bytesRemained = buffer.Length;
            if (bytesRemained >= sizeof(ushort))
            {
                modifyRequest.ModifyCount = BitConverter.ToUInt16(buffer, bytesRead);
                bytesRead += sizeof(ushort);
                bytesRemained -= sizeof(ushort);
                if (bytesRemained >= modifyRequest.ModifyCount * 3 * sizeof(ushort))
                {
                    modifyRequest.ModifyData = new KeyModifyData[modifyRequest.ModifyCount];
                    for (int i = 0; i < modifyRequest.ModifyCount; i++)
                    {
                        modifyRequest.ModifyData[i].KeyStatePredicates = (KeyboardKeyFlag)BitConverter.ToUInt16(buffer, bytesRead);
                        bytesRead += sizeof(ushort);
                        modifyRequest.ModifyData[i].FromScanCode = BitConverter.ToUInt16(buffer, bytesRead);
                        bytesRead += sizeof(ushort);
                        modifyRequest.ModifyData[i].ToScanCode = BitConverter.ToUInt16(buffer, bytesRead);
                        bytesRead += sizeof(ushort);
                    }
                }
            }

            return modifyRequest;
        }

    }

    class KeyModifyDataComparer : EqualityComparer<KeyModifyData>
    {
        public override bool Equals(KeyModifyData x, KeyModifyData y)
        {
            return x.FromScanCode == y.FromScanCode && x.ToScanCode == y.ToScanCode && x.KeyStatePredicates == y.KeyStatePredicates;
        }

        public override int GetHashCode(KeyModifyData obj)
        {
            return obj.GetHashCode();
        }
    }
}
