#!/bin/bash
# detect_environment.sh - auto-detect MIDASSYS and write .env file

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/.helpers.sh"

DEBUG=false

show_help() {
  cat << EOF
Usage: $0 [OPTIONS] [SEARCH_ROOTS...]

Options:
  -d, --debug       Enable debug output
  -h, --help        Show this help message and exit

Positional arguments:
  SEARCH_ROOTS      One or more directories to search for MIDAS installation.
                    Defaults to: \$HOME /opt /usr/local /usr
EOF
}

# Default search roots
SEARCH_ROOTS=("$HOME" "/opt" "/usr/local" "/usr")

# Parse CLI args
while [[ $# -gt 0 ]]; do
  case $1 in
    -d|--debug) DEBUG=true; shift ;;
    -h|--help) show_help; exit 0 ;;
    --) shift; break ;;
    -*) echo "Unknown option: $1" >&2; show_help; exit 1 ;;
    *) 
      # Assume positional args (search roots) start here
      break
      ;;
  esac
done

# If there are positional args left, override SEARCH_ROOTS
if [[ $# -gt 0 ]]; then
  SEARCH_ROOTS=()
  while [[ $# -gt 0 ]]; do
    SEARCH_ROOTS+=("$1")
    shift
  done
fi

REQUIRED_FILES=("MidasConfig.cmake" "include/midas.h")

ENV_FILE="$SCRIPT_DIR/.env"

echo "[INFO] Searching for MIDAS installation, this may take some time ..."

$DEBUG && echo "[DEBUG] Searching for MIDAS installation in: ${SEARCH_ROOTS[*]}" >&2

if found=$(find_root_with_files "${SEARCH_ROOTS[@]}" -- "${REQUIRED_FILES[@]}" "$DEBUG"); then
  echo "[INFO] Found MIDASSYS: $found"
else
  echo "[ERROR]: MIDAS installation not found." >&2
  echo
  echo "You can manually create the .env file with your MIDAS installation path, for example:"
  echo "  echo 'export MIDASSYS=/path/to/midas' > $ENV_FILE"
  echo "  source $ENV_FILE"
  exit 1
fi

cat > "$ENV_FILE" <<EOF
export MIDASSYS=$found
EOF

echo "[INFO] .env file written at: $ENV_FILE"
echo "[INFO] Run 'source $ENV_FILE' before building."
