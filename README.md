# CDA3101 Computer Organization

- **Author:** Daniel Li
- **Semester:** Spring 2025, University of Florida

Directories named `pa#` are programming assignments.

This course uses the ARMv8/AArch64 architecture. To test on an x86-64 or other
machine, an emulator is required.

## Documentation

- [ARM documentation](https://developer.arm.com/documentation)
- [General instructions](https://developer.arm.com/documentation/dui0801/l/A64-General-Instructions/A64-instructions-in-alphabetical-order)
- [Data transfer instructions](https://developer.arm.com/documentation/dui0801/l/A64-Data-Transfer-Instructions/A64-data-transfer-instructions-in-alphabetical-order)
- [Floating-point instructions](https://developer.arm.com/documentation/dui0801/l/A64-Floating-point-Instructions/A64-floating-point-instructions-in-alphabetical-order)

## Emulator Setup

 1. Install [QEMU](https://www.qemu.org):

    - Windows: `scoop install qemu`
    - Arch Linux: `pacman -S qemu-system-aarch64`
    - ...or whatever you use for your system.

 2. Download and extract
    <https://drive.google.com/file/d/1KM871i5SfDimf5hDHwY_fE_Wa2h12IOg/view?usp=sharing>
    to the root of the repository. Inside should be a directory named
    `qemu-arm-img`.

 3. Start the emulator (use Git Bash for Windows):

    ```
    ./emulator.sh
    ```

 4. Login with user `root` and password `root`, or connect via SSH on port 3101:

    ```
    ssh -p 3101 root@localhost
    ```

 5. Transfer files using `scp` or `sftp`:

    ```
    scp -P 3101 <hostpath> root@localhost:<vmpath>
    scp -P 3101 root@localhost:<vmpath> <hostpath>
    sftp -P 3101 root@localhost
    ```

 6. Stop the emulator using the `poweroff` command:

    ```
    poweroff
    ```
