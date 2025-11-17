#!/bin/sh

usage() {
    echo "Usage: $0 --path-config <file> [--path-bin <bin>] [--daemon <options>]"
    echo
    echo "Options:"
    echo "  --path-config <fichier>     (mandatory) path to the config file"
    echo "  --path-bin <chemin-binaire> (optional) path to the httpd binary, default is ./httpd"
    echo "  --daemon <options>          (optional) option passed to httpd"
    echo "  -h, --help                  prints this help page"
    exit 1
}

OPTIONS=$(getopt -o h --long path-config:,path-bin:,daemon:,help -- "$@")
if [ $? -ne 0 ]; then
    usage
fi

eval set -- "$OPTIONS"

PATH_CONFIG=""
PATH_BIN="./httpd"
DAEMON_OPTS=""

# Lecture des arguments
while true; do
    case "$1" in
        --path-config)
            PATH_CONFIG="$2"
            shift 2
            ;;
        --path-bin)
            PATH_BIN="$2"
            shift 2
            ;;
        --daemon)
            DAEMON_OPTS="--daemon $2"
            shift 2
            ;;
        -h|--help)
            usage
            ;;
        --)
            shift
            break
            ;;
        *)
            usage
            ;;
    esac
done

# verify mandatory option
if [ -z "$PATH_CONFIG" ]; then
    echo "error : --path-config is mandatory"
    echo
    usage
fi

! [ -f "$PATH_CONFIG" ] && (echo "config file: $PATH_CONFIG is not a file"; exit 1;)
PATH_BIN_TO_RUN="$PATH_BIN"

if [[ "$PATH_BIN_TO_RUN" != */* ]]; then
    PATH_BIN_TO_RUN="./$PATH_BIN_TO_RUN"
fi

[ -x "$PATH_BIN_TO_RUN" ] || (echo "error: binary file: '$PATH_BIN_TO_RUN' is not executable or does not exist."; exit 1;)

PATH_BIN="$PATH_BIN_TO_RUN"

# main
ARGS=""
regex="([^ ]+) ?= ?([^ ]+)"

while IFS= read -r line || [[ -n "$line" ]];
do
  [ "$line" = "[global]" ] || [ "$line" = "[[vhosts]]" ] && continue;

  if [[ $line =~ $regex ]]; then
    ARGS="$ARGS --${BASH_REMATCH[1]} ${BASH_REMATCH[2]}"
  fi
done < "$PATH_CONFIG"

# launch binary with correct args
echo "command run is: $PATH_BIN $DAEMON_OPTS ${ARGS:1}"
$PATH_BIN $DAEMON_OPTS ${ARGS:1}
