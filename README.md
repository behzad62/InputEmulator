InputEmulator
============

[Download the latest release][latest-release]

Building
--------

Driver source code can be built with [Windows Driver Kit][wdk].

API source code is provided for .Net framework with c# and native with C.

- Tested on Windows 10.

Driver installation
-------------------

Latest vesions of windows(64bit) does not allow installation of unsigned drivers.
To install a unsigend driver, you need to run this command with administrative rights:

bcdedit /set testsigning on

This command requires a reboot to take effect. Also, if you are running it a on Windows 10 with Secure Boot enabled, changing the test signing mode
would fail.

Drivers can be installed through the command line installer, but driver
installation requires execution inside a prompt with administrative rights
and rebooting afterwards for installation to take effect.

Run `installer` without any arguments inside an console executed as
administrator and it will give instructions for installation.

License
-------
InputEmulator is licensed under [GNU Lesser General Public License v3 (LGPL-3.0)][licenses].


© 2019 Behzad A. Shams

[latest-release]: https://github.com/behzad62/InputEmulator/releases/latest
[wdk]: https://docs.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk
[licenses]: https://github.com/behzad62/InputEmulator/tree/master/licenses
