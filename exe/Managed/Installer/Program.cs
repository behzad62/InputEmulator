using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Win32;
using System.IO;
using System.Diagnostics;

namespace Installer
{

    class Program
    {
        const string KEYBOARD_FILTER_KEY = @"System\CurrentControlSet\Control\Class\{4D36E96B-E325-11CE-BFC1-08002BE10318}";
        const string MOUSE_FILTER_KEY = @"System\CurrentControlSet\Control\Class\{4D36E96F-E325-11CE-BFC1-08002BE10318}";
        const string UpperFilters = "UpperFilters";
        const string KeyboardDriverFileName = "KeyboardEmulator.sys";
        const string MouseDriverFileName = "MouseEmulator.sys";
        const string KeyboardServiceName = "KeyboardEmulator";
        const string MouseServiceName = "MouseEmulator";
        const string kbdclass = "kbdclass";
        const string mouclass = "mouclass";
        static void Main(string[] args)
        {
            if (args.Length != 2)
            {
                PrintUsage();
                return;
            }
            switch (args[0].ToLower())
            {
                case "install":
                    if (args[1].ToLower() == "keyboard")
                    {
                        InstallKeyboardDriver();
                    }
                    else if (args[1].ToLower() == "mouse")
                    {
                        InstallMouseDriver();
                    }
                    else if (args[1].ToLower() == "all")
                    {
                        InstallKeyboardDriver();
                        InstallMouseDriver();
                    }
                    else
                    {
                        PrintUsage();
                        return;
                    }
                    break;
                case "uninstall":
                    if (args[1].ToLower() == "keyboard")
                    {
                        UninstallKeyboardDriver();
                    }
                    else if (args[1].ToLower() == "mouse")
                    {
                        UninstallMouseDriver();
                    }
                    else if (args[1].ToLower() == "all")
                    {
                        UninstallKeyboardDriver();
                        UninstallMouseDriver();
                    }
                    else
                    {
                        PrintUsage();
                        return;
                    }
                    break;
                default:
                    PrintUsage();
                    return;
            }
        }

        static bool InstallKeyboardDriver()
        {
            try
            {
                string filePath = Environment.Is64BitOperatingSystem ? Path.Combine(GetInstallerDirectory(), "Sys64", KeyboardDriverFileName) 
                    : Path.Combine(GetInstallerDirectory(), "Sys86", KeyboardDriverFileName);
                FileInfo file = new FileInfo(filePath);
                if (!file.Exists)
                {
                    Console.WriteLine($"'{filePath}' not found. Installing keyboard driver failed.");
                    return false;
                }
                string destPath = GetKeyboardDriverInstallPath();

                file.CopyTo(destPath, true);

                if (!File.Exists(destPath))
                {
                    Console.WriteLine($"Could not copy driver to system directory. Installing keyboard driver failed.");
                    return false;
                }
                if (StartService(KeyboardServiceName, destPath))
                {
                    SetRegistryKeyValue(KEYBOARD_FILTER_KEY, kbdclass, KeyboardServiceName);
                    Console.WriteLine($"Successfully installed  keyboard driver.");
                    return true;
                }
                Console.WriteLine($"Could not create service. Installing keyboard driver failed.");
                return false;
            }
            catch(Exception ex)
            {
                Console.WriteLine($"Installing keyboard driver failed. Error message: {ex.Message}");
                return false;
            }
        }

        static bool InstallMouseDriver()
        {
            try
            {
                string filePath = Environment.Is64BitOperatingSystem ? Path.Combine(GetInstallerDirectory(), "Sys64", MouseDriverFileName)
                    : Path.Combine(GetInstallerDirectory(), "Sys86", MouseDriverFileName);
                FileInfo file = new FileInfo(filePath);
                if (!file.Exists)
                {
                    Console.WriteLine($"'{filePath}' not found. Installing mouse driver failed.");
                    return false;
                }
                string destPath = GetMouseDriverInstallPath();

                file.CopyTo(destPath, true);
                if (!File.Exists(destPath))
                {
                    Console.WriteLine($"Could not copy driver to system directory. Installing mouse driver failed.");
                    return false;
                }
                if (StartService(MouseServiceName, destPath))
                {
                    SetRegistryKeyValue(MOUSE_FILTER_KEY, mouclass, MouseServiceName);
                    Console.WriteLine($"Successfully installed  mouse driver.");
                    return true;
                }
                Console.WriteLine($"Could not create service. Installing mouse driver failed.");
                return false;
            }
            catch(Exception ex)
            {
                Console.WriteLine($"Installing mouse driver failed. Error message: {ex.Message}");
                return false;
            }
        }

        static bool UninstallKeyboardDriver()
        {
            string destPath = GetKeyboardDriverInstallPath();
            try
            {
                if (File.Exists(destPath))
                    File.Delete(destPath);
            }
            catch (Exception ex)
            {
                //TODO: we need to delete this file at next windows startup after we deleted the service
            }
            DeleteService(KeyboardServiceName);

            RemoveRegistryKeyValue(KEYBOARD_FILTER_KEY, kbdclass, KeyboardServiceName);

            return true;
        }

        static bool UninstallMouseDriver()
        {
            string destPath = GetMouseDriverInstallPath();
            try
            {
                if (File.Exists(destPath))
                    File.Delete(destPath);
            }
            catch (Exception ex)
            {
                //TODO: we need to delete this file at next windows startup after we deleted the service
            }
            DeleteService(MouseServiceName);

            RemoveRegistryKeyValue(MOUSE_FILTER_KEY, mouclass, MouseServiceName);

            return true;
        }

        static bool StartService(string serviceName, string servicePath)
        {
            using (Process p = new Process())
            {
                p.StartInfo.WindowStyle = ProcessWindowStyle.Hidden;
                p.StartInfo.CreateNoWindow = true;
                p.StartInfo.FileName = "sc.exe";
                p.StartInfo.UseShellExecute = false;
                p.StartInfo.RedirectStandardOutput = true;
                p.StartInfo.RedirectStandardError = true;
                p.StartInfo.Verb = "runas";
                p.StartInfo.Arguments = $"create {serviceName} type= kernel binPath= {servicePath} DisplayName= {serviceName}";
                try
                {
                    if (p.Start())
                    {
                        while (!p.StandardOutput.EndOfStream)
                        {
                            string line = p.StandardOutput.ReadLine();
                            Console.WriteLine(line);
                        }
                        while (!p.StandardError.EndOfStream)
                        {
                            string line = p.StandardOutput.ReadLine();
                            Console.WriteLine(line);
                        }
                        if (p.ExitCode == 0 || p.ExitCode == 1073)
                            return true;//created or already exists
                        Console.WriteLine($"sc.exe exited with code {p.ExitCode}");
                        return false;

                    }
                    return false;
                }
                catch (Exception ex)
                {
                    Console.WriteLine(ex);
                    return false;
                }
            }
        }

        static bool DeleteService(string serviceName)
        {
            using (Process p = new Process())
            {
                p.StartInfo.WindowStyle = ProcessWindowStyle.Hidden;
                p.StartInfo.CreateNoWindow = true;
                p.StartInfo.FileName = "sc.exe";
                p.StartInfo.UseShellExecute = false;
                p.StartInfo.RedirectStandardOutput = true;
                p.StartInfo.RedirectStandardError = true;
                p.StartInfo.Verb = "runas";
                p.StartInfo.Arguments = $"delete {serviceName}";
                try
                {
                    if (p.Start())
                    {
                        while (!p.StandardOutput.EndOfStream)
                        {
                            string line = p.StandardOutput.ReadLine();
                            Console.WriteLine(line);
                        }
                        while (!p.StandardError.EndOfStream)
                        {
                            string line = p.StandardOutput.ReadLine();
                            Console.WriteLine(line);
                        }
                        if (p.ExitCode == 0 || p.ExitCode == 1060)
                            return true;//deleted or not found
                        Console.WriteLine($"sc.exe ended with code {p.ExitCode}");
                        return false;
                    }
                    return false;
                }
                catch (Exception ex)
                {
                    Console.WriteLine(ex);
                    return false;
                }
            }
        }

        private static void SetRegistryKeyValue(string regKey, string classService, string ourServiceName)
        {
            var value = Registry.LocalMachine.OpenSubKey(regKey, true);
            var upperFiltersCurrent = (string[])value.GetValue(UpperFilters);
            upperFiltersCurrent = upperFiltersCurrent.Where(v => v != ourServiceName).ToArray();//make sure our service name does not exists
            string[] upperFiltersFinal = new string[upperFiltersCurrent.Length + 1];
            int currentIdnex = 0;
            for (int i = 0; i < upperFiltersCurrent.Length; i++)
            {
                if (upperFiltersCurrent[i] == classService)
                {
                    upperFiltersFinal[currentIdnex] = ourServiceName;
                    upperFiltersFinal[++currentIdnex] = classService;
                }
                else
                    upperFiltersFinal[currentIdnex] = upperFiltersCurrent[i];
                currentIdnex++;
            }
            value.SetValue(UpperFilters, upperFiltersFinal, RegistryValueKind.MultiString);
            value.Close();
        }

        private static void RemoveRegistryKeyValue(string regKey, string classService, string ourServiceName)
        {
            var value = Registry.LocalMachine.OpenSubKey(regKey, true);
            var upperFiltersCurrent = (string[])value.GetValue(UpperFilters);
            string[] upperFiltersFinal = upperFiltersCurrent.Where(v => v != ourServiceName).ToArray();

            value.SetValue(UpperFilters, upperFiltersFinal, RegistryValueKind.MultiString);
            value.Close();
        }

        static string GetKeyboardDriverInstallPath()
        {
            return Path.Combine(Environment.SystemDirectory, "drivers", KeyboardDriverFileName);
        }

        static string GetMouseDriverInstallPath()
        {
            return Path.Combine(Environment.SystemDirectory, "drivers", MouseDriverFileName);
        }




        static void PrintUsage()
        {
            Console.WriteLine("Installer must be run with administrative rights.");
            Console.WriteLine($"Usage:" +
                $"\n Install mouse driver-------{GetExeName()} install mouse" +
                $"\n Uninstall mouse driver-----{GetExeName()} uninstall mouse" +
                $"\n Install keyboard driver----{GetExeName()} install keyboard" +
                $"\n Uninstall mouse driver-----{GetExeName()} uninstall keyboard" +
                $"\n Install all----------------{GetExeName()} install all" +
                $"\n Uninstall all--------------{GetExeName()} uninstall all");
        }


        private static string GetExeName()
        {
            string location = Assembly.GetExecutingAssembly().Location;
            return System.IO.Path.GetFileName(location);
        }

        private static string GetInstallerDirectory()
        {
            return AppDomain.CurrentDomain.BaseDirectory;
        }
    }
}
