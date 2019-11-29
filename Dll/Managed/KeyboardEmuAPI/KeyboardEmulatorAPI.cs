using Microsoft.Win32.SafeHandles;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Linq;
using System.Threading.Tasks;
using static KeyboardEmuAPI.NativeMethods;

namespace KeyboardEmuAPI
{
    public class KeyboardEmulatorAPI : IDisposable
    {
        private SafeFileHandle _driverHandle;
        const string deviceName = @"\\.\KeyboardEmulator";

        static uint ControlCode(uint DeviceType, uint Function, uint Method, uint Access) =>
            (DeviceType << 16) | (Access << 14) | (Function << 2) | Method;

        private static readonly uint IOCTL_KEYBOARD_GET_ATTRIBUTES = ControlCode((uint)DeviceType.FileDeviceKeyboard, 0x800, (uint)MemoryPassMode.MethodBufferred, (uint)FileAccessMode.FileReadAccess);
        private static readonly uint IOCTL_KEYBOARD_INSERT_KEY = ControlCode((uint)DeviceType.FileDeviceKeyboard, 0x801, (uint)MemoryPassMode.MethodInDirect, (uint)FileAccessMode.FileWriteAccess);
        private static readonly uint IOCTL_KEYBOARD_SET_DEVICE_ID = ControlCode((uint)DeviceType.FileDeviceKeyboard, 0x802, (uint)MemoryPassMode.MethodBufferred, (uint)FileAccessMode.FileWriteAccess);
        private static readonly uint IOCTL_KEYBOARD_GET_DEVICE_ID = ControlCode((uint)DeviceType.FileDeviceKeyboard, 0x803, (uint)MemoryPassMode.MethodBufferred, (uint)FileAccessMode.FileReadAccess);
        private static readonly uint IOCTL_KEYBOARD_DETECT_DEVICE_ID = ControlCode((uint)DeviceType.FileDeviceKeyboard, 0x804, (uint)MemoryPassMode.MethodBufferred, (uint)FileAccessMode.FileReadAccess);
        private static readonly uint IOCTL_KEYBOARD_SET_FILTER = ControlCode((uint)DeviceType.FileDeviceKeyboard, 0x805, (uint)MemoryPassMode.MethodInDirect, (uint)FileAccessMode.FileWriteAccess);
        private static readonly uint IOCTL_KEYBOARD_GET_FILTER = ControlCode((uint)DeviceType.FileDeviceKeyboard, 0x806, (uint)MemoryPassMode.MethodOutDirect, (uint)FileAccessMode.FileReadAccess);
        private static readonly uint IOCTL_KEYBOARD_SET_MODIFY = ControlCode((uint)DeviceType.FileDeviceKeyboard, 0x807, (uint)MemoryPassMode.MethodInDirect, (uint)FileAccessMode.FileWriteAccess);
        private static readonly uint IOCTL_KEYBOARD_GET_MODIFY = ControlCode((uint)DeviceType.FileDeviceKeyboard, 0x808, (uint)MemoryPassMode.MethodOutDirect, (uint)FileAccessMode.FileReadAccess);
        #region Construction
        static Lazy<KeyboardEmulatorAPI> implementation = new Lazy<KeyboardEmulatorAPI>(() => CreateInstance(), System.Threading.LazyThreadSafetyMode.PublicationOnly);
        private static KeyboardEmulatorAPI CreateInstance()
        {
            return new KeyboardEmulatorAPI();
        }
        public static KeyboardEmulatorAPI Instance { get { return implementation.Value; } }
        ~KeyboardEmulatorAPI()
        {
            Dispose(false);
        }
        private KeyboardEmulatorAPI()
        {
            disposed = false;
            _driverHandle = CreateFile(deviceName, FileAccessMask.GenericRead | FileAccessMask.GenericWrite, FileShareMode.Read,
                IntPtr.Zero, CreationDisposition.OpenExisting, CreateFileFlags.None, IntPtr.Zero);
            if (_driverHandle.IsInvalid)
            {
                throw new Win32Exception(Marshal.GetLastWin32Error());
            }
        }
        private bool disposed;
        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }
        protected virtual void Dispose(bool disposing)
        {
            if (disposed)
                return;
            if (disposing)
            {
                // get rid of managed resources
            }
            // get rid of unmanaged resources
            _driverHandle.Dispose();

            implementation = new Lazy<KeyboardEmulatorAPI>(() => CreateInstance(), System.Threading.LazyThreadSafetyMode.PublicationOnly);
            disposed = true;
        }
        #endregion

        /// <summary>
        /// Returns the number of existing keyboard devices and the current active device Id. 
        /// </summary>
        /// <returns><see cref="KeyboardDevicesQuery"/> structure containing the query data.</returns>
        public KeyboardDevicesQuery GetDevices()
        {
            KeyboardDevicesQuery queryResult;
            IntPtr outputBuffer = IntPtr.Zero;
            try
            {
                int outputSize = Marshal.SizeOf(typeof(KeyboardDevicesQuery));
                outputBuffer = Marshal.AllocHGlobal(outputSize);
                //Marshal.StructureToPtr<KeyboardDevicesQuery>(queryResult, outputBuffer, false);
                if (!DeviceIoControl(_driverHandle, IOCTL_KEYBOARD_GET_DEVICE_ID, IntPtr.Zero, 0, outputBuffer, (uint)outputSize, out uint bytesReturned, IntPtr.Zero))
                {
                    throw new Win32Exception(Marshal.GetLastWin32Error());
                }

                queryResult = (KeyboardDevicesQuery)Marshal.PtrToStructure(outputBuffer, typeof(KeyboardDevicesQuery));
            }
            finally
            {
                if (outputBuffer != IntPtr.Zero)
                    Marshal.FreeHGlobal(outputBuffer);
            }
            return queryResult;
        }

        /// <summary>
        /// When called then next key press on any keyboard will set the active device Id to
        /// the device Id of the detected keyboard.The calling thread will be blocked until
        /// input detected.
        /// </summary>
        /// <returns>Device Id of the detected device.</returns>
        public ushort KeyboardDetectDeviceId()
        {
            ushort deviceId;
            if (!DeviceIoControl(_driverHandle, IOCTL_KEYBOARD_DETECT_DEVICE_ID, IntPtr.Zero, 0, out deviceId, sizeof(ushort), out uint bytesReturned, IntPtr.Zero))
            {
                throw new Win32Exception(Marshal.GetLastWin32Error());
            }
            return deviceId;
        }
        /// <summary>
        /// When called then next key press on any keyboard will set the active device Id to
        /// the device Id of the detected keyboard.
        /// </summary>
        /// <returns>Device Id of the detected device.</returns>
        public async Task<ushort> KeyboardDetectDeviceIdAsync()
        {
            return await Task.Factory.StartNew(() =>
            {
                return KeyboardDetectDeviceId();
            }, TaskCreationOptions.LongRunning);
        }

        /// <summary>
        /// Sets the active device to the given Id. Active device is a device that this API operates on.
        /// </summary>
        /// <param name="deviceId">Target keyboard device id. This value should be in valied range returned by the 'KeyboardGetDevices'.</param>
        public void KeyboardSetActiveDevice(ushort deviceId)
        {
            if (!DeviceIoControl(_driverHandle, IOCTL_KEYBOARD_SET_DEVICE_ID, ref deviceId, sizeof(ushort), IntPtr.Zero, 0, out uint bytesReturned, IntPtr.Zero))
            {
                throw new Win32Exception(Marshal.GetLastWin32Error());
            }
        }

        /// <summary>
        /// Sets the key filtering for the currently active device.
        /// </summary>
        /// <param name="filterRequest">Structure that contains the key filtering data.</param>
        public void KeyboardSetFiltering(KeyFiltering filterRequest)
        {
            if (filterRequest.FilterMode == FilterMode.KEY_NONE || filterRequest.FilterMode == FilterMode.KEY_ALL)
            {
                ushort filterMode = (ushort)filterRequest.FilterMode;
                if (!DeviceIoControl(_driverHandle, IOCTL_KEYBOARD_SET_FILTER, ref filterMode, sizeof(ushort), IntPtr.Zero, 0, out uint bytesReturned, IntPtr.Zero))
                {
                    throw new Win32Exception(Marshal.GetLastWin32Error());
                }
            }
            else
            {
                filterRequest.RemoveRedundant();
                var request = filterRequest.GetBytes();
                if (!DeviceIoControl(_driverHandle, IOCTL_KEYBOARD_SET_FILTER, request, (uint)request.Length, IntPtr.Zero, 0, out uint bytesReturned, IntPtr.Zero))
                {
                    throw new Win32Exception(Marshal.GetLastWin32Error());
                }
            }
        }

        /// <summary>
        /// Gets the key filtering info of the active device.
        /// </summary>
        /// <returns></returns>
        public KeyFiltering KeyboardGetKeyFiltering()
        {
            KeyFiltering filterRequest;

            int outputSize = 2 * sizeof(ushort);

            byte[] outputBuffer = new byte[outputSize];
            if (!DeviceIoControl(_driverHandle, IOCTL_KEYBOARD_GET_FILTER, IntPtr.Zero, 0, outputBuffer, (uint)outputSize, out uint bytesReturned, IntPtr.Zero))
            {
                throw new Win32Exception(Marshal.GetLastWin32Error());
            }

            if (bytesReturned < 2 * sizeof(ushort))
                throw new Exception("Invalid query result from the driver.");

            filterRequest = KeyFiltering.FromBuffer(outputBuffer);

            if (filterRequest.FilterMode == FilterMode.KEY_FLAG_AND_SCANCODE && filterRequest.FlagOrCount > 0)
            {
                outputSize += filterRequest.FlagOrCount * Marshal.SizeOf(typeof(KeyFilterData));
                outputBuffer = new byte[outputSize];
                if (!DeviceIoControl(_driverHandle, IOCTL_KEYBOARD_GET_FILTER, IntPtr.Zero, 0, outputBuffer, (uint)outputSize, out bytesReturned, IntPtr.Zero))
                {
                    throw new Win32Exception(Marshal.GetLastWin32Error());
                }
                filterRequest = KeyFiltering.FromBuffer(outputBuffer);
            }


            return filterRequest;
        }

        /// <summary>
        /// Adds a key filtering data to the collection of key filters of the active device.
        /// If the key filtering mode of the active device is not currently set to
		///	'KEY_FLAG_SCANCODE', it will be set to this mode.
        /// </summary>
        /// <param name="filterData">Structure that contains the key filtering data to be added.</param>
        public void KeyboardAddKeyFiltering(KeyFilterData filterData)
        {
            var currentFiltering = KeyboardGetKeyFiltering();
            if (currentFiltering.FlagOrCount == 0 || currentFiltering.FilterMode != FilterMode.KEY_FLAG_AND_SCANCODE)
            {
                currentFiltering.FilterMode = FilterMode.KEY_FLAG_AND_SCANCODE;
                currentFiltering.FlagOrCount = 1;
                currentFiltering.FilterData = new KeyFilterData[1] { filterData };
                KeyboardSetFiltering(currentFiltering);
                return;
            }
            for (int i = 0; i < currentFiltering.FilterData.Length; i++)
            {
                if (currentFiltering.FilterData[i].KeyFlagPredicates == filterData.KeyFlagPredicates
                    && currentFiltering.FilterData[i].ScanCode == filterData.ScanCode)
                    return;//already exists
            }

            Array.Resize(ref currentFiltering.FilterData, currentFiltering.FilterData.Length + 1);
            currentFiltering.FilterData[currentFiltering.FilterData.Length -1] = filterData;
            currentFiltering.FlagOrCount = (ushort)currentFiltering.FilterData.Length;
            KeyboardSetFiltering(currentFiltering);
        }

        /// <summary>
        /// Removes a key filtering data from the collection of key filters of the active device.
        /// </summary>
        /// <param name="filterData">Filter data to be removed.</param>
        public void KeyboardRemoveKeyFiltering(KeyFilterData filterData)
        {
            var currentFiltering = KeyboardGetKeyFiltering();
            if (currentFiltering.FlagOrCount == 0 || currentFiltering.FilterData == null|| currentFiltering.FilterMode != FilterMode.KEY_FLAG_AND_SCANCODE)
                return;
            currentFiltering.FilterData = currentFiltering.FilterData.Where(d => d.KeyFlagPredicates != filterData.KeyFlagPredicates 
                || d.ScanCode != filterData.ScanCode).ToArray();

            if(currentFiltering.FilterData.Length == 0)
            {
                currentFiltering.FilterMode = FilterMode.KEY_NONE;
            }
            KeyboardSetFiltering(currentFiltering);
        }

        /// <summary>
        /// Sets the key modifications for the currently active device.
        /// </summary>
        /// <param name="modifyRequest">Structure that contains the key modification data.</param>
        public void KeyboardSetModification(KeyModification modifyRequest)
        {
            modifyRequest.RemoveRedundant();

            if (modifyRequest.ModifyData == null || modifyRequest.ModifyData.Length == 0)
            {
                ushort modifyCount = 0;
                if (!DeviceIoControl(_driverHandle, IOCTL_KEYBOARD_SET_MODIFY, ref modifyCount, sizeof(ushort), IntPtr.Zero, 0, out uint bytesReturned, IntPtr.Zero))
                {
                    throw new Win32Exception(Marshal.GetLastWin32Error());
                }
            }
            else
            {
                var request = modifyRequest.GetBytes();
                if (!DeviceIoControl(_driverHandle, IOCTL_KEYBOARD_SET_MODIFY, request, (uint)request.Length, IntPtr.Zero, 0, out uint bytesReturned, IntPtr.Zero))
                {
                    throw new Win32Exception(Marshal.GetLastWin32Error());
                }
            }
        }

        /// <summary>
        /// Gets the key modification info of the currently active device.
        /// </summary>
        /// <returns></returns>
        public KeyModification KeyboardGetKeyModifying()
        {
            KeyModification modifyRequest = new KeyModification();
            if (!DeviceIoControl(_driverHandle, IOCTL_KEYBOARD_GET_MODIFY, IntPtr.Zero, 0, out ushort modifyCount, sizeof(ushort), out uint bytesReturned, IntPtr.Zero))
            {
                throw new Win32Exception(Marshal.GetLastWin32Error());
            }

            modifyRequest.ModifyCount = modifyCount;
            if (modifyRequest.ModifyCount > 0)
            {
                int outputSize = sizeof(ushort) + modifyRequest.ModifyCount * Marshal.SizeOf(typeof(KeyModifyData));
                byte[] outputBuffer = new byte[outputSize];
                if (!DeviceIoControl(_driverHandle, IOCTL_KEYBOARD_GET_MODIFY, IntPtr.Zero, 0, outputBuffer, (uint)outputSize, out bytesReturned, IntPtr.Zero))
                {
                    throw new Win32Exception(Marshal.GetLastWin32Error());
                }
                modifyRequest = KeyModification.FromBuffer(outputBuffer);
            }
            return modifyRequest;
        }

        /// <summary>
        /// Adds a key modification data to the collection of the key modifications of the active device.
        /// </summary>
        /// <param name="modifyData">Structure that contains the key modification data to be added.</param>
        public void KeyboardAddKeyModifying(KeyModifyData modifyData)
        {
            var currentModifications = KeyboardGetKeyModifying();
            if(currentModifications.ModifyCount == 0)
            {
                currentModifications.ModifyCount = 1;
                currentModifications.ModifyData = new KeyModifyData[1] { modifyData };
                KeyboardSetModification(currentModifications);
                return;
            }
            for (int i = 0; i < currentModifications.ModifyData.Length; i++)
            {
                if (currentModifications.ModifyData[i].KeyStatePredicates == modifyData.KeyStatePredicates
                    && currentModifications.ModifyData[i].FromScanCode == modifyData.FromScanCode
                    && currentModifications.ModifyData[i].ToScanCode == modifyData.ToScanCode)
                    return;//already exists
            }

            Array.Resize(ref currentModifications.ModifyData, currentModifications.ModifyData.Length + 1);
            currentModifications.ModifyData[currentModifications.ModifyData.Length -1] = modifyData;
            currentModifications.ModifyCount = (ushort)currentModifications.ModifyData.Length;

            KeyboardSetModification(currentModifications);
        }

        /// <summary>
        /// Removes a key modifying data from the collection of key modifications of the active device.
        /// </summary>
        /// <param name="modifyData">Structure that contains the key modification data to be removed.</param>
        public void KeyboardRemoveKeyModifying(KeyModifyData modifyData)
        {
            var currentModifications = KeyboardGetKeyModifying();
            if (currentModifications.ModifyCount == 0 || currentModifications.ModifyData == null)
                return;

            currentModifications.ModifyData = currentModifications.ModifyData.Where(d => d.KeyStatePredicates != modifyData.KeyStatePredicates 
                || d.FromScanCode != modifyData.FromScanCode 
                || d.ToScanCode != modifyData.ToScanCode).ToArray();

            KeyboardSetModification(currentModifications);
        }

        /// <summary>
        /// Insert keys to the output from the context of the active device.
        /// </summary>
        /// <param name="inputData">Array of 'KEYBOARD_INPUT_DATA' that contain the input data</param>
        public void KeyboardInsertKeys(KEYBOARD_INPUT_DATA[] inputKeys)
        {
            if (inputKeys == null)
                throw new ArgumentNullException(nameof(inputKeys));

            if (inputKeys.Length == 0)
                return;
            if (!DeviceIoControl(_driverHandle, IOCTL_KEYBOARD_INSERT_KEY, inputKeys, (uint)(inputKeys.Length * Marshal.SizeOf(typeof(KEYBOARD_INPUT_DATA))), IntPtr.Zero, 0, out uint bytesReturned, IntPtr.Zero))
            {
                throw new Win32Exception(Marshal.GetLastWin32Error());
            }
        }

        /// <summary>
        /// Gets the attributes of the active keyboard device.
        /// </summary>
        /// <returns>A structure containing the keyboard attributes.</returns>
        public KEYBOARD_ATTRIBUTES KeyboardGetAttributes()
        {
            KEYBOARD_ATTRIBUTES attributes;
            if (!DeviceIoControl(_driverHandle, IOCTL_KEYBOARD_GET_ATTRIBUTES, IntPtr.Zero, 0, out attributes, (uint)Marshal.SizeOf(typeof(KEYBOARD_ATTRIBUTES)), out uint bytesReturned, IntPtr.Zero))
            {
                throw new Win32Exception(Marshal.GetLastWin32Error());
            }
            return attributes;
        }

    }

    /// <summary>
    /// Used for creating IOCTLs
    /// </summary>
    enum MemoryPassMode : uint
    {
        MethodBufferred = 0,
        MethodInDirect = 1,
        MethodOutDirect = 2,
        MethodNeither = 3,
    }
    /// <summary>
    /// Used for creating IOCTLs
    /// </summary>
    enum FileAccessMode : uint
    {
        FileAnyAccess = 0,
        FileReadAccess = 1,
        FileWriteAccess = 2,
    }

    /// <summary>
    /// Used for creating IOCTLs
    /// </summary>
    enum DeviceType : uint
    {
        FileDeviceKeyboard = 0x0000000b,
        FileDeviceMouse = 0x0000000f
    }

}
