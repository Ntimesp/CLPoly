(* 直接输入题集原始公式，不使用手工不等式证明或答案见证。 *)
ClearAll["Global`*"];
$HistoryLength = 0;
formula054 = x > 0 && y > 0 && z > 0 &&
  x^2 y z + x y^2 z + x y z^2 - 1 == 0 &&
  x y + x z + y^2 + y z - 2 < 0;
formula055 = x > 0 && y > 0 && z > 0 &&
  x^2 y z + x y^2 z + x y z^2 - 1 == 0 &&
  x y + x z + y^2 + y z - 2 == 0;

method = Environment["CLPOLY_MMA_METHOD"];
If[!MemberQ[{"resolve", "reduce", "cadxyz", "cadzyx"}, method],
  Print["Set CLPOLY_MMA_METHOD to resolve, reduce, cadxyz or cadzyx."];
  Exit[2];
];
outputDirectory = Environment["CLPOLY_MMA_OUT"];
If[!StringQ[outputDirectory], outputDirectory = DirectoryName[$InputFileName]];
results = {};
SetAttributes[measure, HoldRest];
measure[name_, expression_] := Module[{timing, result, record},
  {timing, result} = AbsoluteTiming[TimeConstrained[expression, 30, $TimedOut]];
  record = <|"name" -> name, "seconds" -> timing,
    "result" -> ToString[result, InputForm]|>;
  AppendTo[results, record];
  Print[name, " : ", result, " / ", timing, " seconds"];
];

(* 每个方法由外部进程启动独立内核，首轮包含该方法的首次加载成本。 *)
Do[
  Switch[method,
    "resolve", measure["054 Resolve / run " <> ToString[i],
      Resolve[Exists[{x, y, z}, formula054], Reals]],
    "reduce", measure["054 Reduce xyz / run " <> ToString[i],
      Reduce[formula054, {x, y, z}, Reals]],
    "cadxyz", measure["054 CylindricalDecomposition xyz / run " <> ToString[i],
      CylindricalDecomposition[formula054, {x, y, z}]],
    "cadzyx", measure["054 CylindricalDecomposition zyx / run " <> ToString[i],
      CylindricalDecomposition[formula054, {z, y, x}]]
  ],
  {i, 1, 3}
];
If[method == "resolve",
  measure["055 Resolve", Resolve[Exists[{x, y, z}, formula055], Reals]]
];
Export[FileNameJoin[{outputDirectory, "mma-054-" <> method <> ".json"}],
  <|"version" -> $Version, "system" -> $SystemID,
    "method" -> method, "results" -> results|>, "RawJSON"];
