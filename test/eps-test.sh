#! /bin/sh
# Test driver for EPS files

# ----------------------------------------------------------------------
# Setup
export LC_ALL=C
cd tmp/
if [ -z "$EXIV2_BINDIR" ] ; then
    bin="$VALGRIND ../../bin"
else
    bin="$VALGRIND $EXIV2_BINDIR"
fi
exiv2version="`$bin/exiv2 -V | sed -n '1 s,^exiv2 [^ ]* \([^ ]*\).*,\1,p'`"
if [ -z "$exiv2version" ]; then
    echo "Error: Unable to determine Exiv2 version"
    exit 1
fi
diffargs="--strip-trailing-cr"
if ! diff -q $diffargs /dev/null /dev/null 2>/dev/null ; then
    diffargs=""
fi
for file in ../data/eps/eps-*.eps.*; do
    if ! grep "_Exiv2Version_" "$file" >/dev/null ; then
        echo "Error: $file contains hard-coded Exiv2 version"
        exit 1
    fi
done

# ----------------------------------------------------------------------
# Tests
(
    for file in ../data/eps/eps-*.eps; do
        image="`basename "$file" .eps`"

        printf "." >&3

        echo
        echo "-----> $image.eps <-----"

        cp "../data/eps/$image.eps" ./

        echo
        echo "Command: exiv2 -u -pa $image.eps"
        $bin/exiv2 -u -pa "$image.eps"
        exitcode="$?"
        echo "Exit code: $exitcode"

        if [ "$exitcode" -ne 0 -a "$exitcode" -ne 253 ] ; then
            continue
        fi

        echo
        echo "Command: exiv2 -dx $image.eps"
        $bin/exiv2 -dx "$image.eps"
        exitcode="$?"
        echo "Exit code: $exitcode"

        if [ "$exitcode" -eq 0 ] ; then
            # using perl instead of sed, because on some systems sed adds a line ending at EOF
            perl -pe "s,_Exiv2Version_,$exiv2version," < "../data/eps/$image.eps.delxmp" > "$image.eps.delxmp"

            if ! diff -q "$image.eps.delxmp" "$image.eps" ; then
                continue
            fi

            # Ensure that "exiv2 -ex" won't merge metadata into the
            # *.exv file generated by a previous run of the test suite.
            rm -f "$image.exv"

            echo
            echo "Command: exiv2 -f -ex $image.eps"
            $bin/exiv2 -f -ex "$image.eps"
            echo "Exit code: $?"

            if ! diff -q "../data/eps/eps-test-delxmp.exv" "$image.exv" ; then
                continue
            fi
        fi

        echo
        echo "Restore: $image.eps"
        cp "../data/eps/$image.eps" ./

        echo
        echo "Command: exiv2 -f -eX $image.eps"
        $bin/exiv2 -f -eX "$image.eps"
        echo "Exit code: $?"

        diff -q "../data/eps/$image.xmp" "$image.xmp"

        # Using "-ix" instead of "-iX" because the latter
        # executes writeMetadata() twice, making it hard to debug.

        cp "../data/eps/eps-test-newxmp.xmp" "$image.exv"

        echo
        echo "Command: exiv2 -ix $image.eps"
        $bin/exiv2 -ix "$image.eps"
        exitcode="$?"
        echo "Exit code: $exitcode"

        if [ "$exitcode" -ne 0 ] ; then
            continue
        fi

        # using perl instead of sed, because on some systems sed adds a line ending at EOF
        perl -pe "s,_Exiv2Version_,$exiv2version," < "../data/eps/$image.eps.newxmp" > "$image.eps.newxmp"

        if ! diff -q "$image.eps.newxmp" "$image.eps" ; then
            continue
        fi

        echo
        echo "Command: (2) exiv2 -ix $image.eps"
        $bin/exiv2 -ix "$image.eps"
        echo "Exit code: $?"

        diff -q "$image.eps.newxmp" "$image.eps"

        # Ensure that "exiv2 -ex" won't merge metadata into the
        # *.exv file generated by a previous run of the test suite.
        rm -f "$image.exv"

        echo
        echo "Command: exiv2 -f -ex $image.eps"
        $bin/exiv2 -f -ex "$image.eps"
        echo "Exit code: $?"

        diff -q "../data/eps/eps-test-newxmp.exv" "$image.exv"
    done
) 3>&1 > "eps-test.out" 2>&1

echo "."

# ----------------------------------------------------------------------
# Result
if ! diff -q $diffargs "../data/eps/eps-test.out" "eps-test.out" ; then
    diff -u $diffargs "../data/eps/eps-test.out" "eps-test.out"
    exit 1
fi
echo "All testcases passed."