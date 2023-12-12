#!/bin/bash

function usage() {
    cat << EOM

Usage: $(basename "$0") [-h]
    -h help

    Converts a text file that was exported from Windows to one that can be used in comparisons with Linux exports

    Sorts the input file as in a CLI export (mostly, collation differs between Java and Qt, so manual adjustments are applied),
    Replaces CRLF with LF (side-effect of sorting)
    Replaces "D:" with "/d"
    Replaces "\" in paths (but not in printed streams) with "/"

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

if [[ $# != 1 ]] ; then
    usage
fi


#fileName=wnd2b.bef.txt
#fileName=q
fileName="$1"

"$(dirname "$(realpath "$0")")"/SortGuiExport.sh "$fileName"
cat "$fileName".out | sed -e '/^D/ s#\\#/#g' -e 's#D:#/d#' > "$fileName".out.slash
