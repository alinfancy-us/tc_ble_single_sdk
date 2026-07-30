#!/usr/bin/env bash
#
# Flash TLSR8258 (EWN-8258FAT1BA) firmware from the macOS terminal.
#
# The TLSR8258 is programmed over Telink's single-wire "SWire" (SWS) interface.
# SEGGER J-Link does NOT speak this protocol - what actually does the job is a
# plain USB-UART adapter whose TX line emulates SWire (pvvx/TlsrComSwireWriter).
# If your J-Link exposes a VCOM port it can be used as that UART, otherwise use
# a CP210x / CH340 USB-TTL adapter (FTDI chips are NOT supported by the tool).
#
# Wiring (see https://github.com/pvvx/TlsrComSwireWriter):
#   USB-UART TX --[ 1k ]-- SWS (module SWS/SWM pin)
#   USB-UART RX ----------- SWS   (directly on the SWS node)
#   USB-UART RTS ---------- RESET  (or module +3.3V if the module has no RESET)
#   USB-UART GND ---------- GND
#   Module VCC ------------ stable 3.3V
#
# Usage:
#   ./flash.sh                          # erase+write build/825x_ble_sample/...bin at 0x0 and run
#   ./flash.sh -f path/to/fw.bin        # flash another binary
#   ./flash.sh -p /dev/cu.usbserial-10  # force the serial port
#   ./flash.sh erase                    # erase whole flash
#   ./flash.sh read dump.bin            # read back 512KB of flash
#   ./flash.sh ports                    # list candidate serial ports
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TOOL_DIR="${ROOT}/tools/flash"
VENV="${TOOL_DIR}/.venv"
FLASHER="${TOOL_DIR}/TLSR825xComFlasher.py"
FLASHER_URL="https://raw.githubusercontent.com/pvvx/TlsrComSwireWriter/master/TLSR825xComFlasher.py"

FW="${ROOT}/build/825x_ble_sample/825x_ble_sample.bin"
PORT=""
BAUD=921600
TACT=70          # activation window in ms after reset, 70ms works for most modules
ADDR=0x0
ACTION="write"
READ_FILE=""

usage() { sed -n '2,30p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0; }

while [[ $# -gt 0 ]]; do
    case "$1" in
        -f|--file)  FW="$2"; shift 2 ;;
        -p|--port)  PORT="$2"; shift 2 ;;
        -b|--baud)  BAUD="$2"; shift 2 ;;
        -t|--tact)  TACT="$2"; shift 2 ;;
        -a|--addr)  ADDR="$2"; shift 2 ;;
        erase)      ACTION="erase"; shift ;;
        read)       ACTION="read"; READ_FILE="${2:-flash_dump.bin}"; shift 2 || shift ;;
        ports)      ACTION="ports"; shift ;;
        -h|--help)  usage ;;
        *) echo "Unknown argument: $1" >&2; usage ;;
    esac
done

# ---------------------------------------------------------------- port discovery
list_ports() {
    ls /dev/cu.usbserial* /dev/cu.SLAB_USBtoUART* /dev/cu.wchusbserial* \
       /dev/cu.usbmodem* /dev/cu.UC-232* 2>/dev/null || true
}

if [[ "${ACTION}" == "ports" ]]; then
    echo "Candidate serial ports:"
    found="$(list_ports)"
    if [[ -z "${found}" ]]; then
        echo "  (none found - plug in the USB-UART adapter / J-Link VCOM)"
    else
        echo "${found}" | sed 's/^/  /'
    fi
    exit 0
fi

# ---------------------------------------------------------------- environment
if [[ ! -f "${FLASHER}" ]]; then
    echo ">>> downloading TLSR825xComFlasher.py ..."
    mkdir -p "${TOOL_DIR}"
    curl -fsSL "${FLASHER_URL}" -o "${FLASHER}"
fi

if [[ ! -x "${VENV}/bin/python" ]]; then
    echo ">>> creating python venv with pyserial ..."
    python3 -m venv "${VENV}"
    "${VENV}/bin/pip" -q install --upgrade pip
    "${VENV}/bin/pip" -q install pyserial
fi

# ---------------------------------------------------------------- port select
if [[ -z "${PORT}" ]]; then
    PORT="$(list_ports | head -1)"
    if [[ -z "${PORT}" ]]; then
        echo "ERROR: no USB serial port found." >&2
        echo "       Plug in the USB-UART adapter, then run: ./flash.sh ports" >&2
        exit 1
    fi
    echo ">>> auto-detected port: ${PORT}"
fi

PY=("${VENV}/bin/python" "${FLASHER}" -p "${PORT}" -b "${BAUD}" -t "${TACT}")

case "${ACTION}" in
    erase)
        echo ">>> erasing whole flash on ${PORT}"
        "${PY[@]}" ea
        ;;
    read)
        echo ">>> reading 512KB flash to ${READ_FILE}"
        "${PY[@]}" rf 0x0 0x80000 "${READ_FILE}"
        ;;
    write)
        if [[ ! -f "${FW}" ]]; then
            echo "ERROR: firmware not found: ${FW}" >&2
            echo "       build it first with: ./docker.sh" >&2
            exit 1
        fi
        echo ">>> flashing $(basename "${FW}") ($(wc -c < "${FW}" | tr -d ' ') bytes) to ${ADDR} on ${PORT}"
        "${PY[@]}" -r wf "${ADDR}" "${FW}"
        echo ">>> done - module should now advertise as 'VHID'"
        ;;
esac
