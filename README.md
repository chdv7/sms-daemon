# sms-daemon
A small SMS and USSD sender/receiver for 2G/3G/4G modem.

Tested with Huawei e1550

For Raspberry Pi mobile internet setup through Huawei E1550, see `docs/raspberry-pi-e1550-internet.md`. A typical PPP setup script is available at `scripts/setup-e1550-ppp.sh`.

Configuration is read from `/etc/sms-daemon/config.cfg` by default. The config file must exist; if a key is missing, the compiled default is used. Use another file with `-c`:

    sms-daemon -c /path/to/config.cfg
    sms-send -c /path/to/config.cfg '*100#'

Supported config keys:

    device=/dev/ttyUSB0
    job_dir=./outsms
    sms_dir=/tmp/sms-daemon/ReceivedSMS
    ussd_dir=/tmp/sms-daemon/ReceivedSMS
    log_file=/tmp/sms-daemon/Log
    sms_hook=/usr/local/bin/sms-hook
    debug=false

`sms_hook` can be repeated. Each configured program is run for every received SMS.
Arguments are passed without shell expansion:

    argv[1] SMS text
    argv[2] sender number
    argv[3] sent timestamp from SMS/PDU
    argv[4] daemon receive timestamp
    argv[5] SMS center number
    argv[6] IMSI
    argv[7] IMEI


Build with CMake, step by step:

1. Install the basic build tools. On Debian, Ubuntu, or Raspberry Pi OS:

       sudo apt update
       sudo apt install build-essential cmake

   GoogleTest is optional. Install it only if you want to build and run unit tests:

       sudo apt install libgtest-dev

2. Configure the project. Run this command from the project root directory:

       cmake -S . -B build -DBUILD_TESTING=OFF

   `-S .` means: source files are in the current directory.
   `-B build` means: put all temporary CMake files and object files into `build/`.
   `-DBUILD_TESTING=OFF` means: do not build unit tests, so GoogleTest is not needed.

3. Build the programs:

       cmake --build build

   After this, the binaries are here:

       build/sms-daemon
       build/sms-send

4. To build with tests, configure with testing enabled:

       cmake -S . -B build -DBUILD_TESTING=ON
       cmake --build build

   If GoogleTest is installed, CMake also builds `build/sms-tests`.
   If GoogleTest is not installed, CMake prints a message and still builds `sms-daemon` and `sms-send`.

5. Run tests when they are built:

       ctest --test-dir build -V

   Or run the test binary directly:

       ./build/sms-tests

6. Install the project system-wide:

       sudo cmake --install build

   This installs `sms-daemon`, `sms-send`, the default config under `/etc/sms-daemon/`, and the systemd unit.

7. Enable and start the Linux daemon through systemd:

       sudo systemctl daemon-reload
       sudo systemctl enable --now sms-daemon.service

   The service runs `sms-daemon` in foreground mode and reads `/etc/sms-daemon/config.cfg`.
   Check status and logs with:

       systemctl status sms-daemon.service
       journalctl -u sms-daemon.service -f

8. Clean CMake temporary files when you want a fresh build:

       rm -rf build
       cmake -S . -B build -DBUILD_TESTING=OFF
       cmake --build build

   If only CMake configuration is stale, removing `build/CMakeCache.txt` is often enough, but deleting the whole `build/` directory is simpler and safer for beginners.

Send an SMS body from stdin:

    sms-send +70123456789 [./outsms] < message.txt

Send a USSD request:

    sms-send '*100#'

Send MMI/supplementary-service call-forwarding codes. Supported service codes are `21`, `67`, `61`, `62`, `002`, and `004`:

    sms-send '*#21#'              # query unconditional forwarding
    sms-send '**21*<number>#'     # register unconditional forwarding
    sms-send '##21#'              # erase unconditional forwarding
    sms-send '*#67#'              # query forwarding on busy
    sms-send '**61*<number>**20#' # register no-reply forwarding with timeout

If `sms-send` is called without an output directory, it uses `job_dir` from the config. USSD jobs are stored as `U"<encoded-payload>",<dcs>`. MMI/call-forwarding jobs are stored as `M<code>`. The daemon writes received SMS and USSD/MMI responses as XML files to `sms_dir` and `ussd_dir`.
