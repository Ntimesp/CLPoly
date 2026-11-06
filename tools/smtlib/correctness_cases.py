"""从官方原库固定选取小规模 SAT/UNSAT 正确性样本，不按求解结果换题。"""
import argparse
import hashlib
import json
import re
from collections import defaultdict
from pathlib import Path

import adapter
import benchmark


def select(archive_path, out, per_status):
    candidates = defaultdict(lambda: defaultdict(list))
    scanned = 0
    with benchmark.archive_stream(archive_path) as archive:
        for member in archive:
            if not member.isfile() or not member.name.endswith('.smt2') or member.size > 16384:
                continue
            text = archive.extractfile(member).read().decode('utf-8')
            # 仅作扫描预筛；正式条件在转换后按声明与实际表检查。
            if len(re.findall(r'\(\s*declare-(?:fun|const)\b', text)) > 8:
                continue
            scanned += 1
            try:
                txt, ctx, clauses = adapter.convert(text)
            except (adapter.Unsupported, RecursionError):
                continue
            if (not 1 <= len(ctx.names) <= 3 or len(ctx.polynomials) > 8 or len(clauses) > 16
                    or max(map(len, ctx.polynomials), default=0) > 2000 or ctx.expected not in ('sat', 'unsat')):
                continue
            parts = Path(member.name).parts
            if parts[:2] != ('non-incremental', 'QF_NRA') or '..' in parts:
                raise ValueError(member.name)
            row = dict(path=member.name, bytes=member.size, sha256=hashlib.sha256(text.encode()).hexdigest(),
                       variables=len(ctx.names), polynomials=len(ctx.polynomials), clauses=len(clauses),
                       expected=ctx.expected, txt_sha256=hashlib.sha256(txt.encode()).hexdigest())
            candidates[ctx.expected][parts[2]].append((row, text))
    selected = []
    for status in ['sat', 'unsat']:
        families = candidates[status]
        for files in families.values():
            files.sort(key=lambda item: hashlib.sha256(('20261106:correctness:' + item[0]['path']).encode()).hexdigest())
        taken = 0
        for index in range(max(map(len, families.values()), default=0)):
            for name in sorted(families):
                if index < len(families[name]):
                    row, text = families[name][index]
                    path = out / 'inputs' / row['path']
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_text(text)
                    selected.append(row)
                    taken += 1
                if taken == per_status: break
            if taken == per_status: break
        if taken != per_status:
            raise ValueError(f'only {taken} candidates for {status}')
    selected.sort(key=lambda row: (Path(row['path']).parts[2], row['expected'], row['path']))
    benchmark.dump(out / 'selection.json', {
        'release': '2025.08.04', 'url': 'https://zenodo.org/records/16740866', 'logic': 'QF_NRA',
        'seed': '20261106:correctness',
        'rule': 'Original file <=16KiB; supported polynomial subset; 1..3 Real variables, <=8 polynomial entries, <=16 CNF clauses, each expression <=2000 chars. Within each known status, round robin families and fixed SHA256 path order. No solver-result filtering or replacement.',
        'candidate_counts': {status: {family: len(files) for family, files in sorted(groups.items())}
                             for status, groups in candidates.items()},
        'selected': selected,
    })
    print('selected', len(selected), 'from', sum(len(files) for groups in candidates.values() for files in groups.values()),
          'supported small candidates;', scanned, 'files inspected', flush=True)


def prepare(out):
    manifest = json.loads((out / 'selection.json').read_text())
    for row in manifest['selected']:
        source = out / 'inputs' / row['path']
        if benchmark.digest(source) != row['sha256']:
            raise ValueError('source changed: ' + row['path'])
        txt, ctx, clauses = adapter.convert(source.read_text())
        if hashlib.sha256(txt.encode()).hexdigest() != row['txt_sha256']:
            raise ValueError('conversion changed: ' + row['path'])
        target = out / 'converted' / (row['path'] + '.txt')
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(txt)
        mapping = out / 'mappings' / (row['path'] + '.json')
        mapping.parent.mkdir(parents=True, exist_ok=True)
        benchmark.dump(mapping, {'source_sha256': row['sha256'], 'txt_sha256': row['txt_sha256'],
                                 'variable_names': ctx.aliases, 'polynomials': len(ctx.polynomials),
                                 'clauses': len(clauses)})
    print('prepared TXT and mappings:', len(manifest['selected']), flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--archive', type=Path)
    parser.add_argument('--prepare-only', action='store_true')
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--per-status', type=int, default=48)
    args = parser.parse_args()
    if args.per_status <= 0: parser.error('--per-status must be positive')
    args.out.mkdir(parents=True, exist_ok=True)
    if not args.prepare_only:
        if args.archive is None: parser.error('--archive is required')
        select(args.archive, args.out, args.per_status)
    prepare(args.out)
