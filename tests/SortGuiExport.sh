#!/bin/bash

function usage() {
    cat << EOM

Usage: $(basename "$0") [-h] <input-file>
    -h help

    Sorts the input file by full path, case-insensitive, and writes the result to a file with the same name plus the suffix ".out"

EOM

    exit 1
}

while getopts h opt; do
    case $opt in
        *)
            usage
            ;;
    esac
done

shift "$((OPTIND-1))"

if [[ "$#" != 1 ]] ; then
    usage
fi

java -classpath /r/ciobi/Java/SortMp3DiagsExport/target/classes org.example.Main <"$1" >"$1".out
