using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using MouseEmuAPI;

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
                    Console.WriteLine("Getting devices Id of your Mouse. Please press any button or move your mouse.");
                    deviceId = MouseEmulatorAPI.Instance.MouseDetectDeviceId();
                    Console.WriteLine($"Device Id is {deviceId}");
                    PrintMouseDevices();
                    break;
                case Command.Reset:
                    MouseEmulatorAPI.Instance.MouseSetFilterMode(FilterMode.MOUSE_NONE);
                    MouseEmulatorAPI.Instance.MouseSetModification(new MouseModification { ModifyCount = 0 });
                    Console.WriteLine("Filterings and modifications cleared for the active device.");
                    break;
                case Command.SetActive:
                    if (!ushort.TryParse(args[1], out deviceId))
                    {
                        Console.WriteLine($"Usage: \n {GetExeName()} active <deviceId>");
                        return;
                    }
                    Console.WriteLine($"Setting device Id to {deviceId}");
                    MouseEmulatorAPI.Instance.MouseSetActiveDevice(deviceId);
                    PrintMouseDevices();
                    break;
                case Command.SetFilter:
                    if (args.Length < 2)
                    {
                        PrintFilterUsage();
                        return;
                    }
                    FilterMode filterMode = FilterMode.MOUSE_NONE;
                    for (int i = 1; i < args.Length; i++)
                    {
                        if (args[i].ToLower() == "scroll")
                            filterMode |= FilterMode.MOUSE_WHEEL;
                        else if (args[i].ToLower() == "move")
                            filterMode |= FilterMode.MOUSE_MOVE;
                        else
                        {
                            if (!ushort.TryParse(args[i], out ushort buttonCode))
                            {
                                PrintFilterUsage();
                                return;
                            }
                            switch (buttonCode)
                            {
                                case 1:
                                    filterMode |= FilterMode.BUTTON_1_PRESS;
                                    break;
                                case 2:
                                    filterMode |= FilterMode.BUTTON_2_PRESS;
                                    break;
                                case 3:
                                    filterMode |= FilterMode.BUTTON_3_PRESS;
                                    break;
                                case 4:
                                    filterMode |= FilterMode.BUTTON_4_PRESS;
                                    break;
                                case 5:
                                    filterMode |= FilterMode.BUTTON_5_PRESS;
                                    break;
                                default:
                                    filterMode |= (FilterMode)buttonCode;
                                    break;
                            }
                        }

                    }

                    MouseEmulatorAPI.Instance.MouseSetFilterMode(filterMode);
                    Console.WriteLine("Filters set.");
                    PrintFilterMode();
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
                        if (parts.Length != 2 || !ushort.TryParse(parts[0], out ushort fromState) || !ushort.TryParse(parts[1], out ushort toState))
                        {
                            PrintModifyUsage();
                            return;
                        }
                        if (args[1].ToLower() == "add")
                        {
                            MouseEmulatorAPI.Instance.MouseAddButtonModification(new ButtonModifyData() { FromState = (MouseButtonState)fromState, ToState = (MouseButtonState)toState });
                            Console.WriteLine("Key modification added.");
                            PrintKeyModifications();
                        }
                        else
                        {
                            MouseEmulatorAPI.Instance.MouseRemoveButtonModification(new ButtonModifyData() { FromState = (MouseButtonState)fromState, ToState = (MouseButtonState)toState });
                            Console.WriteLine("Key modification removed.");
                            PrintKeyModifications();
                        }
                    }
                    else
                    {
                        var buttonPairs = new List<Tuple<ushort, ushort>>();
                        for (int i = 1; i < args.Length; i++)
                        {
                            string[] parts = args[i].Split(new[] { '-' }, StringSplitOptions.RemoveEmptyEntries);
                            if (parts.Length != 2 || !ushort.TryParse(parts[0], out ushort fromButton) || !ushort.TryParse(parts[1], out ushort toButton))
                            {
                                PrintModifyUsage();
                                return;
                            }
                            buttonPairs.Add(new Tuple<ushort, ushort>(fromButton, toButton));
                        }
                        MouseModification modifyRequest = new MouseModification();
                        List<ButtonModifyData> modifyDatas = new List<ButtonModifyData>();
                        for (int i = 0; i < buttonPairs.Count; i++)
                        {
                            ButtonModifyData modifyData1 = new ButtonModifyData();
                            ButtonModifyData modifyData2 = new ButtonModifyData();
                            switch (buttonPairs[i].Item1)
                            {
                                case 1:
                                    modifyData1.FromState = MouseButtonState.BUTTON_1_DOWN;
                                    modifyData2.FromState = MouseButtonState.BUTTON_1_UP;
                                    break;
                                case 2:
                                    modifyData1.FromState = MouseButtonState.BUTTON_2_DOWN;
                                    modifyData2.FromState = MouseButtonState.BUTTON_2_UP;
                                    break;
                                case 3:
                                    modifyData1.FromState = MouseButtonState.BUTTON_3_DOWN;
                                    modifyData2.FromState = MouseButtonState.BUTTON_3_UP;
                                    break;
                                case 4:
                                    modifyData1.FromState = MouseButtonState.BUTTON_4_DOWN;
                                    modifyData2.FromState = MouseButtonState.BUTTON_4_UP;
                                    break;
                                case 5:
                                    modifyData1.FromState = MouseButtonState.BUTTON_5_DOWN;
                                    modifyData2.FromState = MouseButtonState.BUTTON_5_UP;
                                    break;
                            }
                            switch (buttonPairs[i].Item2)
                            {
                                case 1:
                                    modifyData1.ToState = MouseButtonState.BUTTON_1_DOWN;
                                    modifyData2.ToState = MouseButtonState.BUTTON_1_UP;
                                    break;
                                case 2:
                                    modifyData1.ToState = MouseButtonState.BUTTON_2_DOWN;
                                    modifyData2.ToState = MouseButtonState.BUTTON_2_UP;
                                    break;
                                case 3:
                                    modifyData1.ToState = MouseButtonState.BUTTON_3_DOWN;
                                    modifyData2.ToState = MouseButtonState.BUTTON_3_UP;
                                    break;
                                case 4:
                                    modifyData1.ToState = MouseButtonState.BUTTON_4_DOWN;
                                    modifyData2.ToState = MouseButtonState.BUTTON_4_UP;
                                    break;
                                case 5:
                                    modifyData1.ToState = MouseButtonState.BUTTON_5_DOWN;
                                    modifyData2.ToState = MouseButtonState.BUTTON_5_UP;
                                    break;
                            }
                            modifyDatas.Add(modifyData1);
                            modifyDatas.Add(modifyData2);
                        }
                        modifyRequest.ModifyData = modifyDatas.ToArray();
                        modifyRequest.ModifyCount = (ushort)modifyDatas.Count;
                        MouseEmulatorAPI.Instance.MouseSetModification(modifyRequest);
                        Console.WriteLine($"{modifyRequest.ModifyCount} Button modifications set.");
                        PrintKeyModifications();
                    }
                    break;
                case Command.InsertKeys:
                    if (args.Length < 2)
                    {
                        PrintInsertKeyUsage();
                        return;
                    }
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
                    if (args[startIndex].Contains('-'))
                    {
                        string[] parts = args[startIndex].Split(new[] { '-' }, StringSplitOptions.RemoveEmptyEntries);
                        if (parts.Length != 2 || !ushort.TryParse(parts[0], out ushort x) || !ushort.TryParse(parts[1], out ushort y))
                        {
                            PrintModifyUsage();
                            return;
                        }
                        var inputData = new Mouse_Input_Data();
                        inputData.Flags = MouseFlag.MOUSE_MOVE_ABSOLUTE | MouseFlag.MOUSE_VIRTUAL_DESKTOP;
                        inputData.LastX = x;
                        inputData.LastY = y;
                        Mouse_Input_Data[] inputs = new Mouse_Input_Data[1] { inputData };
                        if (delay > 0)
                            Task.Delay(delay).Wait();
                        MouseEmulatorAPI.Instance.MouseInsertInputs(inputs);
                    }
                    else
                    {
                        if (!ushort.TryParse(args[startIndex], out ushort buttonNo) || buttonNo > 5)
                        {
                            PrintInsertKeyUsage();
                            return;
                        }
                        Mouse_Input_Data[] inputs = new Mouse_Input_Data[2];
                        switch (buttonNo)
                        {
                            case 1:
                                inputs[0].Buttons.ButtonFlags = MouseButtonState.BUTTON_1_DOWN;
                                inputs[1].Buttons.ButtonFlags = MouseButtonState.BUTTON_1_UP;
                                break;
                            case 2:
                                inputs[0].Buttons.ButtonFlags = MouseButtonState.BUTTON_2_DOWN;
                                inputs[1].Buttons.ButtonFlags = MouseButtonState.BUTTON_2_UP;
                                break;
                            case 3:
                                inputs[0].Buttons.ButtonFlags = MouseButtonState.BUTTON_3_DOWN;
                                inputs[1].Buttons.ButtonFlags = MouseButtonState.BUTTON_3_UP;
                                break;
                            case 4:
                                inputs[0].Buttons.ButtonFlags = MouseButtonState.BUTTON_4_DOWN;
                                inputs[1].Buttons.ButtonFlags = MouseButtonState.BUTTON_4_UP;
                                break;
                            case 5:
                                inputs[0].Buttons.ButtonFlags = MouseButtonState.BUTTON_5_DOWN;
                                inputs[1].Buttons.ButtonFlags = MouseButtonState.BUTTON_5_UP;
                                break;
                        }
                        if (delay > 0)
                            Task.Delay(delay).Wait();
                        MouseEmulatorAPI.Instance.MouseInsertInputs(inputs);
                    }
                    break;
                case Command.Show:
                    PrintMouseDevices();
                    PrintFilterMode();
                    PrintKeyModifications();
                    break;
                case Command.GetAttributes:
                    PrintMouseAttribs();
                    break;
                default:
                    PrintUsage();
                    break;
            }

        }

        private static void PrintMouseAttribs()
        {
            Console.WriteLine("Getting Mouse attributes...");
            var attribs = MouseEmulatorAPI.Instance.MouseGetAttributes();

            Console.WriteLine($"Attributes:\n" +
                $"  MouseIdentifier-Type    = {attribs.MouseIdentifier}\n" +
                $"  NumberOfButtons         = {attribs.NumberOfButtons}\n" +
                $"  InputDataQueueLength    = {attribs.InputDataQueueLength}\n" +
                $"  SampleRate              = {attribs.SampleRate}\n"
                );
        }

        private static void PrintModifyUsage()
        {
            Console.WriteLine($"Usage: " +
                                            $"\n    Set-    {GetExeName()} modify <From Button no.(1-5)>-<To Button no.(1-5)>..." +
                                            $"\n    Add-    {GetExeName()} modify add <From Button no.(1-5)>-<To Button no.(1-5)>" +
                                            $"\n    Remove- {GetExeName()} modify remove <From Button no.(1-5)>-<To Button no.(1-5)>");
        }

        private static void PrintFilterUsage()
        {
            Console.WriteLine($"Usage: " +
                                            $"\n    Buttons-  {GetExeName()} filter <Button no.(1-5)>" +
                                            $"\n    Movement- {GetExeName()} filter <move>" +
                                            $"\n    Scroll-   {GetExeName()} filter <scroll>");
        }

        private static void PrintInsertKeyUsage()
        {
            Console.WriteLine($"Usage: " +
                                            $"\n    Movement         - {GetExeName()} insert <x-y>" +
                                            $"\n    Buttons          - {GetExeName()} <Button no.(1-5)>" +
                                            $"\n    Delayed Movement - {GetExeName()} insert delay <milliseconds> <x-y>" +
                                            $"\n    Delayed Buttons  - {GetExeName()} insert delay <milliseconds> <Button no.(1-5)>");
        }

        private static void PrintUsage()
        {
            Console.WriteLine($"Usage: \n {GetExeName()} [Command] <value1> <value2>...");
            Console.WriteLine("Commands: " +
                "\n reset--------Clears mouse filterings and modifications." +
                "\n activate-----Sets the active device to the input number." +
                "\n show---------Shows current input filterings and modifications for the active device." +
                "\n detect-------Detects deveice Id of the active Mouse by pressing any key." +
                "\n filter-------Sets input filtering for the active device." +
                "\n modify-------Sets input modification for the active device." +
                "\n insert-------Inserts input to the output from the context of the active device." +
                "\n attribs------Shows attributes of the active Mouse.");
        }

        private static string GetExeName()
        {
            string codeBase = Assembly.GetExecutingAssembly().CodeBase;
            return System.IO.Path.GetFileName(codeBase);
        }

        private static void PrintMouseDevices()
        {
            Console.WriteLine("Querying devices...");
            var devices = MouseEmuAPI.MouseEmulatorAPI.Instance.GetDevices();
            Console.WriteLine($"Devices count = {devices.NumberOfDevices}, Active device = {devices.ActiveDeviceId}");
        }

        private static void PrintKeyModifications()
        {
            Console.WriteLine("Getting device button modifications...");
            var modifying = MouseEmulatorAPI.Instance.MouseGetModifications();
            Console.WriteLine($"Modify count = {modifying.ModifyCount}");
            if (modifying.ModifyData != null)
            {
                for (int i = 0; i < modifying.ModifyData.Length; i++)
                {
                    Console.WriteLine($"{i})    Modification:  From State = {modifying.ModifyData[i].FromState}, To State = {modifying.ModifyData[i].ToState}");
                }
            }
        }

        private static void PrintFilterMode()
        {
            Console.WriteLine("Getting device filter mode...");
            var filtering = MouseEmulatorAPI.Instance.MouseGetFilterMode();
            Console.WriteLine($"Filtering mode = {filtering}");
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
