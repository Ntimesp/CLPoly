#!/bin/sh
# 构建理论 PDF，中间文件写入临时目录，不留在仓库中。
set -eu
cd "$(dirname "$0")"
cad_tex_build=$(mktemp -d "${TMPDIR:-/tmp}/clpoly-theory.XXXXXX")
trap 'rm -rf "$cad_tex_build"' EXIT HUP INT TERM
for cad_tex_pass in 1 2 3; do
    if ! xelatex -interaction=nonstopmode -halt-on-error -output-directory="$cad_tex_build" lazard-evaluation.tex > "$cad_tex_build/build.log" 2>&1; then
        cat "$cad_tex_build/build.log"
        exit 1
    fi
done
if rg 'undefined references|Label\(s\) may have changed|Overfull' "$cad_tex_build/lazard-evaluation.log"; then
    echo 'Unresolved TeX references or overflowing boxes; PDF not published.' >&2
    exit 1
fi
cp "$cad_tex_build/lazard-evaluation.pdf" lazard-evaluation.pdf
printf '%s\n' 'Built lazard-evaluation.pdf'
