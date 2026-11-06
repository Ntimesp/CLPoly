"""在全量自然暂停窗口，复测一个已核对的共同 UNSAT 候选。"""
import argparse
import json
import statistics
import time
import uuid
from pathlib import Path

import benchmark
import full_benchmark as full


def repeat(out, relative, count):
    if count <= 0:
        raise ValueError('repeat count must be positive')
    environment=json.loads((out/'environment.json').read_text())
    full.check_hashes(environment)
    results=full.completed(full.read_events(out))
    for phase,status in [('convert','converted'),('verify','equivalent'),('z3','unsat'),('clpoly','unsat')]:
        if (relative,phase) not in results or results[(relative,phase)]['status'] != status:
            raise ValueError('candidate must already have equivalent conversion and two UNSAT answers')
    mapping=json.loads((out/'mappings'/(relative+'.json')).read_text())
    if not mapping['variables']:
        raise ValueError('constant/trivial input: not a nonconstant CAD candidate')
    source=out/'inputs'/relative;txt=out/'converted'/(relative+'.txt')
    if benchmark.digest(source)!=mapping['source_sha256'] or benchmark.digest(txt)!=mapping['txt_sha256']:
        raise ValueError('candidate input changed')
    marker=out/'control/pause'
    token='candidate retest '+uuid.uuid4().hex
    with marker.open('x') as stream:stream.write(token)
    record={'path':relative,'source_sha256':mapping['source_sha256'],'txt_sha256':mapping['txt_sha256'],
            'variables':mapping['variables'],'repeats':count,'pause_requested_at':full.utc(),'calls':[]}
    target=out/'retests'/(Path(relative).name+'.json')
    try:
        deadline=time.monotonic()+30
        while json.loads((out/'status.json').read_text())['state']!='paused':
            if time.monotonic()>deadline:raise TimeoutError('worker did not reach pause boundary')
            time.sleep(.05)
        record['pause_confirmed_at']=full.utc()
        commands={'clpoly':[environment['binary'],str(txt)],'z3':[environment['z3'],'-smt2',str(source)]}
        for iteration in range(count):
            for solver in (['clpoly','z3'] if iteration%2==0 else ['z3','clpoly']):
                call=benchmark.run_process(commands[solver],10)
                record['calls'].append(dict(iteration=iteration+1,solver=solver,**call))
                full.atomic_json(target,record)
        all_unsat=all(c['status']=='unsat' for c in record['calls'])
        record['all_unsat']=all_unsat
        if all_unsat:
            record['statistics']={solver:{'median_seconds':statistics.median(c['wall_seconds'] for c in record['calls'] if c['solver']==solver),
                                          'min_seconds':min(c['wall_seconds'] for c in record['calls'] if c['solver']==solver),
                                          'max_seconds':max(c['wall_seconds'] for c in record['calls'] if c['solver']==solver)} for solver in ('clpoly','z3')}
            record['cad_lower_median']=record['statistics']['clpoly']['median_seconds']<record['statistics']['z3']['median_seconds']
        full.check_hashes(environment)
    finally:
        record['finished_at']=full.utc()
        full.atomic_json(target,record)
        if marker.exists() and marker.read_text()==token:marker.unlink()
    return record


def main():
    p=argparse.ArgumentParser()
    p.add_argument('--out',type=Path,required=True)
    p.add_argument('--case',required=True)
    p.add_argument('--repeats',type=int,default=5)
    a=p.parse_args()
    print(json.dumps(repeat(a.out.resolve(),a.case,a.repeats),ensure_ascii=False,indent=2))


if __name__=='__main__':main()
