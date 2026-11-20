# path_map.sh -- sourced by derive_args.sh (derive_c1.sh, derive_c5.sh, derive_plan.sh) and env/redesign/one_build.sh:
# the mechanical mapping of the reference build's absolute paths into a derived work space.
#
# A generated plan names paths of the build its facts were extracted from (module table prefixes, executable list;
# kz-static plan writes them from the basis).  A derived build is made in another directory from copies of the same
# inputs, so each of those paths is rewritten by the copy it lies in.  The pairs come from a map file, never from a
# fixed value in a script: derive_c1.sh prepare writes <W>/path_map.tsv from the copies it makes (one line per copied
# tree: the reference directory and the directory of its copy).
#
# Map file: '#' lines are comments; every other line is "<reference prefix><TAB><derived prefix>", both absolute and
# ending with '/'; a reference prefix appears once.  A path is mapped by the longest reference prefix it starts with
# (a directory equal to a prefix without its '/' maps too); a relative path is left as it is (it is relative to a root
# the caller chooses, e.g. @ROOT/ or the install root); an absolute path that no line covers is refused.
# Functions (they return 1 with a message on stderr):
#   path_map_load FILE    read and check FILE; sets PM_FILE, PM_FROM[], PM_TO[] (longest prefix first)
#   path_map_apply PATH   print PATH mapped by the loaded map
PM_FILE=; PM_FROM=(); PM_TO=()
path_map_load() {
  local f=$1 from to rest n=0 k
  PM_FILE=; PM_FROM=(); PM_TO=()
  [ -s "$f" ] || { echo "path map: $f is missing or empty" >&2; return 1; }
  local -a lines=()
  while IFS= read -r l || [ -n "$l" ]; do
    n=$((n + 1))
    case "$l" in ''|'#'*) continue ;; esac
    IFS=$'\t' read -r from to rest <<< "$l"
    [ -n "$from" ] && [ -n "$to" ] && [ -z "$rest" ] || { echo "path map $f line $n: want <reference prefix><TAB><derived prefix>" >&2; return 1; }
    case "$from$to" in *$'\r'*) echo "path map $f line $n: carriage return" >&2; return 1 ;; esac
    for k in "$from" "$to"; do
      case "$k" in /*/) ;; *) echo "path map $f line $n: $k is not an absolute directory prefix ending with /" >&2; return 1 ;; esac
    done
    for k in "${lines[@]}"; do [ "${k%%$'\t'*}" != "$from" ] || { echo "path map $f line $n: reference prefix $from appears twice" >&2; return 1; }; done
    lines+=("$from"$'\t'"$to")
  done < "$f"
  [ ${#lines[@]} -gt 0 ] || { echo "path map: $f has no mapping line" >&2; return 1; }
  # longest reference prefix first (ties cannot happen: the prefixes are distinct)
  while IFS=$'\t' read -r _ from to; do PM_FROM+=("$from"); PM_TO+=("$to"); done < <(
    printf '%s\n' "${lines[@]}" | awk -F'\t' '{print length($1) "\t" $0}' | LC_ALL=C sort -t$'\t' -k1,1nr -k2,2)
  PM_FILE=$f
}
path_map_apply() {
  local p=$1 i
  case "$p" in /*) ;; *) printf '%s\n' "$p"; return 0 ;; esac
  [ -n "$PM_FILE" ] || { echo "path map: no map loaded for $p" >&2; return 1; }
  for i in "${!PM_FROM[@]}"; do
    if [ "${p#"${PM_FROM[$i]}"}" != "$p" ]; then printf '%s\n' "${PM_TO[$i]}${p#"${PM_FROM[$i]}"}"; return 0; fi
    if [ "$p/" = "${PM_FROM[$i]}" ]; then printf '%s\n' "${PM_TO[$i]%/}"; return 0; fi
  done
  echo "path map $PM_FILE: no line covers $p" >&2
  return 1
}
