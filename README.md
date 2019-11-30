InputEmulator
============
InputEmulator consists of keyboard and mouse filter drivers which can be used to 
inject, modify and filter raw inputs from keyboard and mouse devices connected 
to the PC.
[Download the latest release][latest-release]

Building
--------

Driver source code can be built with [Windows Driver Kit][wdk].

API source code is available for .Net framework with c# and native with C.

- Tested on Windows 10.

Driver installation
-------------------

Latest vesions of windows(64bit) does not allow installation of unsigned drivers.
To install an unsigend driver, you need to run this command with administrative rights:

bcdedit /set testsigning on

Above command requires a reboot to take effect. Also, if you are running it a on Windows 10 
with Secure Boot enabled, changing the test signing mode as above would fail.

Drivers can be installed through the command line installer. You need to run the installer 
with administrative rights and restart your PC afterwards for installation to take effect.

Run `installer.exe` without any arguments inside a command prompt 
to see the instructions for installation.

License
-------
InputEmulator is licensed under [GNU Lesser General Public License v3 (LGPL-3.0)][licenses].


© 2019 Behzad A. Shams

[latest-release]: https://github.com/behzad62/InputEmulator/releases/download/1.0/InputEmulator.zip
[wdk]: https://docs.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk
[licenses]: https://github.com/behzad62/InputEmulator/blob/master/LICENSE
