#!/usr/bin/env bash

set -ueo pipefail
readonly tmpdir=$(mktemp -d)
trap "rm -rf ${tmpdir}" EXIT

cp ${1}.yucc src/include/${1}.yucc
grep -E -o 'DEFAULT_[A-Z_]* .*' src/include/config.h | sort | uniq | tac \
    > "${tmpdir}/defaults.def"
sed 's; ;=;g' "${tmpdir}/defaults.def" > "${tmpdir}/defaults.sh"
sed -i 's;";\\\\\\\\\\";g' "${tmpdir}/defaults.sh"
cut -d ' ' -f1 "${tmpdir}/defaults.def" > "${tmpdir}/defaults.lst"
source "${tmpdir}/defaults.sh"
while read -r line; do
    sed -i "s;${line};${!line};g" "src/include/${1}.yucc"
done < "${tmpdir}/defaults.lst"
