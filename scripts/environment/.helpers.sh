# .helpers.sh - reusable helper functions for environment detection

# find_root_with_files ROOT1 [ROOT2 ...] -- FILE1 [FILE2 ...] DEBUG
# Finds the first directory under any ROOT that contains all specified FILES (relative paths)
find_root_with_files() {
  local debug="${@: -1}"        # Last arg is debug flag
  set -- "${@:1:$(($#-1))}"     # All but last are usable args

  # Split args at '--' to separate roots and required files
  local roots=()
  local files=()
  local in_roots=true

  for arg in "$@"; do
    if [[ "$arg" == "--" ]]; then
      in_roots=false
      continue
    fi
    if $in_roots; then
      roots+=("$arg")
    else
      files+=("$arg")
    fi
  done

  if [[ ${#roots[@]} -eq 0 || ${#files[@]} -eq 0 ]]; then
    echo "ERROR: Usage: find_root_with_files ROOT1 [ROOT2 ...] -- FILE1 [FILE2 ...] DEBUG" >&2
    return 2
  fi

  local total_searched=0
  local candidates=()

  for root in "${roots[@]}"; do
    $debug && echo "[DEBUG] Searching under: $root for files: ${files[*]}" >&2
    local searched=0
    while IFS= read -r dir; do
      ((searched++))
      local all_found=true
      for relpath in "${files[@]}"; do
        if [[ ! -e "$dir/$relpath" ]]; then
          all_found=false
          break
        fi
      done
      if $all_found; then
        candidates+=("$dir")
      fi
    done < <(find "$root" -type d 2>/dev/null)
    ((total_searched+=searched))
  done

  $debug && echo "[DEBUG] Total directories searched: $total_searched" >&2
  $debug && echo "[DEBUG] Candidates found: ${#candidates[@]}" >&2

  if [[ ${#candidates[@]} -eq 0 ]]; then
    echo "ERROR: No directory found containing: ${files[*]}" >&2
    return 1
  fi

  if [[ ${#candidates[@]} -gt 1 ]]; then
    echo "WARNING: Multiple directories found containing all required files:" >&2
    for c in "${candidates[@]}"; do
      echo "  $c" >&2
    done
    echo "Using the first one: ${candidates[0]}" >&2
  fi

  echo "${candidates[0]}"
  return 0
}
