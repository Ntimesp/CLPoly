"""后台全量设施测试；不运行真实大题，不修改生产 CAD。"""
import json
import subprocess
import sys
import tempfile
import concurrent.futures
import time
import unittest
from pathlib import Path

import full_benchmark as full


class FullBenchmarkTests(unittest.TestCase):
    def test_journal_recovery_preserves_incomplete_tail(self):
        with tempfile.TemporaryDirectory() as directory:
            out = Path(directory)
            full.append_event(out, {'type':'result','path':'a','phase':'convert','status':'converted'})
            with (out/'events.jsonl').open('ab') as stream:stream.write(b'{"incomplete":')
            self.assertEqual(len(full.read_events(out)),1)
            self.assertEqual(len(full.read_events(out,repair=True)),1)
            self.assertEqual(len(list(out.glob('events.incomplete.*'))),1)
            full.append_event(out, {'type':'result','path':'a','phase':'verify','status':'equivalent'})
            self.assertEqual(len(full.read_events(out)),2)

    def test_atomic_status_updates_allow_concurrent_queries(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'summary.json'
            def writer(index):
                for i in range(20):
                    full.atomic_json(path,{'writer':index,'iteration':i,'payload':str(index)*1000})
            with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
                list(pool.map(writer,range(4)))
            result=json.loads(path.read_text())
            self.assertEqual(result['payload'],str(result['writer'])*1000)
            self.assertFalse(list(path.parent.glob('*.tmp.*')))

    def test_conversion_and_mapping_are_saved_before_native_use(self):
        with tempfile.TemporaryDirectory() as directory:
            out=Path(directory);source=out/'case.smt2';txt=out/'case.txt';mapping=out/'case.json'
            source.write_text('(set-logic QF_NRA)(declare-fun |a b| () Real)(assert (= |a b| 1))(check-sat)')
            fake=out/'check';fake.write_text('#!/bin/sh\nprintf "VALID\\n"\n');fake.chmod(0o755)
            result=full.convert_one(source,txt,mapping,fake)
            self.assertEqual(result['status'],'converted')
            metadata=json.loads(mapping.read_text())
            self.assertEqual(metadata['aliases'],{'a b':'v0'})
            self.assertTrue(txt.read_text().startswith('v0\n\n'))
            self.assertEqual(metadata['txt_sha256'],full.benchmark.digest(txt))
            self.assertEqual(metadata['source_sha256'],full.benchmark.digest(source))

    def test_conversion_failure_does_not_become_cad_timeout(self):
        with tempfile.TemporaryDirectory() as directory:
            out=Path(directory);source=out/'case.smt2'
            source.write_text('(set-logic QF_NRA)(declare-fun b () Bool)(assert b)(check-sat)')
            result=full.convert_one(source,out/'case.txt',out/'case.json',Path('/not/used'))
            self.assertEqual(result['status'],'unsupported')
            self.assertFalse((out/'case.txt').exists())
        self.assertEqual(full.classify_phase('convert',{'status':'timeout'})['status'],'conversion_timeout')
        self.assertEqual(full.classify_phase('verify',{'status':'timeout'})['status'],'verification_timeout')
        self.assertEqual(full.classify_phase('clpoly',{'status':'timeout'})['status'],'timeout')

    def test_timeout_kills_descendant_even_if_outer_worker_is_gone(self):
        with tempfile.TemporaryDirectory() as directory:
            out=Path(directory);ready=out/'ready';late=out/'late';result=out/'result.json'
            child='from pathlib import Path; import time; Path('+repr(str(ready))+').write_text("ready"); time.sleep(1); Path('+repr(str(late))+').write_text("late")'
            parent='import subprocess,sys,time; subprocess.Popen([sys.executable,"-c",'+repr(child)+']); time.sleep(5)'
            command=[sys.executable,'-c',parent]
            supervisor=subprocess.Popen([sys.executable,str(Path(full.__file__).resolve()),'--stage','call','--out',str(out),'--command',json.dumps(command),'--result-file',str(result),'--timeout','.3'])
            supervisor.wait(timeout=5)
            self.assertTrue(ready.exists())
            self.assertEqual(json.loads(result.read_text())['status'],'timeout')
            time.sleep(1.1)
            self.assertFalse(late.exists())

    def test_supervisor_survives_launcher_exit(self):
        with tempfile.TemporaryDirectory() as directory:
            out=Path(directory);result=out/'result.json'
            command=[sys.executable,'-c','import time; time.sleep(.2); print("SAT")']
            args=[sys.executable,str(Path(full.__file__).resolve()),'--stage','call','--out',str(out),'--command',json.dumps(command),'--result-file',str(result),'--timeout','2']
            launcher='import subprocess; subprocess.Popen('+repr(args)+', start_new_session=True, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)'
            subprocess.run([sys.executable,'-c',launcher],check=True)
            deadline=time.monotonic()+5
            while not result.exists() and time.monotonic()<deadline:time.sleep(.05)
            self.assertTrue(result.exists())
            self.assertEqual(json.loads(result.read_text())['status'],'sat')

    def test_frozen_worker_pause_resume_and_native_txt_invocation(self):
        with tempfile.TemporaryDirectory() as directory:
            out=Path(directory);frozen=out/'frozen';frozen.mkdir();(out/'control').mkdir()
            for name in ['full_benchmark.py','adapter.py','benchmark.py']:
                (frozen/name).write_bytes(Path(full.__file__).with_name(name).read_bytes())
            (frozen/'verify.py').write_text('import json; print(json.dumps({"status":"equivalent"}))')
            cad=frozen/'cad';cad.write_text('#!/bin/sh\nif [ "$1" = "--check" ]; then printf "VALID\\n"; else printf "SAT\\n"; fi\n');cad.chmod(0o755)
            z3=frozen/'z3';z3.write_text('#!/bin/sh\nprintf "SAT\\n"\n');z3.chmod(0o755)
            source=out/'inputs'/'case.smt2';source.parent.mkdir()
            source.write_text('(set-logic QF_NRA)(declare-fun x () Real)(assert (= x 1))(check-sat)')
            full.atomic_json(out/'manifest.json',{'cases':[{'path':'case.smt2','sha256':full.benchmark.digest(source),'declared_status':'sat'}]})
            hashes={str(p):full.benchmark.digest(p) for p in frozen.iterdir() if p.is_file()}
            full.atomic_json(out/'environment.json',{'binary':str(cad),'z3':str(z3),'sha256':hashes,'conversion_timeout_seconds':10,'verification_timeout_seconds':10})
            (out/'control/pause').write_text('test pause')
            args=[sys.executable,str(frozen/'full_benchmark.py'),'--stage','worker','--out',str(out)]
            worker=subprocess.Popen(args,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
            try:
                deadline=time.monotonic()+5
                while time.monotonic()<deadline:
                    if (out/'status.json').exists() and json.loads((out/'status.json').read_text())['state']=='paused':break
                    time.sleep(.02)
                self.assertEqual(json.loads((out/'status.json').read_text())['state'],'paused')
                self.assertFalse((out/'events.jsonl').exists())
                (out/'control/pause').unlink()
                _,stderr=worker.communicate(timeout=15)
                self.assertEqual(worker.returncode,0,stderr)
                records=full.read_events(out)
                results=full.completed(records)
                self.assertEqual([results[('case.smt2',p)]['status'] for p in ['convert','verify','z3','clpoly']],['converted','equivalent','sat','sat'])
                self.assertEqual([Path(p).resolve() for p in results[('case.smt2','clpoly')]['command']],[cad.resolve(),(out/'converted/case.smt2.txt').resolve()])
                self.assertEqual(json.loads((out/'mappings/case.smt2.json').read_text())['aliases'],{'x':'v0'})
                completed_count=len(records)
                rerun=subprocess.run(args,capture_output=True,text=True,timeout=5)
                self.assertEqual(rerun.returncode,0,rerun.stderr)
                self.assertEqual(len(full.read_events(out)),completed_count)
            finally:
                if worker.poll() is None:
                    (out/'control/stop').write_text('test cleanup')
                    worker.wait(timeout=15)

    def test_last_result_wins_and_interrupted_is_resumable(self):
        records=[{'type':'result','path':'a','phase':'clpoly','status':'interrupted'},
                 {'type':'result','path':'b','phase':'clpoly','status':'timeout'},
                 {'type':'result','path':'b','phase':'clpoly','status':'unsat'}]
        result=full.completed(records)
        self.assertNotIn(('a','clpoly'),result)
        self.assertEqual(result[('b','clpoly')]['status'],'unsat')

    def test_summary_separates_processing_completion_from_answers(self):
        with tempfile.TemporaryDirectory() as directory:
            out=Path(directory)
            full.atomic_json(out/'manifest.json',{'cases':[{'path':'a','declared_status':'unsat'},{'path':'b','declared_status':'sat'}]})
            for phase,status,seconds in [('convert','converted',.01),('verify','equivalent',.01),('z3','unsat',.5),('clpoly','unsat',.1)]:
                full.append_event(out,dict(type='result',path='a',phase=phase,status=status,wall_seconds=seconds))
            for phase,status in [('convert','unsupported'),('verify','not_run'),('z3','sat'),('clpoly','not_run')]:
                full.append_event(out,dict(type='result',path='b',phase=phase,status=status,wall_seconds=None))
            result=full.summarize(out)
            self.assertEqual(result['complete_cases'],2)
            self.assertEqual(result['common_decided'],1)
            self.assertEqual(len(result['cad_faster_unsat']),1)
            self.assertEqual(result['completed_calls']['clpoly'],{'unsat':1,'not_run':1})


if __name__=='__main__':unittest.main(verbosity=2)
