# CDA3101 Computer Organization

- **Author:** Daniel Li
- **Semester:** Spring 2025, University of Florida

## Emulator Setup

 1. Install QEMU:

    ```
    scoop install qemu
    ```

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

 5. Stop the emulator using the `poweroff` command:

    ```
    poweroff
    ```
