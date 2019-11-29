using Microsoft.Win32.SafeHandles;
using System;
using System.Runtime.InteropServices;

namespace MouseEmuAPI
{
    static class NativeMethods
    {
        [Flags]
        public enum FileShareMode
        {
            None = 0,
            Read = 1,
            Write = 2
        }

        public enum CreationDisposition
        {
            OpenExisting = 3
        }

        public enum CreateFileFlags
        {
            None = 0,
            Overlapped = 0x40000000
        }

        [Flags]
        public enum FileAccessMask : uint
        {
            GenericRead = 0x80000000,
            GenericWrite = 0x40000000
        }

        //[DllImport("kernel32", CharSet = CharSet.Unicode, SetLastError = true)]
        //public static extern bool DeviceIoControl(SafeFileHandle hDevice, uint controlCode, void* inputBuffer, uint inputSize,
        //void* oututBuffer, uint outputSize, out uint returned, IntPtr lpOverlapped);

        [DllImport("kernel32", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern bool DeviceIoControl(SafeFileHandle hDevice, uint controlCode, ref ushort request, uint inputSize,
                IntPtr result, uint outputSize, out uint returned, IntPtr lpOverlapped);

        [DllImport("kernel32", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern bool DeviceIoControl(SafeFileHandle hDevice, uint controlCode, IntPtr request,
            uint inputSize, out ushort result, uint outputSize, out uint returned, IntPtr lpOverlapped);

        [DllImport("kernel32", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern bool DeviceIoControl(SafeFileHandle hDevice, uint controlCode, IntPtr request,
            uint inputSize, out MOUSE_ATTRIBUTES result, uint outputSize, out uint returned, IntPtr lpOverlapped);

        [DllImport("kernel32", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern bool DeviceIoControl(SafeFileHandle hDevice, uint controlCode, Mouse_Input_Data[] inputKeys, uint inputSize,
            IntPtr result, uint outputSize, out uint returned, IntPtr lpOverlapped);

        [DllImport("kernel32", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern bool DeviceIoControl(SafeFileHandle hDevice, uint controlCode, IntPtr request,
            uint inputSize, byte[] result, uint outputSize, out uint returned, IntPtr lpOverlapped);

        [DllImport("kernel32", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern bool DeviceIoControl(SafeFileHandle hDevice, uint controlCode, byte[] request,
            uint inputSize, IntPtr result, uint outputSize, out uint returned, IntPtr lpOverlapped);

        [DllImport("kernel32", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern bool DeviceIoControl(SafeFileHandle hDevice, uint controlCode, IntPtr request, uint inputSize,
                IntPtr result, uint outputSize, out uint returned, IntPtr lpOverlapped);

        [DllImport("kernel32", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern SafeFileHandle CreateFile(string path, FileAccessMask accessMask, FileShareMode shareMode,
    IntPtr securityDescriptor, CreationDisposition disposition, CreateFileFlags flags, IntPtr hTemplateFile);
    }
}
