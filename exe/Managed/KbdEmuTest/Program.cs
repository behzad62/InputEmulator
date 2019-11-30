using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using KeyboardEmuAPI;

namespace KbdEmuTest
{
    class Program
    {
        static void Main(string[] args)
        {
            if (args.Length == 0)
            {
                PrintUsage();
                return;
            }
            Command command;
            switch (args[0].ToLower())
            {
                case "reset":
                    command = Command.Reset;
                    break;
                case "detect":
                    command = Command.Detect;
                    break;
                case "filter":
                    command = Command.SetFilter;
                    break;
                case "modify":
                    command = Command.SetModify;
                    break;
                case "show":
                    command = Command.Show;
                    break;
                case "activate":
                    command = Command.SetActive;
                    break;
                case "insert":
                    command = Command.InsertKeys;
                    break;
                case "attribs":
                    command = Command.GetAttributes;
                    break;
                default:
                    PrintUsage();
                    return;
            }
            ushort deviceId;
            switch (command)
            {
                case Command.Detect:
                    Console.WriteLine("Getting devices Id of your keyboard. Please press any key.");
                    deviceId = KeyboardEmulatorAPI.Instance.KeyboardDetectDeviceId();
                    Console.WriteLine($"Device Id is {deviceId}");
                    PrintKeyboardDevices();
                    break;
                case Command.Reset:
                    KeyboardEmulatorAPI.Instance.KeyboardSetFiltering(new KeyFiltering() { FilterMode = FilterMode.KEY_NONE });
                    KeyboardEmulatorAPI.Instance.KeyboardSetModification(new KeyModification { ModifyCount = 0 });
                    Console.WriteLine("Key filterings and modifications cleared for the active device.");
                    break;
                case Command.SetActive:
                    if (!ushort.TryParse(args[1], out deviceId))
                    {
                        Console.WriteLine($"Usage: \n {GetExeName()} active <deviceId>");
                        return;
                    }
                    Console.WriteLine($"Setting device Id to {deviceId}");
                    KeyboardEmulatorAPI.Instance.KeyboardSetActiveDevice(deviceId);
                    PrintKeyboardDevices();
                    break;
                case Command.SetFilter:
                    if (args.Length < 2)
                    {
                        PrintFilterUsage();
                        return;
                    }
                    var scanCodes = new List<ushort>();
                    if (args[1].ToLower() == "add" || args[1].ToLower() == "remove")
                    {
                        if (args.Length < 3 || !ushort.TryParse(args[2], out ushort scanCode))
                        {
                            PrintFilterUsage();
                            return;
                        }
                        if (args[1].ToLower() == "add")
                        {
                            KeyboardEmulatorAPI.Instance.KeyboardAddKeyFiltering(new KeyFilterData() { KeyFlagPredicates = KeyboardKeyFlag.KEY_PRESS, ScanCode = scanCode });
                            Console.WriteLine("Key filter added.");
                            PrintKeyFilterings();
                        }
                        else
                        {
                            KeyboardEmulatorAPI.Instance.KeyboardRemoveKeyFiltering(new KeyFilterData() { KeyFlagPredicates = KeyboardKeyFlag.KEY_PRESS, ScanCode = scanCode });
                            Console.WriteLine("Key filter removed.");
                            PrintKeyFilterings();
                        }
                    }
                    else
                    {
                        for (int i = 1; i < args.Length; i++)
                        {
                            if (!ushort.TryParse(args[i], out ushort scanCode))
                            {
                                PrintFilterUsage();
                                return;
                            }
                            scanCodes.Add(scanCode);
                        }
                        KeyFiltering filterRequest = new KeyFiltering();
                        filterRequest.FilterMode = FilterMode.KEY_FLAG_AND_SCANCODE;
                        filterRequest.FlagOrCount = (ushort)scanCodes.Count;
                        filterRequest.FilterData = new KeyFilterData[scanCodes.Count];
                        for (int i = 0; i < scanCodes.Count; i++)
                        {
                            filterRequest.FilterData[i].KeyFlagPredicates = KeyboardKeyFlag.KEY_PRESS;
                            filterRequest.FilterData[i].ScanCode = scanCodes[i];
                        }
                        KeyboardEmulatorAPI.Instance.KeyboardSetFiltering(filterRequest);
                        Console.WriteLine("Filters set.");
                        PrintKeyFilterings();
                    }
                    break;
                case Command.SetModify:
                    if (args.Length < 2)
                    {
                        PrintModifyUsage();
                        return;
                    }

                    if (args[1].ToLower() == "add" || args[1].ToLower() == "remove")
                    {
                        if (args.Length < 3)
                        {
                            PrintModifyUsage();
                            return;
                        }
                        string[] parts = args[2].Split(new[] { '-' }, StringSplitOptions.RemoveEmptyEntries);
                        if (parts.Length != 2 || !ushort.TryParse(parts[0], out ushort fromScanCode) || !ushort.TryParse(parts[1], out ushort toScanCode))
                        {
                            PrintModifyUsage();
                            return;
                        }
                        if (args[1].ToLower() == "add")
                        {
                            KeyboardEmulatorAPI.Instance.KeyboardAddKeyModifying(new KeyModifyData() { KeyStatePredicates = KeyboardKeyFlag.KEY_PRESS, FromScanCode = fromScanCode, ToScanCode = toScanCode });
                            Console.WriteLine("Key modification added.");
                            PrintKeyModifications();
                        }
                        else
                        {
                            KeyboardEmulatorAPI.Instance.KeyboardRemoveKeyModifying(new KeyModifyData() { KeyStatePredicates = KeyboardKeyFlag.KEY_PRESS, FromScanCode = fromScanCode, ToScanCode = toScanCode });
                            Console.WriteLine("Key modification removed.");
                            PrintKeyModifications();
                        }
                    }
                    else
                    {
                        var scanCodePairs = new List<Tuple<ushort, ushort>>();
                        for (int i = 1; i < args.Length; i++)
                        {
                            string[] parts = args[i].Split(new[] { '-' }, StringSplitOptions.RemoveEmptyEntries);
                            if (parts.Length != 2 || !ushort.TryParse(parts[0], out ushort fromScanCode) || !ushort.TryParse(parts[1], out ushort toScanCode))
                            {
                                PrintModifyUsage();
                                return;
                            }
                            scanCodePairs.Add(new Tuple<ushort, ushort>(fromScanCode, toScanCode));
                        }
                        KeyModification modifyRequest = new KeyModification();
                        modifyRequest.ModifyCount = (ushort)scanCodePairs.Count;
                        modifyRequest.ModifyData = new KeyModifyData[scanCodePairs.Count];
                        for (int i = 0; i < scanCodePairs.Count; i++)
                        {
                            modifyRequest.ModifyData[i].KeyStatePredicates = KeyboardKeyFlag.KEY_PRESS;
                            modifyRequest.ModifyData[i].FromScanCode = scanCodePairs[i].Item1;
                            modifyRequest.ModifyData[i].ToScanCode = scanCodePairs[i].Item2;
                        }

                        KeyboardEmulatorAPI.Instance.KeyboardSetModification(modifyRequest);
                        Console.WriteLine("Key modifications set.");
                        PrintKeyModifications();
                    }
                    break;
                case Command.InsertKeys:
                    if (args.Length < 2)
                    {
                        PrintInsertKeyUsage();
                        return;
                    }
                    List<ushort> insertScanCodes = new List<ushort>();
                    int delay = 0;
                    int startIndex = 1;
                    if (args[1].ToLower() == "delay")
                    {
                        if (args.Length < 4 || !int.TryParse(args[2], out delay))
                        {
                            PrintInsertKeyUsage();
                            return;
                        }
                        startIndex = 3;
                    }
                    for (int i = startIndex; i < args.Length; i++)
                    {
                        if (!ushort.TryParse(args[i], out ushort scanCode))
                        {
                            PrintInsertKeyUsage();
                            return;
                        }
                        insertScanCodes.Add(scanCode);
                    }
                    if (delay > 0)
                        Task.Delay(delay).Wait();
                    KEYBOARD_INPUT_DATA[] inputs = new KEYBOARD_INPUT_DATA[insertScanCodes.Count * 2];

                    for (int i = 0; i < insertScanCodes.Count; i++)
                    {
                        inputs[i * 2].Flags = KeyboardKeyState.KEY_DOWN;
                        inputs[i * 2].MakeCode = insertScanCodes[i];
                        inputs[i * 2 + 1].Flags = KeyboardKeyState.KEY_UP;
                        inputs[i * 2 + 1].MakeCode = insertScanCodes[i];
                    }
                    KeyboardEmulatorAPI.Instance.KeyboardInsertKeys(inputs);
                    break;
                case Command.Show:
                    PrintKeyboardDevices();
                    PrintKeyFilterings();
                    PrintKeyModifications();
                    break;
                case Command.GetAttributes:
                    PrintKeyboardAttribs();
                    break;
                default:
                    PrintUsage();
                    break;
            }

        }

        private static void PrintKeyboardAttribs()
        {
            Console.WriteLine("Getting keyboard attributes...");
            var attribs = KeyboardEmulatorAPI.Instance.KeyboardGetAttributes();

            Console.WriteLine($"Attributes:\n" +
                $"  KeyboardIdentifier-Type     = {attribs.KeyboardIdentifier.Type}\n" +
                $"  KeyboardIdentifier-Subtype  = {attribs.KeyboardIdentifier.Subtype}\n" +
                $"  KeyboardMode                = {attribs.KeyboardMode}\n" +
                $"  InputDataQueueLength        = {attribs.InputDataQueueLength}\n" +
                $"  NumberOfFunctionKeys        = {attribs.NumberOfFunctionKeys}\n" +
                $"  NumberOfIndicators          = {attribs.NumberOfIndicators}\n" +
                $"  NumberOfKeysTotal           = {attribs.NumberOfKeysTotal}\n" +
                $"  KeyRepeatMaximum-UnitId     = {attribs.KeyRepeatMaximum.UnitId}\n" +
                $"  KeyRepeatMaximum-Delay      = {attribs.KeyRepeatMaximum.Delay}\n" +
                $"  KeyRepeatMaximum-Rate       = {attribs.KeyRepeatMaximum.Rate}\n" +
                $"  KeyRepeatMinimum-UnitId     = {attribs.KeyRepeatMinimum.UnitId}\n" +
                $"  KeyRepeatMinimum-Delay      = {attribs.KeyRepeatMinimum.Delay}\n" +
                $"  KeyRepeatMinimum-Rate       = {attribs.KeyRepeatMinimum.Rate}"
                );
        }

        private static void PrintModifyUsage()
        {
            Console.WriteLine($"Usage: " +
                                            $"\n    Set-    {GetExeName()} modify <FromScanCode1>-<ToScanCode2>..." +
                                            $"\n    Add-    {GetExeName()} modify add <FromScanCode1>-<ToScanCode2>" +
                                            $"\n    Remove- {GetExeName()} modify remove <FromScanCode1>-<ToScanCode2>");
        }

        private static void PrintFilterUsage()
        {
            Console.WriteLine($"Usage: " +
                                            $"\n    Set-    {GetExeName()} filter <scanCode1> <scanCode2>..." +
                                            $"\n    Add-    {GetExeName()} filter add <scanCode>" +
                                            $"\n    Remove- {GetExeName()} filter remove <scanCode>");
        }

        private static void PrintInsertKeyUsage()
        {
            Console.WriteLine($"Usage: " +
                                            $"\n    Now-    {GetExeName()} insert <scanCode1> <scanCode2>..." +
                                            $"\n    Delay-  {GetExeName()} insert delay <milliseconds> <scanCode>");
        }

        private static void PrintUsage()
        {
            Console.WriteLine($"Usage: \n {GetExeName()} [Command] <value1> <value2>...");
            Console.WriteLine("Commands: " +
                "\n reset--------Clears Key filterings and modifications." +
                "\n activate-----Sets the active device to the input number." +
                "\n show---------Shows current input filterings and modifications for the active device." +
                "\n detect-------Detects deveice Id of the active keyboard by pressing any key." +
                "\n filter-------Sets input filtering for the active device." +
                "\n modify-------Sets input modification for the active device." +
                "\n insert-------Inserts key to the output from the context of the active device." +
                "\n attribs------Shows attributes of the active keyboard.");
        }

        private static string GetExeName()
        {
            string codeBase = Assembly.GetExecutingAssembly().CodeBase;
            return System.IO.Path.GetFileName(codeBase);
        }

        private static void PrintKeyboardDevices()
        {
            Console.WriteLine("Querying devices...");
            var devices = KeyboardEmuAPI.KeyboardEmulatorAPI.Instance.GetDevices();
            Console.WriteLine($"Devices count = {devices.NumberOfDevices}, Active device = {devices.ActiveDeviceId}");
        }

        private static void PrintKeyModifications()
        {
            Console.WriteLine("Getting device key modifications...");
            var modifying = KeyboardEmulatorAPI.Instance.KeyboardGetKeyModifying();
            Console.WriteLine($"Modify count = {modifying.ModifyCount}");
            if (modifying.ModifyData != null)
            {
                for (int i = 0; i < modifying.ModifyData.Length; i++)
                {
                    Console.WriteLine($"{i})    Modification flag = {modifying.ModifyData[i].KeyStatePredicates}, From Scan code = {modifying.ModifyData[i].FromScanCode}, To Scan code = {modifying.ModifyData[i].ToScanCode}");
                }
            }
        }

        private static void PrintKeyFilterings()
        {
            Console.WriteLine("Getting device key filters...");
            var filtering = KeyboardEmulatorAPI.Instance.KeyboardGetKeyFiltering();
            Console.WriteLine($"Filtering mode = {filtering.FilterMode}, Filtering count = {filtering.FlagOrCount}");
            if (filtering.FilterData != null)
            {
                for (int i = 0; i < filtering.FilterData.Length; i++)
                {
                    Console.WriteLine($"{i})    Filtering flag = {filtering.FilterData[i].KeyFlagPredicates}, Filtering Scan code = {filtering.FilterData[i].ScanCode}");
                }
            }
        }
    }

    enum Command
    {
        Reset,
        Detect,
        SetActive,
        SetFilter,
        SetModify,
        GetAttributes,
        InsertKeys,
        Show,
    }
}
