using Microsoft.Win32.SafeHandles;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using static MouseEmuAPI.NativeMethods;

namespace MouseEmuAPI
{
    public class MouseEmulatorAPI : IDisposable
    {
        private SafeFileHandle _driverHandle;
        const string deviceName = @"\\.\MouseEmulator";

        static uint ControlCode(uint DeviceType, uint Function, uint Method, uint Access) =>
            (DeviceType << 16) | (Access << 14) | (Function << 2) | Method;

        private static readonly uint IOCTL_MOUSE_GET_ATTRIBUTES = ControlCode((uint)DeviceType.FileDeviceMouse, 0x800, (uint)MemoryPassMode.MethodBufferred, (uint)FileAccessMode.FileReadAccess);
        private static readonly uint IOCTL_MOUSE_INSERT_KEY = ControlCode((uint)DeviceType.FileDeviceMouse, 0x801, (uint)MemoryPassMode.MethodInDirect, (uint)FileAccessMode.FileWriteAccess);
        private static readonly uint IOCTL_MOUSE_SET_DEVICE_ID = ControlCode((uint)DeviceType.FileDeviceMouse, 0x802, (uint)MemoryPassMode.MethodBufferred, (uint)FileAccessMode.FileWriteAccess);
        private static readonly uint IOCTL_MOUSE_GET_DEVICE_ID = ControlCode((uint)DeviceType.FileDeviceMouse, 0x803, (uint)MemoryPassMode.MethodBufferred, (uint)FileAccessMode.FileReadAccess);
        private static readonly uint IOCTL_MOUSE_DETECT_DEVICE_ID = ControlCode((uint)DeviceType.FileDeviceMouse, 0x804, (uint)MemoryPassMode.MethodBufferred, (uint)FileAccessMode.FileReadAccess);
        private static readonly uint IOCTL_MOUSE_SET_FILTER = ControlCode((uint)DeviceType.FileDeviceMouse, 0x805, (uint)MemoryPassMode.MethodInDirect, (uint)FileAccessMode.FileWriteAccess);
        private static readonly uint IOCTL_MOUSE_GET_FILTER = ControlCode((uint)DeviceType.FileDeviceMouse, 0x806, (uint)MemoryPassMode.MethodOutDirect, (uint)FileAccessMode.FileReadAccess);
        private static readonly uint IOCTL_MOUSE_SET_MODIFY = ControlCode((uint)DeviceType.FileDeviceMouse, 0x807, (uint)MemoryPassMode.MethodInDirect, (uint)FileAccessMode.FileWriteAccess);
        private static readonly uint IOCTL_MOUSE_GET_MODIFY = ControlCode((uint)DeviceType.FileDeviceMouse, 0x808, (uint)MemoryPassMode.MethodOutDirect, (uint)FileAccessMode.FileReadAccess);
        #region Construction
        static Lazy<MouseEmulatorAPI> implementation = new Lazy<MouseEmulatorAPI>(() => CreateInstance(), System.Threading.LazyThreadSafetyMode.PublicationOnly);
        private static MouseEmulatorAPI CreateInstance()
        {
            return new MouseEmulatorAPI();
        }
        public static MouseEmulatorAPI Instance { get { return implementation.Value; } }
        ~MouseEmulatorAPI()
        {
            Dispose(false);
        }
        private MouseEmulatorAPI()
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

            implementation = new Lazy<MouseEmulatorAPI>(() => CreateInstance(), System.Threading.LazyThreadSafetyMode.PublicationOnly);
            disposed = true;
        }
        #endregion

        /// <summary>
        /// Returns the number of existing Mouse devices and the current active device Id. 
        /// </summary>
        /// <returns><see cref="MouseDevicesQuery"/> structure containing the query data.</returns>
        public MouseDevicesQuery GetDevices()
        {
            MouseDevicesQuery queryResult;
            IntPtr outputBuffer = IntPtr.Zero;
            try
            {
                int outputSize = Marshal.SizeOf(typeof(MouseDevicesQuery));
                outputBuffer = Marshal.AllocHGlobal(outputSize);
                //Marshal.StructureToPtr<MouseDevicesQuery>(queryResult, outputBuffer, false);
                if (!DeviceIoControl(_driverHandle, IOCTL_MOUSE_GET_DEVICE_ID, IntPtr.Zero, 0, outputBuffer, (uint)outputSize, out uint bytesReturned, IntPtr.Zero))
                {
                    throw new Win32Exception(Marshal.GetLastWin32Error());
                }

                queryResult = (MouseDevicesQuery)Marshal.PtrToStructure(outputBuffer, typeof(MouseDevicesQuery));
            }
            finally
            {
                if (outputBuffer != IntPtr.Zero)
                    Marshal.FreeHGlobal(outputBuffer);
            }
            return queryResult;
        }

        /// <summary>
        /// When called then next key press on any Mouse will set the active device Id to
        /// the device Id of the detected Mouse.The calling thread will be blocked until
        /// input detected.
        /// </summary>
        /// <returns>Device Id of the detected device.</returns>
        public ushort MouseDetectDeviceId()
        {
            ushort deviceId;
            if (!DeviceIoControl(_driverHandle, IOCTL_MOUSE_DETECT_DEVICE_ID, IntPtr.Zero, 0, out deviceId, sizeof(ushort), out uint bytesReturned, IntPtr.Zero))
            {
                throw new Win32Exception(Marshal.GetLastWin32Error());
            }
            return deviceId;
        }
        /// <summary>
        /// When called then next key press on any Mouse will set the active device Id to
        /// the device Id of the detected Mouse.
        /// </summary>
        /// <returns>Device Id of the detected device.</returns>
        public async Task<ushort> MouseDetectDeviceIdAsync()
        {
            return await Task.Factory.StartNew(() =>
            {
                return MouseDetectDeviceId();
            }, TaskCreationOptions.LongRunning);
        }

        /// <summary>
        /// Sets the active device to the given Id. Active device is a device that this API operates on.
        /// </summary>
        /// <param name="deviceId">Target Mouse device id. This value should be in valied range returned by the 'MouseGetDevices'.</param>
        public void MouseSetActiveDevice(ushort deviceId)
        {
            if (!DeviceIoControl(_driverHandle, IOCTL_MOUSE_SET_DEVICE_ID, ref deviceId, sizeof(ushort), IntPtr.Zero, 0, out uint bytesReturned, IntPtr.Zero))
            {
                throw new Win32Exception(Marshal.GetLastWin32Error());
            }
        }

        /// <summary>
        /// Sets the key filtering for the currently active device.
        /// </summary>
        /// <param name="filterRequest">Structure that contains the key filtering data.</param>
        public void MouseSetFilterMode(FilterMode filterRequest)
        {
            ushort filterMode = (ushort)filterRequest;
            if (!DeviceIoControl(_driverHandle, IOCTL_MOUSE_SET_FILTER, ref filterMode, sizeof(ushort), IntPtr.Zero, 0, out uint bytesReturned, IntPtr.Zero))
            {
                throw new Win32Exception(Marshal.GetLastWin32Error());
            }
        }

        /// <summary>
        /// Gets the key filtering info of the active device.
        /// </summary>
        /// <returns></returns>
        public FilterMode MouseGetFilterMode()
        {
            ushort filterRequest;

            int outputSize = sizeof(ushort);

            if (!DeviceIoControl(_driverHandle, IOCTL_MOUSE_GET_FILTER, IntPtr.Zero, 0, out filterRequest, (uint)outputSize, out uint bytesReturned, IntPtr.Zero))
            {
                throw new Win32Exception(Marshal.GetLastWin32Error());
            }

            return (FilterMode)filterRequest;
        }


        /// <summary>
        /// Sets the button modifications for the currently active device.
        /// </summary>
        /// <param name="modifyRequest">Structure that contains the key modification data.</param>
        public void MouseSetModification(MouseModification modifyRequest)
        {
            modifyRequest.RemoveRedundant();

            if (modifyRequest.ModifyData == null || modifyRequest.ModifyData.Length == 0)
            {
                ushort modifyCount = 0;
                if (!DeviceIoControl(_driverHandle, IOCTL_MOUSE_SET_MODIFY, ref modifyCount, sizeof(ushort), IntPtr.Zero, 0, out uint bytesReturned, IntPtr.Zero))
                {
                    throw new Win32Exception(Marshal.GetLastWin32Error());
                }
            }
            else
            {
                var request = modifyRequest.GetBytes();
                if (!DeviceIoControl(_driverHandle, IOCTL_MOUSE_SET_MODIFY, request, (uint)request.Length, IntPtr.Zero, 0, out uint bytesReturned, IntPtr.Zero))
                {
                    throw new Win32Exception(Marshal.GetLastWin32Error());
                }
            }
        }

        /// <summary>
        /// Gets the button modification info of the currently active device.
        /// </summary>
        /// <returns></returns>
        public MouseModification MouseGetModifications()
        {
            MouseModification modifyRequest = new MouseModification();
            if (!DeviceIoControl(_driverHandle, IOCTL_MOUSE_GET_MODIFY, IntPtr.Zero, 0, out ushort modifyCount, sizeof(ushort), out uint bytesReturned, IntPtr.Zero))
            {
                throw new Win32Exception(Marshal.GetLastWin32Error());
            }

            modifyRequest.ModifyCount = modifyCount;
            if (modifyRequest.ModifyCount > 0)
            {
                int outputSize = sizeof(ushort) + modifyRequest.ModifyCount * Marshal.SizeOf(typeof(ButtonModifyData));
                byte[] outputBuffer = new byte[outputSize];
                if (!DeviceIoControl(_driverHandle, IOCTL_MOUSE_GET_MODIFY, IntPtr.Zero, 0, outputBuffer, (uint)outputSize, out bytesReturned, IntPtr.Zero))
                {
                    throw new Win32Exception(Marshal.GetLastWin32Error());
                }
                modifyRequest = MouseModification.FromBuffer(outputBuffer);
            }
            return modifyRequest;
        }

        /// <summary>
        /// Adds a button modification data to the collection of the key modifications of the active device.
        /// </summary>
        /// <param name="modifyData">Structure that contains the key modification data to be added.</param>
        public void MouseAddButtonModification(ButtonModifyData modifyData)
        {
            var currentModifications = MouseGetModifications();
            if (currentModifications.ModifyCount == 0)
            {
                currentModifications.ModifyCount = 1;
                currentModifications.ModifyData = new ButtonModifyData[1] { modifyData };
                MouseSetModification(currentModifications);
                return;
            }
            for (int i = 0; i < currentModifications.ModifyData.Length; i++)
            {
                if (currentModifications.ModifyData[i].FromState == modifyData.FromState
                    && currentModifications.ModifyData[i].ToState == modifyData.ToState)
                    return;//already exists
            }

            Array.Resize(ref currentModifications.ModifyData, currentModifications.ModifyData.Length + 1);
            currentModifications.ModifyData[currentModifications.ModifyData.Length - 1] = modifyData;
            currentModifications.ModifyCount = (ushort)currentModifications.ModifyData.Length;

            MouseSetModification(currentModifications);
        }

        /// <summary>
        /// Removes a button modification data from the collection of key modifications of the active device.
        /// </summary>
        /// <param name="modifyData">Structure that contains the key modification data to be removed.</param>
        public void MouseRemoveButtonModification(ButtonModifyData modifyData)
        {
            var currentModifications = MouseGetModifications();
            if (currentModifications.ModifyCount == 0 || currentModifications.ModifyData == null)
                return;

            currentModifications.ModifyData = currentModifications.ModifyData.Where(d => d.FromState != modifyData.FromState
                || d.ToState != modifyData.ToState).ToArray();

            MouseSetModification(currentModifications);
        }

        /// <summary>
        /// Insert inputs to the output from the context of the active device.
        /// </summary>
        /// <param name="inputData">Array of 'Mouse_INPUT_DATA' that contain the input data</param>
        public void MouseInsertInputs(Mouse_Input_Data[] inputKeys)
        {
            if (inputKeys == null)
                throw new ArgumentNullException(nameof(inputKeys));

            if (inputKeys.Length == 0)
                return;
            if (!DeviceIoControl(_driverHandle, IOCTL_MOUSE_INSERT_KEY, inputKeys, (uint)(inputKeys.Length * Marshal.SizeOf(typeof(Mouse_Input_Data))), IntPtr.Zero, 0, out uint bytesReturned, IntPtr.Zero))
            {
                throw new Win32Exception(Marshal.GetLastWin32Error());
            }
        }

        /// <summary>
        /// Gets the attributes of the active Mouse device.
        /// </summary>
        /// <returns>A structure containing the Mouse attributes.</returns>
        public MOUSE_ATTRIBUTES MouseGetAttributes()
        {
            MOUSE_ATTRIBUTES attributes;
            if (!DeviceIoControl(_driverHandle, IOCTL_MOUSE_GET_ATTRIBUTES, IntPtr.Zero, 0, out attributes, (uint)Marshal.SizeOf(typeof(MOUSE_ATTRIBUTES)), out uint bytesReturned, IntPtr.Zero))
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
