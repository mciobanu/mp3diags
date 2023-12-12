#!/bin/bash

function checkAndPrintVar {
    local v
    eval v='$'"$1"
    echo "$1: $v"
    [[ -n $v ]] || { echo "$1 is empty; exiting ..." ; exit 1 ; }
}

SCRIPT_FULL_PATH=$(realpath "$0")
checkAndPrintVar SCRIPT_FULL_PATH
SCRIPT_DIR=$(dirname "$SCRIPT_FULL_PATH")
checkAndPrintVar SCRIPT_DIR

function usage {
    cat << EOM

Usage: $(basename "$0") [-h] [-f]
    -h help
    -f just copy the files, without running exports or transformations

    Runs tests. Currently this is a partially manual process, which consists in:
        - Copying a "source folder" containing MP3 files to a "work folder"
        - Scanning the work folder and exporting the results to a text file inside mp3diags/tests/results
        - Applying the 4th custom transformation list with the default settings
        - Scanning the work folder again and exporting the results to another text file inside mp3diags/tests/results
        - Manually comparing the exported text files with older versions and drawing conclusions from the this
        - Manually deleting the processed files (this is not automatic in order to allow files to be examined)

    A file named SetPaths.sh must be created in the same folder as $(basename "$0") ("$SCRIPT_DIR")
    and it must define these environment variables:
        MP3DIAGS_EXE - (full) path to the executable
        TEST_DIR_SOURCE - folder containing the MP3 files that are going to be processed
        WORK_DIR - empty or non-existing folder on a partition with enough space and access rights where the files
            fill be copied to, scanned, and processed

EOM
    #ttt0 compare with previous, delete if nothing changed, ...

    exit 1
}


while getopts hf opt; do
    case $opt in
        f)
            JUST_FILES=true
            ;;
        *)
            usage
            ;;
    esac
done

shift "$((OPTIND-1))"

if [[ "$#" != 0 ]] ; then
    usage
fi

source "$SCRIPT_DIR"/SetPaths.sh

TEST_DIR_SOURCE=$(realpath "$TEST_DIR_SOURCE")  # to remove trailing slash, if present
WORK_DIR=$(realpath "$WORK_DIR")  # to remove trailing slash, if present

checkAndPrintVar TEST_DIR_SOURCE
checkAndPrintVar WORK_DIR

if [[ ! -d "$WORK_DIR" ]]; then
    mkdir -p "$WORK_DIR" || { echo "Failed to create folder $WORK_DIR" ; exit 1 ; }
fi

set -x
rsync -a --delete "$TEST_DIR_SOURCE"/ "$WORK_DIR"/
set +x

[[ -n $JUST_FILES ]] && exit 0

checkAndPrintVar MP3DIAGS_EXE
checkAndPrintVar SESSION
checkAndPrintVar RESULTS_DIR

set -x
"$MP3DIAGS_EXE" --session "$SESSION" --format long --severity support "$WORK_DIR" | tee "$RESULTS_DIR/$(date '+%Y.%m.%d_%H.%M.%S').initial.txt"
"$MP3DIAGS_EXE" --session "$SESSION" --format long --severity support --transf-list 4 "$WORK_DIR" | tee "$RESULTS_DIR/$(date '+%Y.%m.%d_%H.%M.%S').transf.txt"
"$MP3DIAGS_EXE" --session "$SESSION" --format long --severity support "$WORK_DIR" | tee "$RESULTS_DIR/$(date '+%Y.%m.%d_%H.%M.%S').after-trans.txt"

# Process for older version, which don't have "--format long":
#   Run "RunTests.sh -f" to get correct files without running exports or transformations
#   Start old version in GUI
#   Abort parse
#   In Options make all notes visible and don't restart MP3 Diags after that or play with sessions, as a bug will hide some notes
#   Force reparse
#   Export as text
#   Apply list 4
#   Export as text
#   Exit
#   Run SortGuiExport.sh over the exported files
#   Manually sort what's left (default collation is different between Java and Qt)
#   Compare
