#!/usr/bin/env bash
set -euo pipefail

ld_lld=${LLVM_AVR32_LD_LLD:-ld.lld}
empty_obj=${AVR32_EMPTY_OBJECT:-${TMPDIR:-/tmp}/empty-avr32-linkrelax.o}
avr32_as=${AVR32_AS:-avr32-as}

ensure_empty_object() {
  if [[ ! -f "$empty_obj" ]]; then
    mkdir -p "$(dirname "$empty_obj")"
    tmp=$(mktemp "${empty_obj}.XXXXXX")
    "$avr32_as" --linkrelax -o "$tmp" /dev/null
    mv "$tmp" "$empty_obj"
  fi
}

out=
partial=0
has_emulation=0
inputs=()
skip_next=0

for arg in "$@"; do
  if (( skip_next )); then
    skip_next=0
    continue
  fi

  case "$arg" in
    -r)
      partial=1
      ;;
    -o)
      skip_next=1
      ;;
    -T|-m|-L|-R|-Map|--script|--format|--oformat|--defsym)
      if [[ "$arg" == "-m" ]]; then
        has_emulation=1
      fi
      skip_next=1
      ;;
    -m*)
      has_emulation=1
      ;;
    -o*)
      out=${arg#-o}
      ;;
    --start-group|--end-group|--whole-archive|--no-whole-archive)
      ;;
    -*)
      ;;
    *)
      inputs+=("$arg")
      ;;
  esac
done

for ((i = 1; i <= $#; ++i)); do
  if [[ ${!i} == "-o" ]]; then
    next=$((i + 1))
    if (( next <= $# )); then
      out=${!next}
    fi
  fi
done

if (( partial )) && [[ ${#inputs[@]} -eq 0 && -n "$out" ]]; then
  ensure_empty_object
  cp "$empty_obj" "$out"
  exit 0
fi

if (( partial )); then
  ensure_empty_object
  set -- "$@" "$empty_obj"
fi

# Match GNU ld's final-link AVR32 relaxation behavior. Keep partial links
# relocatable and let the final vmlinux link decide which call pools are in
# direct-call range.
if (( has_emulation )); then
  if (( partial )); then
    exec "$ld_lld" "$@"
  fi
  exec "$ld_lld" --direct-data "$@"
fi

if (( partial )); then
  exec "$ld_lld" -m avr32elf "$@"
fi

exec "$ld_lld" -m avr32elf --direct-data "$@"
