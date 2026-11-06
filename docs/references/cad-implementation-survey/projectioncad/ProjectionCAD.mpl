# Projection CAD Library (V2.6)
# Matthew England (M.England@bath.ac.uk)
# 24th May 2013
print("This is V2.6 of the ProjectionCAD module from 24th May 2013, designed for use in Maple 16 / 17"):

ProjectionCAD := module()

description "ProjectionCAD module: Calculating various cylindrical algebraic decompositions via projection and lifting.":

option package:
# Telling Maple that the module is designed to be a Maple package

export
	CADProjection,
	CADLifting,
	CADFull,
	CADGenerateStack,
	ECCAD,
	ECCADProjFactors,
	ECCADProjOp,
	ECCADFormulations,
	ECCADHeuristic,
	NumCellsInPiecewiseCAD,
	NumCellsInCAD,
	TTICAD,
	TTICADResCADSet,
	TTICADResCAD,
	TTICADProjFactors,
	TTICADProjOp,
	TTICADQFFFormulations,
	TTICADFormulations,
	TTICADQFFHeuristic,
	TTICADHeuristic,
	VariableOrderings,
	VariableOrderingHeuristic,
	sotd,
	ndrr:

local
	PCAD_SortEqsRC,
	PCAD_LdCf,
	PCAD_MakeMonic,
	PCAD_MakeInteger,
	PCAD_RemoveConstantMultiples,
	PCAD_SFBasis,
	PCAD_ContSet,
	PCAD_PrimSet,
	PCAD_SetGCD,
	PCAD_SetFactors,
	PCAD_DoesIntervalIntersect,
	PCAD_CCADmat,
	PCAD_CCADpsc,
	PCAD_CCADpscset,
	PCAD_CCADreductum,
	PCAD_CCADreductumset,
	PCAD_CCADProj1,
	PCAD_CCADProj2,
	PCAD_CCADProj,
	PCAD_CCADProjPolys,
	PCAD_McCallumProj,
	PCAD_McCcoeffset,
	PCAD_McCProjPolys,
	PCAD_IsCellZeroDim,
	PCAD_IsCellFullDim,
	PCAD_MinimalDelineatingPolynomial,
	PCAD_MakeCoprimeOverRC,
	PCAD_WhichRCInSplitHasSolutionAtSP,
	PCAD_MakeSquareFreeOverRC,
	PCAD_SPtoRootOf,
	PCAD_SortCadCell,
	PCAD_GenerateStack,
	PCAD_ProjCADLift,
	PCAD_LWRCADtoPWCAD,
	TTI_ECReducedProjectionOperator,
	TTI_TTIProjectionOperator,
	TTI_ECCAD,
	TTI_ECPP,
	TTI_TTIPP,
	TTI_TTICAD,
	TTI_UnivariateCase,
	TTI_LowerDimCAD,
	TTI_TTIGenerateStack,
	TTI_ResCAD,
	TTI_ResCADSet,
	TTI_ExclProj_phi,
	Formulation_VarOrdFree,
	Formulation_VarOrdRestricted,
	Formulations_RGS,
	Formulations_Classification,
	Formulations_SetPartitions,
	Formulations_FunctionPartitions,
	Formulations_ECsForTTI,
	Formulations_ECCAD,
	Formulations_QFF,
	Formulations_TTICAD,
	Heuristics_sotdP,
	Heuristics_sotdL,
	Heuristics_ndrr,
	Heuristics_PickSuggestions,
	Heuristics_RunOnPPs,
	Heuristics_FindPos,
	Heuristics_Display,
	Heuristics_ECCAD,
	Heuristics_TTICADQFF,
	Heuristics_TTICAD,
	Heuristics_VariableOrdering,
	Heuristics_GVOCADFull,
	Heuristics_GVOCADFullBlocks:

# global
# 	`type/CADList`:

#####################################################################
###  CONTENTS
###  Section 1: User Commands = Just error checking
###  Section 2: Miscellaneous Commands
###  Section 3: Collins Projection
###  Section 4: McCallum projection
###  Section 5: Lifting
###  Section 6: ECCAD and TTICAD
###  Section 7: Formulations for CAD
###  Section 8: Heuristics
#####################################################################

#####################################################################
###  Section 1: User Commands
#####################################################################

CADProjection := proc(in_F::{list, set}, vars::list, {method::symbol:='McCallum'}, $) :: 'set':
local F:
description "CADProjection: Computes a set of projection polynomials for use in constructing a CAD.",
			"Input: A list or set of polynomials and a list of variables. The polynomials are defined over the variables with coefficients in Q. Also, an optional input specifying the method to use.",
			"Output: A list of polynomials.":

F := convert(in_F, 'set');
if F::'set(list(polynom))' then
	error("invalid input: %1 expects the first argument to be a list or set containing only polynomials.", procname):
fi:
if vars::'Not(list(symbol))' or vars=[] then
	error("invalid input: %1 expects the second argument to be a list of symbols.", procname):
fi:
F := expand(F):
if map(X->coeffs(X, vars), F)::'Not(set(rational))' then
	error("invalid input: %1 expects the polynomials in the first argument to be defined over the ring of variables in the second argument, with coefficients in the rationals", procname):
fi:
if method::'Not(identical(Collins, McCallum))' then
	error("invalid input: expected method = Collins, or McCallum, received: method = %1", method)
fi:
if not has([_passed],[_options][1]) then
	WARNING("no method was specified, McCallum's projection operator will be used"):
fi:
if method='McCallum' then
	return(PCAD_McCProjPolys(F,vars)):
elif method='Collins' then
	return(PCAD_CCADProjPolys(F,vars)):
fi:
end proc:

#####################################################################

CADLifting := proc(in_F::{list, set}, vars::list, {failure::symbol:='warn', finalCAD::symbol:='SI', method::symbol:='McCallum', output::symbol:=list, retcad::nonnegint:=0}, $) :: 'set':
local F,dim:
description "CADLifting: Computes a CAD in list format from a set of projection polynomials.",
			"Input: A set of projection polynomials and a list of variables. Optional arguments specifying the method and whether the final output should be order invariant.",
			"Output: A CAD in list format. Each entry is an index and a sample point (encoded by a regular chain and isolating cube).":

F := convert(in_F, 'set');
if F::'Not(set(polynom))' then
	error("invalid input: %1 expects the first argument to be a list or set containing only polynomials.", procname):
fi:
if vars::'Not(list(symbol))' then
	error("invalid input: %1 expects the second argument to be either a symbol, or list of symbols.", procname):
elif vars=[] then
	error("invalid input: %1 expects the second argument to be either a symbol, or list of symbols.", procname):
fi:
if map(X->coeffs(X, vars), F)::'Not(set(rational))' then
	error("invalid input: %1 expects the polynomials in the first argument to be defined over the ring of variables in the second argument, with coefficients in the rationals", procname):
fi:
F := expand(F):
if method::'Not(identical(Collins, McCallum))' then
	error("invalid input: expected method = Collins, or McCallum, received: method = %1", method)
fi:
if not has([_passed],[_options][3]) then
	WARNING("no method was specified. Assuming that the projection used was McCallum"):
fi:
if finalCAD::'Not'('identical(SI, OI)') then
	error("invalid input: expected finalCAD = SI or OI, received: finalCAD = %1", finalCAD)
fi:
if method='Collins' and finalCAD='OI' then
	error("the final CAD can only be guaranteed order invariant when using McCallum algorithm."):
fi:
dim:=nops(vars):
if retcad>dim then
	error("invalid input: %1 expects the optional argument retcad to be a non-negative integer less than or equal to the number of variables specified in the second argument.", procname):
fi:
if output::'Not(identical(list, listwithrep, piecewise, rootof))' then
	error("invalid input: expected output = list, or listwithrep, or piecewise, or rootof, received: output = %1", output)
fi:
if failure::'Not(identical(warn, err, giveFAIL))' then
	error("invalid input: expected failue = warn, or err, or giveFAIL, received: failure = %1", output):
fi:
if not has(packages(),RegularChains) then
	WARNING("the CAD has sample points encoded using regular chains. We recommend you load the RegularChains package for working with the output"):
fi:
PCAD_ProjCADLift( F, vars, method, finalCAD, retcad, output, failure ):
end proc:

#####################################################################

CADFull := proc(in_F::{list, set}, vars::list, {failure::symbol:='warn', finalCAD::symbol:='SI', method::symbol:='McCallum', output::symbol:=list, retcad::nonnegint:=0}, $) :: 'set':
local F, pset, dim:
description "CADFull: Computes a CAD in list format from a set of projection polynomials.",
			"Input: A list or set of polynomials and a list of variables. The polynomials are defined over the variables with coefficients in Q. Also, an optional input specifying the method to use and whether the final output should be order invariant.",
			"Output: A CAD in list format. Each entry is an index and a sample point (encoded by a regular chain and isolating cube).":

F := convert(in_F, 'set');
if F::'Not(set(polynom))' then
	error("invalid input: %1 expects the first argument to be a list or set containing only polynomials.", procname):
fi:
if vars::'Not(list(symbol))' then
	error("invalid input: %1 expects the second argument to be either a symbol, or list of symbols.", procname):
elif vars=[] then
	error("invalid input: %1 expects the second argument to be either a symbol, or list of symbols.", procname):
fi:
F := expand(F):
if map(X->coeffs(X, vars), F)::'Not(set(rational))' then
	error("invalid input: %1 expects the polynomials in the first argument to be defined over the ring of variables in the second argument, with coefficients in the rationals", procname):
fi:
if method::'Not(identical(Collins, McCallum))' then
	error("invalid input: expected method = Collins, or McCallum, received: method = %1", method):
fi:
if not has([_passed],[_options][3]) then
	WARNING("no method was specified, McCallum's algorithm will be used"):
fi:
if finalCAD::'Not'('identical(SI, OI)') then
	error("invalid input: expected finalCAD = SI or OI, received: finalCAD = %1", finalCAD):
fi:
if method='Collins' and finalCAD='OI' then
	error("ehe final CAD can only be guaranteed order invariant when using McCallum algorithm."):
fi:
dim:=nops(vars):
if retcad>dim then
	error("invalid input: %1 expects the optional argument retcad to be a non-negative integer less than or equal to the number of variables specified in the second argument.", procname):
fi:
if output::'Not(identical(list, listwithrep, piecewise, rootof))' then
	error("invalid input: expected output = list, or listwithrep, or piecewise, or rootof, received: output = %1", output):
fi:
if failure::'Not(identical(warn, err, giveFAIL))' then
	error("invalid input: expected failue = warn, or err, or giveFAIL, received: failure = %1", output):
fi:
if not has(packages(),RegularChains) then
	WARNING("the CAD has sample points encoded using regular chains. We recommend you load the RegularChains package for working with the output"):
fi:
if method='McCallum' then
	pset := PCAD_McCProjPolys(F,vars):
	userinfo(2, {'ProjectionCAD', 'CADFull'}, "produced set of", nops(pset), "projection factors using the", method, "algorithm."):
	userinfo(4, {'ProjectionCAD', 'CADFull'}, "the projection factors are:", pset):
elif method='Collins' then
	pset := PCAD_CCADProjPolys(F,vars):
	userinfo(2, {'ProjectionCAD', 'CADFull'}, "produced set of", nops(pset), "projection factors using the", method, "algorithm."):
	userinfo(4, {'ProjectionCAD', 'CADFull'}, "the projection factors are:", pset):
fi:
PCAD_ProjCADLift( pset, vars, method, finalCAD, retcad, output, failure ):
end proc:

#####################################################################

NumCellsInPiecewiseCAD:=proc( pwcad::piecewise, $ ) :: posint:
local i,size,num,nest:
description "NumCellsInPiecewiseCAD: Calculate the number of cells in a MapleCAD given in the piecewise output format.",
			"Input: A CAD in piecewise format. This could be output from either this module or the Regular Chains implementation.",
			"Output: The number of cells in the CAD.":

size:=nops(pwcad)/2:
num:=0:
for i from 1 to size do
	nest:=op(2*i,pwcad):
	if nest::piecewise then
		num:=num + NumCellsInPiecewiseCAD( nest ):
	else
		num:=num+1:
	fi:
od:
num:
end proc:

#####################################################################

NumCellsInCAD := proc( cad )
description "PCAD_NuMCellsInCAD: Gives the number of cells in a CAD of given output format.":

if cad::piecewise then
	WARNING("this command is assuming the input is a CAD in piecewise output format"):
	return(NumCellsInPiecewiseCAD(cad)):
else
	WARNING("this command is assuming the input is a CAD in either list, listwithrep, rootof or cadcell output format"):
	return(nops(cad)):
fi:
end proc:

#####################################################################

CADGenerateStack:=proc( cell::list, in_F::{list, set}, vars::list, {output::symbol:=list}, $ ) :: 'list':
local F, R, cellIndex, cellSP, SPrc, SPcube, retstack, alpha, cellN:
description "CADGenerateStack: Generate a stack over a cell given a set of polynomials. Uses TRDgenerate_stack but first error checks and preconditions polynomials so they are of the right format.",
			"Input: A cell from a CAD in list or listwithrep output format, a list or set of polynomials and a list of variables.  The polynomials should be defined over the variables and the cell be from a cad of dimension 1 less."
			"Output: A list of new cells forming a stack over the original cell.":

if vars::'Not(list(symbol))' then
	error("invalid input: %1 expects the second argument to be either a symbol, or list of symbols.", procname):
elif vars=[] then
	error("invalid input: %1 expects the second argument to be either a symbol, or list of symbols.", procname):
fi:
R:=RegularChains:-PolynomialRing(vars):
F := convert(in_F, 'list');
if F::'Not(list(polynom))' then
	error("invalid input: %1 expects the first argument to be a list or set containing only polynomials.", procname):
fi:
F := expand(F):
if map(X->coeffs(X, vars), F)::'Not(list(rational))' then
	error("invalid input: %1 expects the polynomials in the first argument to be defined over the ring of variables in the second argument, with coefficients in the rationals", procname):
fi:
if output::'Not(identical(list, listwithrep))' then
	error("invalid input: expected output = list, or listwithrep, received: output = %1", output)
fi:
cellN:=nops(cell):
if cellN=2 then
	if output='listwithrep' then
		error("invalid input: first argument is not a cell from a cad in listwithrep format"):
	fi:
elif cellN=3 then
	if output='list' then
	error("invalid input: first argument is not a cell from a cad in list format. If it is 'listwithrep' then specify output."):
	fi:
else
	error("invalid input: first argument is not a cell from a cad in list or listwithrep format, (incorrect number of entries)"):
fi:
cellIndex:=cell[1]:
cellSP:=cell[-1]:
if not cellIndex::'list(integer)' or nops(cellSP)<>2 then
	error("invalid input: first argument is not a cell from a cad in list or listwithrep format, (entries not correct type)"):
fi:
SPrc:=cellSP[1]:
SPcube:=cellSP[2]:
if SPrc['type']<>'regular_chain' or SPcube::'Not(list(list))' then
	error("invalid input: first argument is not a cell from a cad in list or listwithrep format, (SP entries not correct type)"):
fi:
alpha:=PCAD_SPtoRootOf( SPrc,SPcube, RegularChains:-PolynomialRing( vars[2..nops(vars)] ) ):
F:=remove(X->evalb(is( subs(alpha,X)=0 )), F):
retstack:=PCAD_GenerateStack(cell, F, R, 'output'):
retstack:=map(X->op(1,X),retstack):
end proc:

#####################################################################

ECCADProjOp := proc( E::set, A::set, vars::list(symbol), $ ) :: set:
local mvar, lvars, chk:
description "ECCADProjOp: Computes one application of the reduced Projection operator for equational constraints.",
			"Input: Sets E and A,  the ordered list of variables.",
			"Output: The set of polynomials obtained by applying the reduced projection operator.":

mvar := vars[1]:
lvars := remove(X -> X=mvar, vars):
if not E::'set'( 'polynom'( 'rational', vars )) then
	error("invalid input: %1 expects the entries of the set in the first argument to be polynomials in the variables stated.", procname):
fi:
chk := map(X -> has(X, vars[1]), E):
if chk<>{true} then
	error("invalid input: %1 expects the polynomials in the first set to all contain the main variable.", procname):
fi:
if not A::'set'( 'polynom'( 'rational', vars )) then
	error("invalid input: %1 expects the entries of the set in the first argument to be polynomials in the variables stated.", procname):
fi:
TTI_ECReducedProjectionOperator(E, A, mvar, lvars):
end proc:

#####################################################################

TTICADProjOp := proc( E::table, A::table, t::posint, vars::list(symbol), $ ) :: set:
local i, mvar, lvars, indE, indA, Epols, Apols, chk:
description "TTICADProjOp: Computes one application of the TTICAD Projection operator.",
			"Input: Tables E and A, an integer t, the ordered list of variables.  The integr t is the number of QFFs in Phi.",
			"The tables have entries for 1..t with each entry a set of polynomials.",
			"Output: The set of polynomials obtained by applying the TTI projection operator.":

mvar := vars[1]:
lvars := remove(X -> X=mvar, vars):
indE:=[indices(E, 'nolist')]:
if nops(indE)<>t or convert(indE,'set')<>{seq(i, i=1..t)} then
	error("invalid input: %1 expects the first argument to be a table indexed from 1 to t=%2.", procname, t):
fi:
indA:=[indices(A, 'nolist')]:
if nops(indA)<>t or convert(indA,'set')<>{seq(i, i=1..t)} then
	error("invalid input: %1 expects the second argument to be a table indexed from 1 to t=%2.", procname, t):
fi:
Epols:=map(X -> op(X), {entries(E, 'nolist')}):
if not Epols::'set'( 'polynom'( 'rational', vars )) then
	error("invalid input: %1 expects the entries of the table in the first argument to be polynomials in the variables stated.", procname):
fi:
chk := map(X -> has(X, vars[1]), Epols):
if chk<>{true} then
	error("invalid input: %1 expects the polynomials in the first table to all contain the main variable.", procname):
fi:
Apols:=map(X -> op(X), {entries(A, 'nolist')}):
if not Apols::'set'( 'polynom'( 'rational', vars )) then
	error("invalid input: %1 expects the entries of the table in the first argument to be polynomials in the variables stated.", procname):
fi:
TTI_TTIProjectionOperator(E, A, t, mvar, lvars):
end proc:

#####################################################################

ECCAD:=proc( IN::list, vars::list(symbol), {LiftAll::boolean:=false, output::symbol:=list, retcad::nonnegint:=0}, $ )
local f, gs, dim:
description "ECCAD: Compute the CAD using equational constraint at first level.",
			"Input: A list containing f, a polynomial representing an equational constraint and gs, a list of polynomials representing the other constraints.",
			"Also a list of ordered variables and optionally, an ouput choice."
			"Output: A CAD which is sign invariant for f and also for g on those cells where f=0.":

if nops(IN)<>2 then
	error("invalid input: %1 expects the first argument to be a list with first entry polynomial and second entry list of polynomials.", procname):
else
	f:=IN[1]:
	gs:=IN[2]:
fi:
dim := nops(vars):
if retcad>dim then
	error("invalid input: %1 expects the optional argument retcad to be a non-negative integer less than or equal to the number of variables specified in the second argument.", procname):
fi:
if not f::'polynom' ('rational', vars) then
	error("invalid input: %1 expects each list in the first argument to be a polynomial in the stated variables over the rationals.", procname):
fi:
if not gs::'list'( 'polynom' ('rational', vars) ) then
	error("invalid input: %1 expects the second argument to be a list of polynomials in the stated variables over the rationals.", procname):
fi:
if output::'Not(identical(list, listwithrep, piecewise, rootof))' then
	error("invalid input: expected output = list, or listwithrep, or piecewise, or rootof, received: output = %1", output)
fi:
TTI_ECCAD( [expand(f), expand(gs)], vars, LiftAll, output, retcad):
end proc:

#####################################################################

ECCADProjFactors:=proc( IN::list, vars::list(symbol), $ ) :: set:
local f, gs:
description "ECProjFactors: Compute the projection factors used for projection by ECCAD.",
			"Input: f, a polynomial representing an equational constraint and gs, a list of polynomials representing the other constraints.",
			"Also a list of ordered variables."
			"Output: The set of projection factors.":

if nops(IN)<>2 then
	error("invalid input: %1 expects the first argument to be a list with first entry polynomial and second entry list of polynomials.", procname):
else
	f:=IN[1]:
	gs:=IN[2]:
fi:
if not f::'polynom' ('rational', vars) then
	error("invalid input: %1 expects each list in the first argument to be a polynomial in the stated variables over the rationals.", procname):
fi:
if not gs::'list'( 'polynom' ('rational', vars) ) then
	error("invalid input: %1 expects the second argument to be a list of polynomials in the stated variables over the rationals.", procname):
fi:
TTI_ECPP( [expand(f), expand(gs)], vars):
end proc:

#####################################################################

TTICAD:=proc( PHI::list, vars::list(symbol), {LiftAll::boolean:=false, output::symbol:=list, retcad::nonnegint:=0}, $ )
local i, t, phi, f, gs, dim, ECs, PHIX, phix:
description "TTICAD: Compute the TTICAD for a list of QFFs.",
			"Input: PHI, a list of lists phi[i], each of which represent a QFF.  Each phi[i] has two arguments; the first a polynomial or list of polynomials (equational constraint) and the second a list of polynomials (other constraints).",
			"Also a list of ordered variables and optionally, an ouput choice and retcad level."
			"Output: A truth-table invariant CAD for PHI or failure if the polynomials are not well-oriented.":

t:=nops(PHI):
# Error checking
for i from 1 to t do
	phi := PHI[i]:
	if not phi::'list' then
		error("invalid input: %1 expects the first argument to be a list of lists (each representing a QFF).", procname):
	fi:
	if nops(phi)<>2 then
		error("invalid input: %1 expects each list in the first argument to consist of two components; but list %2 has %3 components", procname, i, nops(phi)):
	fi:
	gs := expand(phi[2]):
	if not gs::'list'( 'polynom' ('rational', vars) ) then
		error("invalid input: %1 expects each list in the first argument to consist of two parts, with the second a list of polynomials in the stated variables.  This is not the case for QFF %2.", procname, i):
	fi:
	ECs:=phi[1]:
	if ECs::'polynom' then
		f := expand(phi[1]):
		if not f::'polynom' ('rational', vars) then
			error("invalid input: %1 expects each list in the first argument to consist of two parts, with the first (the equational constraints) being polynomial in the stated variables.  This is not the case for QFF %2.", procname, i):
		fi:
	elif ECs::'list' then
		if not ECs::'list'( 'polynom' ('rational', vars) ) then
			error("invalid input: %1 expects each list in the first argument to consist of two parts, with the first a list of polynomials in the stated variables.  This is not the case for QFF %2.", procname, i):
		fi:
	else
		error("invalid input: %1 expects each list in the first argument to consist of two parts, with the first representing the equational constraint(s) either a polynomial or a list of polynomials.  This is not the case for QFF %2.", procname, i):
	fi:
od:
dim := nops(vars):
if retcad>dim then
	error("invalid input: %1 expects the optional argument retcad to be a non-negative integer less than or equal to the number of variables specified in the second argument.", procname):
fi:
if output::'Not(identical(list, listwithrep, piecewise, rootof))' then
	error("invalid input: expected output = list, or listwithrep, or piecewise, or rootof, received: output = %1", output)
fi:
# Dealing with undesignated ECs
PHIX := []:
for i from 1 to t do
	phi := PHI[i]:
	ECs := phi[1]:
	if ECs::'list' then
		if nops(ECs)=0 then
			phix := [false, phi[2]]:
		elif nops(ECs)=1 then
			phix := [op(phi[1]), phi[2]]:
		else
			userinfo(4, {'ProjectionCAD'},"the user did not designate an equational constraint in QFF %1.  Hence the QFF Heuristic command is being used:", i):
			#NOTE:  This is not efficient as we are calculating the projection set twice!
			phix := op(Heuristics_TTICADQFF( phi, vars, 'NS', ['vary',80], false, true, [1,1] )):
		fi:
	else
		phix := phi:
	fi:
	PHIX := [op(PHIX), phix]:
od:
TTI_TTICAD( expand(PHIX), vars, LiftAll, output, retcad):
end proc:

#####################################################################

TTICADProjFactors:=proc( PHI::list, vars::list(symbol), $ ) :: set:
local i, t, phi, f, gs, PHIX, phix, ECs:
description "TTICADProjFactors: Compute the projection factors used for TTICAD for a list of QFFs.",
			"Input: PHI, a list of lists phi[i], each of which represent a QFF.  Each phi[i] has two arguments; the first a polynomial or list of polynomials (equational constraint) and the second a list of polynomials (from the other constraints).",
			"Also a list of ordered variables."
			"Output: The set of projection factors.":

t:=nops(PHI):
# Error Checking
for i from 1 to t do
	phi := PHI[i]:
	if not phi::'list' then
		error("invalid input: %1 expects the first argument to be a list of lists (each representing a QFF).", procname):
	fi:
	if nops(phi)<>2 then
		error("invalid input: %1 expects each list in the first argument to consist of two components; but list %2 has %3 components", procname, i, nops(phi)):
	fi:
	ECs:=phi[1]:
	if ECs::'polynom' then
		f := expand(phi[1]):
		if not f::'polynom' ('rational', vars) then
			error("invalid input: %1 expects each list in the first argument to consist of two parts, with the first (the equational constraints) being polynomial in the stated variables.  This is not the case for QFF %2.", procname, i):
		fi:
	elif ECs::'list' then
		if not ECs::'list'( 'polynom' ('rational', vars) ) then
			error("invalid input: %1 expects each list in the first argument to consist of two parts, with the first a list of polynomials in the stated variables.  This is not the case for QFF %2.", procname, i):
		fi:
	else
		error("invalid input: %1 expects each list in the first argument to consist of two parts, with the first representing the equational constraint(s) either a polynomial or a list of polynomials.  This is not the case for QFF %2.", procname, i):
	fi:
	gs := expand(phi[2]):
	if not gs::'list'( 'polynom' ('rational', vars) ) then
		error("invalid input: %1 expects each list in the first argument to consist of two parts, with the second a list of polynomials in the stated variables.  This is not the case for QFF %2.", procname, i):
	fi:
od:
# Dealing with undesignated ECs
PHIX := []:
for i from 1 to t do
	phi := PHI[i]:
	ECs := phi[1]:
	if ECs::'list' then
		if nops(ECs)=0 then
			phix := [false, phi[2]]:
		elif nops(ECs)=1 then
			phix := [op(phi[1]), phi[2]]:
		else
			error("invalid input: %1 expects the equational constraints to have already been designated, but QFF %2 contains %3 constraints.", procname, i, nops(ECs)):
		fi:
	else
		phix := phi:
	fi:
	PHIX := [op(PHIX), phix]:
od:
TTI_TTIPP( expand(PHIX), vars):
end proc:

#####################################################################

TTICADResCADSet:=proc( PHI::list, vars::list(symbol), $ ) :: list:
local i, t, phi, f, gs, PHIX, phix, ECs:
description "TTICADResCADSet: Compute the ResCADSet.",
			"Input: PHI, a list of lists phi[i], each of which represent a QFF.  Each phi[i] has two arguments; the first a polynomial or list of polynomials (equational constraint) and the second a list of polynomials (from the other constraints).",
			"Also a list of ordered variables."
			"Output: The ResCADSet.":

t:=nops(PHI):
for i from 1 to t do
	phi := PHI[i]:
	if not phi::'list' then
		error("invalid input: %1 expects the first argument to be a list of lists (each representing a QFF).", procname):
	fi:
	if nops(phi)<>2 then
		error("invalid input: %1 expects each list in the first argument to consist of two parts; a polynomial (equational constraint) and a list of polynomials (other constraints)", procname):
	fi:
	ECs:=phi[1]:
	if ECs::'polynom' then
		f := expand(phi[1]):
		if not f::'polynom' ('rational', vars) then
			error("invalid input: %1 expects each list in the first argument to consist of two parts, with the first (the equational constraints) being polynomial in the stated variables.  This is not the case for QFF %2.", procname, i):
		fi:
	elif ECs::'list' then
		if not ECs::'list'( 'polynom' ('rational', vars) ) then
			error("invalid input: %1 expects each list in the first argument to consist of two parts, with the first a list of polynomials in the stated variables.  This is not the case for QFF %2.", procname, i):
		fi:
	else
		error("invalid input: %1 expects each list in the first argument to consist of two parts, with the first representing the equational constraint(s) either a polynomial or a list of polynomials.  This is not the case for QFF %2.", procname, i):
	fi:
	gs := expand(phi[2]):
	if not gs::'list'( 'polynom' ('rational', vars) ) then
		error("invalid input: %1 expects each list in the first argument to consist of two parts, with the second a list of polynomials in the stated variables.  This is not the case for QFF %2.", procname, i):
	fi:
od:
# Dealing with undesignated ECs
PHIX := []:
for i from 1 to t do
	phi := PHI[i]:
	ECs := phi[1]:
	if ECs::'list' then
		if nops(ECs)=0 then
			phix := [false, phi[2]]:
		elif nops(ECs)=1 then
			phix := [op(phi[1]), phi[2]]:
		else
			error("invalid input: %1 expects the equational constraints to have already been designated, but QFF %2 contains %3 constraints.", procname, i, nops(ECs)):
		fi:
	else
		phix := phi:
	fi:
	PHIX := [op(PHIX), phix]:
od:
TTI_ResCADSet(PHIX, vars):
end proc:

#####################################################################

TTICADResCAD:=proc( PHI::list, vars::list(symbol), {output::symbol:=list}, $ ) :: list:
local mvar, lvars, i, t, phi, f, gs, ECs, PHIX, phix:
description "TTICADResCAD: Compute a TTICAD for a list of QFFs using the RESCAD method.",
			"Input: PHI, a list of lists phi[i], each of which represent a QFF.  Each phi[i] has two arguments; the first a polynomial or list of polynomials (equational constraint) and the second a list of polynomials (from the other constraints).",
			"Also a list of ordered variables and optionally, an ouput choice."
			"Output: Failure if the reduced projection set is not well oriented.  Otherwise a CAD for PHI which is guarenteed truth table invariant provided that no equational constraint is nullified.":

mvar:=vars[1]:
lvars:=remove(has,vars,mvar):
if nops(vars)>1 then
	WARNING("The output from TTICADResCAD is only guaranteed truth table invariant if no equational constraint is nullified by a point in %1", lvars):
fi:
t:=nops(PHI):
# Error Checking
for i from 1 to t do
	phi := PHI[i]:
	if not phi::'list' then
		error("invalid input: %1 expects the first argument to be a list of lists (each representing a QFF).", procname):
	fi:
	if nops(phi)<>2 then
		error("invalid input: %1 expects each list in the first argument to consist of two parts; a polynomial (equational constraint) and a list of polynomials (other constraints)", procname):
	fi:
	ECs:=phi[1]:
	if ECs::'polynom' then
		f := expand(phi[1]):
		if not f::'polynom' ('rational', vars) then
			error("invalid input: %1 expects each list in the first argument to consist of two parts, with the first (the equational constraints) being polynomial in the stated variables.  This is not the case for QFF %2.", procname, i):
		fi:
	elif ECs::'list' then
		if not ECs::'list'( 'polynom' ('rational', vars) ) then
			error("invalid input: %1 expects each list in the first argument to consist of two parts, with the first a list of polynomials in the stated variables.  This is not the case for QFF %2.", procname, i):
		fi:
	else
		error("invalid input: %1 expects each list in the first argument to consist of two parts, with the first representing the equational constraint(s) either a polynomial or a list of polynomials.  This is not the case for QFF %2.", procname, i):
	fi:
	gs := expand(phi[2]):
	if not gs::'list'( 'polynom' ('rational', vars) ) then
		error("invalid input: %1 expects each list in the first argument to consist of two parts, with the second a list of polynomials in the stated variables.  This is not the case for QFF %2.", procname, i):
	fi:
od:
if output::'Not(identical(list, listwithrep, piecewise, rootof))' then
	error("invalid input: expected output = list, or listwithrep, or piecewise, or rootof, received: output = %1", output)
fi:
# Dealing with undesignated ECs
PHIX := []:
for i from 1 to t do
	phi := PHI[i]:
	ECs := phi[1]:
	if ECs::'list' then
		if nops(ECs)=0 then
			phix := [false, phi[2]]:
		elif nops(ECs)=1 then
			phix := [op(phi[1]), phi[2]]:
		else
			error("invalid input: %1 expects the equational constraints to have already been designated, but QFF %2 contains %3 constraints.", procname, i, nops(ECs)):
		fi:
	else
		phix := phi:
	fi:
	PHIX := [op(PHIX), phix]:
od:
TTI_ResCAD(PHIX, vars, output):
end proc:

#####################################################################

ECCADFormulations := proc( IN::list, $) :: list:
local ecs, necs:
description "ECCADFormulations: Create the list of choices for entry into ECCAD.",
			"Input: Two lists of polynomials, the first assumed equational constraints.",
			"Ouput: A list of possible formulations for ECCAD.  Each list has a polynomial (designated EC) and a list of other constraints.":

if nops(IN)<>2 then
	error("invalid input: %1 expects the first argument to be a list with first entry polynomial and second entry list of polynomials."):
else
	ecs:=IN[1]:
	necs:=IN[2]:
fi:
if not ecs::'list(polynom(rational))' then
	error("invalid input: %1 expects each list in the first argument to contain polynomials over the rationals."):
fi:
if not necs::'list'( 'polynom' ('rational') ) then
	error("invalid input: %1 expects each list in the first argument to contain polynomials over the rationals."):
fi:
Formulations_ECCAD( [expand(ecs), expand(necs)] ):
end proc:

##################################################################### map(X-> [X[1],X[2..nops(X)] ],

TTICADQFFFormulations := proc( IN::list(list(polynom)), {splitting::boolean:=true}, $ ) :: list:
local F, G:
description "TTICADQFFFormulations: Create the list of possible formulations for a TTICAD QFF.",
			"Input: Two lists of polynomials, the first assumed the equational constraints.  Optionally, the choice of whether to consider splitting.",
			"Ouput: The list of all possible formulations for TTICAD, by default including splitting.":

F := IN[1]:
G := IN[2]:
if splitting=false then
	return( map(X->[X],Formulations_ECCAD(F,G)) ):
else
	return( Formulations_QFF( [expand(F), expand(G)] ) ):
fi:
end proc:

#####################################################################

TTICADFormulations := proc( LL::list(list(list(polynom))), {splitting::boolean:=true}, $ ) :: list:
description "TTICADFormulations: Create the list of possible formulations for TTICAD.",
			"Input: A list of lists each representing a QFF.  Each QFF has two lists of polynomials, the first assumed the equational constraints.  Optionally, the choice of whether to consider splitting.",
					"Note: it is assumed that the QFFs come from a disjunctive normal form.  I.e. trivial merging is not possible.",
			"Ouput: The list of all possible formulations for TTICAD, by default including splitting.":

Formulations_TTICAD(LL, splitting):
end proc:

#####################################################################

sotd := proc(IN, $) :: posint:
description "sotd: Calculate the sotd (sum of total degree) of a polynomial or list / set of polynomials.",
			"Input: Either a polynomial of list / set of polynomials.",
			"Ouput: The sotd of the polynomials or sum of the sotds for a list / set of polynomials.":

if IN::'set'('polynom') or IN::'list'('polynom') then
	return( Heuristics_sotdL(IN) ):
elif IN::'polynom' then
	return( Heuristics_sotdP(IN) ):
else
	error("invalid input:  sotd expects its input to be a polynomial, or list/set of polynomials"):
fi:
end proc:

#####################################################################

ndrr := proc(INN, var::symbol, {method::symbol:='unstated', naivedeg::posint:=80}, $) :: posint:
description "ndrr: Calculate the ndrr (number of distinct real roots) of a polynomial or list / set of polynomials with respect to a variable.  Note: if multivariate polynomials are included then they are ignored.",
			"Input: Either a polynomial of list / set of polynomials, and a variable.",
			"Ouput: The ndrr.":

if method::'Not(identical(full, naive, vary, unstated, SFnaive))' then
	error("invalid input: expected method = full, or naive, or vary, received: method = %1", method)
fi:
if not INN::'{polynom, list(polynom), set(polynom)}' then
	error("invalid input:  ndrr expects its input to be a polynomial, or list/set of polynomials"):
fi:
if method='unstated' then
	return( Heuristics_ndrr(INN, var, 'SFnaive', naivedeg, false) ):
else
	return( Heuristics_ndrr(INN, var, method, naivedeg, true) ):
fi:
end proc:

#####################################################################

ECCADHeuristic := proc(IN::list, vars::list(symbol), {heuristic::symbol:='NS', ndrrOptions::list:=['vary',80], SeeAll::boolean:=false, Wratio::list(posint):=[1,1]}, $) :: list(list):
local ecs, necs, ndrrOp:
description "ECHeuristic: Selects formulation(s) for ECCAD using heuristics.",
			"Input: Two lists of polynomials (the first assumed equational constraints) and a variable ordering.  Optionally a heuristic choice (S, N, NS or SN, W) for which the default is NS.  If W is picked then Wratio should also be provided.",
			"Ouput: A single ECCAD formulation or list of the ECCAD formulations suggested by the heuristicselected partitions.":

ecs := IN[1]:
necs := IN[2]:
if nops(IN)<>2 then
	error("invalid input: %1 expects the first argument to be a list with first entry polynomial and second entry list of polynomials."):
else
	ecs:=IN[1]:
	necs:=IN[2]:
fi:
if not ecs::'list(polynom(rational))' then
	error("invalid input: %1 expects each list in the first argument to contain polynomials over the rationals."):
fi:
if not necs::'list'( 'polynom' ('rational') ) then
	error("invalid input: %1 expects each list in the first argument to contain polynomials over the rationals."):
fi:
if heuristic::'Not(identical(N, S, NS, SN, W))' then
	error("expected heuristic = S, or N, or NS, or SN, or W, received: heuristic = %1", heuristic):
fi:
if nops(Wratio)<>2 then
	error("The optional argument Wratio should be a list containing two positive integers."):
fi:
if nops(ndrrOptions)>2 then
	error("The optional argument ndrrOptions should be a list with one or two entries"):
elif ndrrOptions[1]::'Not(identical(full, naive, vary))' then
	error("The optional argument ndrrOptions should be a list with first entry an acceptable ndrr method"):
elif nops(ndrr)=1 then
	ndrrOp:=[ndrrOptions[1], 80]:
elif not ndrrOptions[2]::posint then
		error("The optional argument ndrrOptions should be a list.  If the list contains a second entry then this should be a positive integer indicating the degree that the ndrr method changes."):
else
	ndrrOp:=ndrrOptions:
fi:
Heuristics_ECCAD( ecs, necs, vars, heuristic, ndrrOp, SeeAll, Wratio ):
end proc:

#####################################################################

TTICADQFFHeuristic := proc( IN::list(list(polynom)), vars::list(symbol), {heuristic::symbol:='NS', ndrrOptions::list:=['vary',80], SeeAll::boolean:=false, splitting::boolean:=true, Wratio::list(posint):=[1,1]}, $) :: list(list):
local F, G, ndrrOp:
description "TTICADQFFHeuristic: Selects formulation(s) for a TTICADQFF using heuristics.",
			"Input: Two lists of polynomials (the first assumed equational constraints) and a variable ordering.  Optionally a heuristic choice (S, N, NS or SN) for which the default is NS and the choice to consider splittings (for which the default is true).",
			"Ouput: A single TTICAD formulation or list of the TTICAD formulations suggested by the heuristicselected partitions.":

F := IN[1]:
G := IN[2]:
if heuristic::'Not(identical(N, S, NS, SN, W))' then
	error("expected heuristic = S, or N, or NS, or SN, or W, received: heuristic = %1", heuristic):
fi:
if nops(Wratio)<>2 then
	error("The optional argument Wratio should be a list containing two positive integers."):
fi:
if nops(ndrrOptions)>2 then
	error("The optional argument ndrrOptions should be a list with one or two entries"):
elif ndrrOptions[1]::'Not(identical(full, naive, vary))' then
	error("The optional argument ndrrOptions should be a list with first entry an acceptable ndrr method"):
elif nops(ndrr)=1 then
	ndrrOp:=[ndrrOptions[1], 80]:
elif not ndrrOptions[2]::posint then
		error("The optional argument ndrrOptions should be a list.  If the list contains a second entry then this should be a positive integer indicating the degree that the ndrr method changes."):
else
	ndrrOp:=ndrrOptions:
fi:
Heuristics_TTICADQFF( [expand(F), expand(G)], vars, heuristic, ndrrOp, SeeAll, splitting, Wratio ):
end proc:

#####################################################################

TTICADHeuristic := proc(LL::list(list(list(polynom))), vars::list(symbol), {modular::boolean:=false, heuristic::symbol:='NS', ndrrOptions::list:=['vary',80], SeeAll::boolean:=false, splitting::boolean:=true, Wratio::list(posint):=[1,1]}, $) :: list(list):
local ndrrOp:
description "TTICADHeuristic: Selects formulation(s) for a sequence of QFFs for TTICAD using heuristics.",
			"Input: A list of lists each representing a QFF.  Each QFF has two lists of polynomials, the first assumed the equational constraints.",
					"Optionally a heuristic choice (S, N, NS or SN) for which the default is NS and the choice to consider splittings (for which the default is true).",
					"Note: it is assumed that the QFFs come from a disjunctive normal form.  I.e. trivial merging is not possible.",
			"Ouput: A single TTICAD formulation or list of the TTICAD formulations suggested by the heuristicselected partitions.":

if heuristic::'Not(identical(N, S, NS, SN, W))' then
	error("expected heuristic = S, or N, or NS, or SN, or W, received: heuristic = %1", heuristic):
fi:
if nops(Wratio)<>2 then
	error("The optional argument Wratio should be a list containing two positive integers."):
fi:
if modular=true and SeeAll=true then
	WARNING("SeeAll=true is specified but this is repressed when using the modular method.  Suggest running TTICADQFFHeuristic on the individual caluses."):
fi:
if nops(ndrrOptions)>2 then
	error("The optional argument ndrrOptions should be a list with one or two entries"):
elif ndrrOptions[1]::'Not(identical(full, naive, vary))' then
	error("The optional argument ndrrOptions should be a list with first entry an acceptable ndrr method"):
elif nops(ndrr)=1 then
	ndrrOp:=[ndrrOptions[1], 80]:
elif not ndrrOptions[2]::posint then
		error("The optional argument ndrrOptions should be a list.  If the list contains a second entry then this should be a positive integer indicating the degree that the ndrr method changes."):
else
	ndrrOp:=ndrrOptions:
fi:
Heuristics_TTICAD( LL, vars, modular, heuristic, ndrrOp, SeeAll, splitting, Wratio ):
end proc:

#####################################################################

VariableOrderings := proc( vars::list, $ ) :: list:
description "VariableOrderings: Finds all the possible acceptable variable orderings.",
			"Input: Either a list of variables (if there are no restrictions) or a list of lists of variables (if the variables must be in set blocks, e.g. for quantifier elimination.",
			"Ouput: A list of all posible acceptable variable orderings (each a list of variables).":

if vars::'list'(symbol) then
	return( Formulation_VarOrdFree(vars) ) :
elif vars::'list'('list'('symbol')) then
	return( Formulation_VarOrdRestricted(vars) ) :
else
	error("The input should either be a list of variables or a list of lists of variables.  Variables should be of type symbol."):
fi:
end proc:

#####################################################################

VariableOrderingHeuristic := proc( VarsList::list, IN, {algorithm::symbol:='CADFull', greedy::boolean:=false, heuristic::symbol:='NS', method::symbol:='McCallum', ndrrOptions::list:=['vary',80], SeeAll::boolean:=false, Wratio::list(posint):=[1,1] }, $ ) :: list:
local vars, i, INN, f, gs, t, phi, alg, ndrrOp:
description "VariableOrderingHeuristic: Pick a variable ordering based on a heuristic.",
			"Input: The input for a CAD algorithm, a variable list (possibly in blocks) and an algorithm selection.",
			"Ouput: Suggested variable ordering(s).":

if VarsList::'list(symbol)' then
	vars:=VarsList:
elif VarsList::'list(list(symbol))' then
	vars:=map(X->op(X),VarsList):
else
	error("invalid input: %1 expects the first argument to be either a list of symbols, or list of lists of symbols.", procname):
fi:
if algorithm=CADFull then
	if IN::'Not({list,set})' then
		error("invalid input: algorithm is set to %1 but the first argument is not admissible as a first argument for CADProjection", algorithm):
	fi:
	INN := convert(IN, 'set');
	if INN::'set(list(polynom))' then
		error("invalid input: %1 expects the first argument to be a list or set containing only polynomials.", algorithm):
	fi:
	INN := expand(INN):
	if map(X->coeffs(X, vars), INN)::'Not(set(rational))' then
		error("invalid input: %1 expects the polynomials in the first argument to be defined over the ring of variables in the second argument, with coefficients in the rationals", algorithm):
	fi:
	if method::'Not(identical(Collins, McCallum))' then
		error("invalid input: expected method = Collins, or McCallum, received: method = %1", method)
	fi:
	alg:='algorithm_CADFull':
elif algorithm=ECCAD then
	if IN::'Not(list)' then
		error("invalid input: algorithm is set to %1 but the first argument is not admissible as a first argument for ECCADProjFactors", algorithm):
	fi:
	if nops(IN)<>2 then
		error("invalid input: %1 expects the first argument to be a list with first entry polynomial and second entry list of polynomials.", algorithm):
	else
		f:=IN[1]:
		gs:=IN[2]:
	fi:
	if not f::'polynom' ('rational', vars) then
		error("invalid input: %1 expects each list in the first argument to be a polynomial in the stated variables over the rationals.", algorithm):
	fi:
	if not gs::'list'( 'polynom' ('rational', vars) ) then
		error("invalid input: %1 expects the second argument to be a list of polynomials in the stated variables over the rationals.", algorithm):
	fi:
	alg:='algorithm_ECCAD':
	INN := IN:
elif algorithm=TTICAD then
	if IN::'Not(list)' then
		error("invalid input: algorithm is set to %1 but the first argument is not admissible as a first argument for TTICADProjFactors", algorithm):
	fi:
	t:=nops(IN):
	for i from 1 to t do
		phi := IN[i]:
		if not phi::'list' then
			error("invalid input: %1 expects the first argument to be a list of lists (each representing a QFF).", algorithm):
		fi:
		if nops(phi)<>2 then
			error("invalid input: %1 expects each list in the first argument to consist of two components; but list %2 has %3 components", algorithm, i, nops(phi)):
		fi:
		f := expand(phi[1]):
		gs := expand(phi[2]):
		if not f::'polynom' ('rational', vars) then
			error("invalid input: %1 expects each list in the first argument to consist of two parts, with the first a polynomial in the stated variables.  This is not the case for QFF %2.", algorithm, i):
		fi:
		if not gs::'list'( 'polynom' ('rational', vars) ) then
			error("invalid input: %1 expects each list in the first argument to consist of two parts, with the second a list of polynomials in the stated variables.  This is not the case for QFF %2.", algorithm, i):
		fi:
	od:
	alg:='algorithm_TTICAD':
	INN := IN:
else
	error("expected algorithm = CADFull, or ECCAD, or TTICAD, received: algorithm = %1", algorithm):
fi:
if nops(Wratio)<>2 then
	error("The optional argument Wratio should be a list containing two positive integers."):
fi:
if greedy=true and SeeAll=true then
	WARNING("SeeAll=true is specified but this is repressed when using the greedy method.  Suggest running TTICADQFFHeuristic on the individual caluses."):
fi:
if nops(ndrrOptions)>2 then
	error("The optional argument ndrrOptions should be a list with one or two entries"):
elif ndrrOptions[1]::'Not(identical(full, naive, vary))' then
	error("The optional argument ndrrOptions should be a list with first entry an acceptable ndrr method"):
elif nops(ndrr)=1 then
	ndrrOp:=[ndrrOptions[1], 80]:
elif not ndrrOptions[2]::posint then
		error("The optional argument ndrrOptions should be a list.  If the list contains a second entry then this should be a positive integer indicating the degree that the ndrr method changes."):
else
	ndrrOp:=ndrrOptions:
fi:
Heuristics_VariableOrdering( VarsList, INN, alg, greedy, heuristic, method, ndrrOp, SeeAll, Wratio ):
end proc:

#####################################################################
#####################################################################
#####################################################################
### LOCAL COMMANDS
#####################################################################
#####################################################################
#####################################################################

#####################################################################
### Section 2: Miscellaneous useful things
#####################################################################

PCAD_SortEqsRC := proc( eqs, R, $ ) :: 'list':
local vars,dim,i,E:
description "PCAD_SortEqsRC: Sort a list of equations into ascending main variable. Error if main variables are not distinct.":

vars:=ListTools:-Reverse(R['variables']):
dim:=nops(vars):
E:=table():
for i from 1 to nops(vars) do
	E[i]:=select(X->RegularChains:-MainVariable(X,R)=vars[i], eqs):
	if nops(E[i])>1 then
		error("the equations inputted do not have distinct main variable. This is an unexpected error message - please report to M.England@bath.ac.uk"):
	fi:
od:
[seq( op(E[i]), i=1..dim)]
end proc:

#####################################################################

PCAD_LdCf := proc( poly::polynom, var::symbol, $ ) :: 'polynom':
description "PCAD_LdCf: Computes the leading coefficient of a polynomial with respect to a variable":

coeff( poly, var, degree(poly,var)):
end proc:

#####################################################################

PCAD_MakeMonic:=proc( poly::polynom, RorVar, $ ) :: polynom:
local i, vars, R, pol:
description "PCAD_MakeMonic: Divide polynomial by its leading coefficient (in the base field).":

if RorVar::'table' and RorVar['type']='polynomial_ring' then
	vars:=RorVar['variables']:
	R:=RorVar:
elif RorVar::'list'('symbol') then
	vars:=RorVar:
	R:=RegularChains:-PolynomialRing(vars):
else
	error("the second argument should either be a polynomial ring or a list of variables. This is an unexpected error message - please report to M.England@bath.ac.uk"):
fi:
if indets(poly,'name') minus convert(vars,set) <> {} then
	error("the polynomial has variables other than those specified. This is an unexpected error message - please report to M.England@bath.ac.uk"):
fi:
if poly::'constant' then
	return(poly)
else
	pol:=poly:
	i:=1:
fi:
while i<nops(vars)+1 do
	pol:=RegularChains:-Initial(pol,R):
	if pol::'constant' then
		return( expand(poly/pol) ):
	else
		i:=i+1:
	fi:
od:
error("leading coefficient could not be identified. This is an unexpected error message - please report to M.England@bath.ac.uk"):
end proc:

#####################################################################

PCAD_MakeInteger:=proc( poly::polynom, $ ) :: polynom:
local bottom:
description "PCAD_MakeInteger: If the denominator is a constant then multiply up.":

bottom:=denom(poly):
if bottom::'constant' then
	return( expand(bottom*poly) ):
else
	return(poly)
fi:
end proc:

#####################################################################

PCAD_RemoveConstantMultiples:=proc( pols, $ ) :: set;
local polys, vars, RET:
description "PCAD_RemoveConstantMultiples: Removes polynomials from a set that are constant multiples of polynomials previously appearing in the set.":

if pols::list then
	polys:=convert(pols,'set'):
elif pols::set then
	polys:=pols:
else
	error("first input should be a list or set of polynomials. This is an unexpected error message - please report to M.England@bath.ac.uk"):
fi:
if polys={} then
	return({}):
fi:
vars:=convert(indets(polys,'name'),list):
RET := map(X->PCAD_MakeMonic(X, vars), polys):
RET := map(X->PCAD_MakeInteger(X), RET):
end proc:

#####################################################################

PCAD_SFBasis:=proc( pols, var::symbol, $ ) :: set;
local polys, i, j, decomp, head, body, RET:
description "PCAD_SFBasis: Construct a square free basis of a set of polynomials, with respect to a variable":

if pols::list then
	polys:=convert(pols,set):
elif pols::set
	then polys:=pols:
else
	error("first input should be a list or set of polynomials. This is an unexpected error message - please report to M.England@bath.ac.uk"):
fi:
RET:={}:
for i from 1 to nops(polys) do
	decomp:=sqrfree(op(i,polys),var):
	head:=op(1,decomp):
	body:=op(2,decomp):
	if type(head,constant)=false then
		RET:={op(RET),op(1,decomp)}:
	fi:
	for j from 1 to nops(body) do
		RET:={op(RET), op(1,op(j,body))}:
	od:
od:
return(RET):
end proc:

#####################################################################

PCAD_ContSet:=proc( pols, var::symbol, $ ) :: set:
local S, i, polys:
description "PCAD_ContSet: Compute the set of contents (with respect to a variable) from a set of polynomials":

if pols::list then
	polys:=convert(pols,set):
elif pols::set then
	polys:=pols:
else
	error("first input should be a list or set of polynomials. This is an unexpected error message - please report to M.England@bath.ac.uk"):
fi:
S:={seq( content(op(i,polys),var), i=1..nops(polys))}:
remove(X->X::'constant',S):
end proc:

#####################################################################

PCAD_PrimSet:=proc( pols, var::symbol, $ ) :: set:
local S, i, polys:
description "PCAD_PrimSet: Compute the set of primitive parts (with respect to a variable) from a set of polynomials":

if pols::list then
	polys:=convert(pols,set):
elif pols::set then
	polys:=pols:
else
	error("first input should be a list or set of polynomials. This is an unexpected error message - please report to M.England@bath.ac.uk"):
fi:
S:={seq( primpart(op(i,polys),var), i=1..nops(polys))}:
remove(X->X::'constant',S):
end proc:

#####################################################################

PCAD_SetGCD:=proc(inpols, $) :: polynom:
local S, Div, i:
description "PCAD_SetGCD: Compute the gcd of a set of polynomials.":

if inpols::list then
	S:=convert(inpols,set):
elif inpols::set then
	S:=inpols:
else
	error("first input should be a list or set of polynomials. This is an unexpected error message - please report to M.England@bath.ac.uk"):
fi:
if nops(S)=1 then
	Div:=op(S):
else
	Div:=gcd(op(1,S),op(2,S)):
fi:
for i from 3 to nops(S) do
	Div:=gcd(Div,op(i,S)):
od: Div:
end proc:

#####################################################################

PCAD_SetFactors:=proc(inpols, $) :: set:
local S, F, i;
description "CADdebug_SetFactors: Compute the factors of a set of polynomials.":

if inpols::list then
	S:=convert(inpols,set):
elif inpols::set then
	S:=inpols:
else
	error("first input should be a list or set of polynomials. This is an unexpected error message - please report to M.England@bath.ac.uk"):
fi:
F:=table():
for i from 1 to nops(S) do
	F[i]:=op(map(X->op(1,X),op(2,factors(op(i,S))))):
od:
F:={entries(F,'nolist')}:
end proc:

#####################################################################

PCAD_DoesIntervalIntersect:=proc( c1::list, c2::list, $ ) :: boolean:
local L1, L2, R1, R2, l2, r1:
description "PCAD_DoesIntervalIntersect: Check whether two intervals intersect.":

if nops(c1)<>2 or nops(c2)<>2 then
	error("incorrect input. This is an unexpected error message - please report to M.England@bath.ac.uk"):
fi:
L1:=op(1,c1):
R1:=op(2,c1):
L2:=op(1,c2):
R2:=op(2,c2):
if L1::rational and L2::rational and R1::rational and R2::rational then
	if L2<L1 then
		l2:=L1: r1:=R2:
	else
		l2:=L2: r1:=R1:
	fi:
else
	error("incorrect input. This is an unexpected error message - please report to M.England@bath.ac.uk"):
fi:
if r1<l2 then
	return(false):
else
	return(true):
fi:
end proc:

#####################################################################
###  Section 3: Collins Projection
#####################################################################

PCAD_CCADmat:=proc(S::list,var::symbol, $) :: 'Matrix':
local i, j, k, ell, M;
description "PCAD_CCADmat: Compute the matrix associated with a set of polynomials (see Jirstrand page 15).":

if S=[] then
	return(Matrix([0]));
fi:
if {op(map(type,S,polynom))}<>{true} then
	error("The first input should be a list of polynomials. This is an unexpected error message - please report to M.England@bath.ac.uk"):
fi:
k:=nops(S):
ell:=1+max(map(degree,S,var)):
M:=Matrix(k,ell):
for i from 1 to k do
	for j from 1 to ell do
		M[i,j]:=coeff(S[i],var,ell-j):
	od:
od:
M:
end proc:

#####################################################################

PCAD_CCADpsc:=proc(F::polynom, G::polynom, K::nonnegint, var::symbol, $) :: 'polynom':
local f, g, m, n, i, k, ell, M, count, S;
description "PCAD_CCADpsc: Compute the kth principal subresultant coefficient of two polynomials with respect to a variable.":

if degree(F,var)<degree(G,var) then
	f:=G: g:=F:
else
	f:=F: g:=G:
fi:
m:=degree(f,var):
n:=degree(g,var):
if K>n+1 then
	error("third input must be an integer less than minimum degree of the input polynomials. This is an unexpected error message - please report to M.England@bath.ac.uk"):
fi:
S:=table():
count:=1:
for i from (n-K-1) by -1 to 0 do
	S[count]:=expand(var^i*f):
	count:=count+1:
od:
for i from (m-K-1) by -1 to 0 do
	S[count]:=expand(var^i*g):
	count:=count+1:
od:
S:=[entries(S,'nolist')]:
M:=PCAD_CCADmat(S,var):
k:=op(1,[LinearAlgebra:-Dimensions(M)]);
ell:=op(2,[LinearAlgebra:-Dimensions(M)]):
if k=ell then
	return(LinearAlgebra:-Determinant(M)):
else
	return( LinearAlgebra:-Determinant( LinearAlgebra:-DeleteColumn(M,[op({seq(k..ell)} minus {ell-K})]) )):
fi:
end proc:

#####################################################################

PCAD_CCADpscset:=proc(f::polynom, g::polynom, var::symbol, $) :: 'set':
local n, j:
description "PCAD_CCADpscset: Compute the set of principal subresultant coefficients of two polynomials with respect to a variable.":

n:=min(degree(f,var),degree(g,var)):
{seq( PCAD_CCADpsc(f,g,j,var), j=0..n)} minus {0}:
end proc:

#####################################################################

PCAD_CCADreductum:=proc(poly::polynom, var::symbol, deg::nonnegint, $) :: 'polynom':
description "PCAD_CCADreductum: Compute the reductum of a polynomial.":

if deg>degree(poly,var) then
	error("reductum degree is bigger than polynomial degree. This is an unexpected error message - please report to M.England@bath.ac.uk"):
fi:
convert(series( poly,var,deg+1 ),polynom):
end proc:

#####################################################################

PCAD_CCADreductumset:=proc(poly::polynom, var::symbol, lvars::set, $) :: 'set':
local i, j, deg, rlist, cflist, rset, count, fin, tvar, res, crw:
description "PCAD_CCADreductumset: Compute the reductum set of a polynomial.  Note that this is the simplified reductum set as required for Collins projection.":

[var,op(lvars)]:
deg:=degree(poly,var):
cflist:=ListTools:-Reverse( [seq( coeff(poly,var,i), i=0..deg)] ):
rlist:=ListTools:-Reverse( [seq( PCAD_CCADreductum(poly,var,j), j=0..deg)] ):
crw:=op(1,cflist):
fin:=false:
if crw::'constant' or nops(lvars)=1 then
	rset:=rlist[1..1]:
	fin:=true:
fi:
count:=2:
while fin=false do
	if count>=nops(cflist) then
		fin:=true:
		rset:=rlist:
	else
		tvar:=indets(crw,'name')[1]:
		res:=resultant(crw, op(count,cflist), tvar):
		if res=0 then
			crw:=crw:
			count:=count+1:
		elif res::'constant' then
			rset:=[op(1..count-1,rlist)]:
			fin:=true:
		else
			crw:=res:
			count:=count+1:
		fi:
	fi:
od:
rset:={op(rset)} minus {0}:
end proc:

#####################################################################

PCAD_CCADProj1:=proc( polyset::set, var::symbol, lvars::list, $ ) :: 'set':
local i, entry, redset, S, count:
description "PCAD_CCADProj1: Compute the first part of the Collins projection operator.":

if map(X->type(X,polynom),polyset)<>{true} then
	error("input should be set of polynomials. This is an unexpected error message - please report to M.England@bath.ac.uk");
fi:
S:=table():
count:=1:
for i from 1 to nops(polyset) do
	redset:=PCAD_CCADreductumset(op(i,polyset), var, convert(lvars,'set')):
	for entry in redset do
		S[count]:=PCAD_LdCf(entry,var): count:=count+1:
		S[count]:=op(PCAD_CCADpscset(entry,diff(entry,var),var)): count:=count+1:
	od:
od:
S:={entries(S,'nolist')}:
S:=remove(X->X::'constant',S):
S:=PCAD_RemoveConstantMultiples(S):
end proc:

#####################################################################

PCAD_CCADProj2:=proc( polyset::set, var::symbol, lvars::list, $ ) :: 'set':
local i, j, entry, ent, redset1, redset2, S, count:
description "PCAD_CCADProj2: The second part of the Collins projection operator.":

if map(X->type(X,polynom),polyset)<>{true} then
	error("input should be set of polynomials. This is an unexpected error message - please report to M.England@bath.ac.uk");
fi:
if nops(polyset)=1 then
	return({}):
fi:
S:=table(): count:=1:
for i from 1 to nops(polyset) do
	redset1:=PCAD_CCADreductumset(op(i,polyset),var, convert(lvars,'set') ):
	for j from i+1 to nops(polyset) do
		redset2:=PCAD_CCADreductumset(op(j,polyset),var, convert(lvars,'set')):
		for entry in redset1 do
			for ent in redset2 do
				S[count]:=op(PCAD_CCADpscset(entry,ent,var)):
				count:=count+1:
			od:
		od:
	od:
od:
S:={entries(S,'nolist')}:
S:=remove(X->X::'constant',S):
S:=PCAD_RemoveConstantMultiples(S):
end proc:

#####################################################################

PCAD_CCADProj:=proc( polyset::set, var::symbol, lvars::list, {SF::boolean:=true}, $ ) :: 'set':
local pset, S1, S2, S:
description "PCAD_CCADProj: The full Collins projection operator.":

userinfo(3,{'PCAD_CCADProj', 'CADProjection'},"now using Collins projection with respect to", var):
if SF=true then
	pset:=PCAD_SFBasis(polyset,var):
	userinfo(3,{'PCAD_CCADProj', 'CADProjection'},"square free basis consisting of ", nops(pset), "polynomials"):
else
	pset:=polyset:
	userinfo(3,{'PCAD_CCADProj', 'CADProjection'},"set of ", nops(pset), "polynomials"):
fi:
userinfo(4,{'PCAD_CCADProj', 'CADProjection'},"the polynomials are", pset):
S1:=PCAD_CCADProj1(pset,var,lvars):
userinfo(3,{'PCAD_CCADProj', 'CADProjection'},"first operator added ", nops(S1)):
S2:=PCAD_CCADProj2(pset,var,lvars):
userinfo(3,{'PCAD_CCADProj', 'CADProjection'},"second operator added ", nops(S2)):
S:=S1 union S2:
S:=PCAD_RemoveConstantMultiples(S):
end proc:

#####################################################################

PCAD_CCADProjPolys:=proc( polyset::set, vars::list, {SF::boolean:=true}, $ ) :: 'set':
local i, pset, ret, cont, dim:
description "PCAD_CCADProjPolys: Compute the set of projection polynomials for a given set of polynomials using the Collins projection operator and with respect to a list of variables in descending order.":

dim:=nops(vars):
pset:=table():
pset[0] := PCAD_PrimSet(polyset,vars[1]):
pset[0] := PCAD_SFBasis(pset[0], vars[1]):
pset[0] := PCAD_SetFactors(pset[0]):
cont:=PCAD_ContSet(polyset,vars[1]):
for i from 1 to dim-1 do
	pset[i]:=PCAD_CCADProj( pset[i-1], vars[i], vars[i+1..dim] ) union cont:
	cont:=PCAD_ContSet(pset[i],vars[i+1]):
	pset[i]:=PCAD_PrimSet(pset[i], vars[i+1]):
	if i=nops(vars)-1 then
		if SF=true then
			pset[i]:=PCAD_SFBasis(pset[i],vars[i+1]):
		fi:
	else
		pset[i]:=PCAD_SFBasis(pset[i],vars[i+1]):
	fi:
	pset[i]:=PCAD_SetFactors(pset[i]):
od:
ret:=map(X->op(X),[entries(pset,'nolist')]):
ret:=PCAD_RemoveConstantMultiples(ret):
end proc:

#####################################################################
### Section 4: McCallum Projection
#####################################################################

PCAD_McCallumProj:=proc( polys::set, var::symbol, lvars::list, $) :: set:
local Polys, cont, Pol, clist, Pset1, Pset2, Pset, i, j:
description "PCAD_McCallumProj: The McCallum projection operator.":

Polys:=PCAD_PrimSet(polys,var):
cont:=PCAD_ContSet(polys,var);
Polys:=PCAD_SFBasis(Polys,var):
userinfo(3,{'PCAD_McCallumProj', 'CADProjection'},"now using McCallum projection with respect to", var):
userinfo(3,{'PCAD_McCallumProj', 'CADProjection'},"square free basis of primitive part consisting of ", nops(Polys), "polynomials"):
userinfo(4,{'PCAD_McCallumProj', 'CADProjection'},"the polynomials are", Polys):
userinfo(4,{'PCAD_McCallumProj', 'CADProjection'},"content ", cont, " was added to output."):
Pset1:=table():
for i from 1 to nops(Polys) do
	Pol:=op(i,Polys):
	clist:=PCAD_McCcoeffset(Pol,var, convert(lvars,'set')):
	Pset1[i]:=discrim(Pol,var),op(clist):
	userinfo(4,{'PCAD_McCallumProj', 'CADProjection'},"added discriminant of polynomial", i, "and first", nops(clist), "coeffs"):
od:
Pset2:=table():
for i from 1 to nops(Polys) do
	for j from i+1 to nops(Polys) do
		Pset2[i,j]:=resultant( op(i,Polys), op(j,Polys), var):
		userinfo(4,{'PCAD_McCallumProj', 'CADProjection'},"added resultant of polynomials ", i, "and ", j):
	od:
od:
Pset:={op(cont), entries(Pset1,'nolist'), entries(Pset2,'nolist')}:
Pset:=remove(X->type(X,constant),Pset):
userinfo(3,{'PCAD_McCallumProj', 'CADProjection'}, nops(Pset), "non constant projection polynomials found"):
Pset:=PCAD_RemoveConstantMultiples(Pset):
userinfo(3,{'PCAD_McCallumProj', 'CADProjection'},"removed constants and constant multiples to leave", nops(Pset)):
Pset:
end proc:

#####################################################################

PCAD_McCcoeffset:=proc( poly::polynom, var::symbol, lvars::set, $ ) :: list:
local i, crw, deg, cflist, tvar, res:
description "PCAD_McCcoeffset: Compute the set of coefficients required by the McCallum projection operator.":

deg:=degree(poly,var):
cflist:=ListTools:-Reverse( [seq( coeff(poly,var,i), i=0..deg)] ):
crw:=op(1,cflist):
if crw::'constant' then
	return([]):
elif nops(lvars)=1 then
	return([crw]):
fi:
for i from 2 to nops(cflist) do
	tvar:=indets(crw,'name')[1]:
	res:=resultant(crw,op(i,cflist), tvar):
	if res=0 then
		crw:=crw:
	elif res::'constant' then
		return([op(1..i,cflist)]):
	else
		crw:=res:
	fi:
od:
cflist:
end proc:

#####################################################################

PCAD_McCProjPolys:=proc( polyset::set, vars::list, {SF::boolean:=true}, $ )::'set':
local i, pset, ret, cont, dim:
description "PCAD_McCProjPolys: Compute the set of projection polynomials for a given set of polynomials using the McCallum projection operator and with respect to a list of variables in descending order.":

dim:=nops(vars):
pset:=table():
pset[0] := PCAD_PrimSet(polyset,vars[1]):
pset[0] := PCAD_SFBasis(pset[0], vars[1]):
pset[0] := PCAD_SetFactors(pset[0]):
cont:=PCAD_ContSet(polyset,vars[1]):
for i from 1 to dim-1 do
	pset[i]:=PCAD_McCallumProj( pset[i-1], vars[i], vars[i+1..dim]  ) union cont:
	cont:=PCAD_ContSet(pset[i],vars[i+1]):
	pset[i]:=PCAD_PrimSet(pset[i], vars[i+1]):
	if i=nops(vars)-1 then
		if SF=true then
			pset[i]:=PCAD_SFBasis(pset[i],vars[i+1]):
		fi:
	else
		pset[i]:=PCAD_SFBasis(pset[i],vars[i+1]):
	fi:
	pset[i]:=PCAD_SetFactors(pset[i]):
od:
ret:=map(X->op(X),[entries(pset,'nolist')]):
ret:=PCAD_RemoveConstantMultiples(ret):
end proc:

#####################################################################
### Section 5: Lifting
#####################################################################

PCAD_IsCellZeroDim:=proc( cellindex::list, $ ) :: boolean:
local alleven:
description "PCAD_IsCellZeroDim: Check to see if a cell is zero dimensional by examining the index.":

alleven:=convert(map(X->is(X::'even'),cellindex),set):
if alleven={true} then
	return(true):
elif alleven={false} or alleven={false,true} then
	return(false):
else
	error("could not determine parity. This is an unexpected error message - please report to M.England@bath.ac.uk"):
fi:
end proc:

#####################################################################

PCAD_IsCellFullDim:=proc( cellindex::list, $ ) :: boolean:
local allodd:
description "PCAD_IsCellFullDim: Check to see if a cell is of full dimension by examining the index.":

allodd:=convert(map(X->is(X::'odd'),cellindex),set):
if allodd={true} then
	return(true):
elif allodd={false} or allodd={false,true} then
	return(false):
else
	error("could not determine parity. This is an unexpected error message - please report to M.England@bath.ac.uk"):
fi:
end proc:

#####################################################################

PCAD_MinimalDelineatingPolynomial:=proc(f::polynom,alpha::list, R, $) :: polynom:
local k, j, vars, mdeg, t, Dp, S, vchoice:
description "PCAD_MinimalDelineatingPolynomial: Find the minimal delineating polynomial following Brown's 2005 technical note.":

vars:=R['variables']:
if RegularChains:-MainVariable(f,R)<>vars[1] then
	error("incorrect polynomial ring in input. This is an unexpected error message - please report to M.England@bath.ac.uk"):
fi:
mdeg:=degree(f,vars):
t:=0:
k:=1:
while t=0 do
	if k>mdeg then
		error("the minimal delineating polynomial was not found. This is an unexpected error message - please report to M.England@bath.ac.uk"):
	fi:
	vchoice:=map(X -> seq(X+0*j,j=1..k),vars):
	Dp:=combinat:-choose(vchoice,k):
	Dp:=map(X->diff(f,X), Dp):
	S:=remove(X->is(subs(alpha,X)=0)=true, Dp):
	if S<>[] then
		t:=k:
	fi:
	k:=k+1:
od:
PCAD_SetGCD(S):
end proc:

#####################################################################

PCAD_MakeCoprimeOverRC:=proc( rc::table, pset::list, R, $ ) :: 'list':
local i, j, out, ret, new, RSs, rs, tmprc, mvar, eqs, inits, Hinits, RCeqs:
description "PCAD_MakeCoprimeOverRC: Given a regular chain and a set of polynomials, produce a set of polynomials with equivalent zeros which are coprime over the chain.":

if rc[type]<>'regular_chain' then
	error("first input should be regular chain. This is an unexpected error message - please report to M.England@bath.ac.uk"):
fi:
if pset=[] then
	return(pset):
fi:
map(X->type(X,'polynom'),pset):
if {op(%)}<>{true} then
	error("second input should be a list of polynomials. This is an unexpected error message - please report to M.England@bath.ac.uk"):
fi:
if R[type]<>'polynomial_ring' then
	error("third input should be a polynomial ring. This is an unexpected error message - please report to M.England@bath.ac.uk"):
fi:
mvar:=R['variables'][1]:
RCeqs:=convert(map(X->PCAD_MakeMonic(X,R), RegularChains:-Equations(rc,R)),'set'):
ret:=[]:
for i from 1 to nops(pset) do
	out:=RegularChains:-Triangularize( [op(i,pset)],ret,rc,R ):
#	We repeatedly look for the solutions to the equations, such that (a) the base rc is satisfied and (b) the equations in ret (from previous steps) are not zero.
	if out::table and out[type]='constructible_set' then
		RSs:=out['list_regular_system']:
		for j from 1 to nops(RSs) do
			rs:=op(j,RSs):
			if rs[type]<>'regular_system' then
				error("output from Triangularize was not as expected. This is an unexpected error message - please report to M.England@bath.ac.uk"):
			fi:
#			if rs['inequations']<>[] then
#				error("inequations in the decomposition - need to fix"):
#				This only occurs when rc has dim<nops(vars)-1 and this condition boils down onto the "free" variable(s).
#				However, these variables are really fixed into ranges where this is true or false.
#			fi:
			eqs:=map(X->PCAD_MakeMonic(X,R), RegularChains:-Equations(rs['regular_system_chain'],R)):
			inits:=convert(map(X->PCAD_MakeMonic(RegularChains:-Initial(X,R),R), ret),'set'):
			Hinits:=select(X->has(inits,X)=true, eqs):
			Hinits:=select(X->has(RCeqs,X)=false, Hinits):
			if Hinits=[] then
				new:=eqs[1]:
				if RegularChains:-MainVariable(new,R)=mvar then
					ret:=[op(ret),new]:
				fi:
#			else this is the special case where the initial of one of the proj poly is forced zero, but we are not in the cell where that happens.  Hence we ignore this case.
			fi:
		od:
	elif out::list then
		for j from 1 to nops(out) do
			tmprc:=op(j,out):
			if tmprc[type]<>'regular_chain' then
				error("output from Triangularize was not as expected. This is an unexpected error message - please report to M.England@bath.ac.uk"):
			fi:
			new:=RegularChains:-Equations(tmprc,R)[1]:
			if RegularChains:-MainVariable(new,R)=mvar then
				ret:=[op(ret),new]:
			fi:
		od:
	else error("output from Triangularize was not as expected. This is an unexpected error message - please report to M.England@bath.ac.uk"):
	fi:
od:
return( convert(convert(map(X->PCAD_MakeInteger(X),ret),set),list) ):
end proc:

#####################################################################

PCAD_WhichRCInSplitHasSolutionAtSP:=proc( LRCs::list, Cube::list, R, $) :: posint:
local i, dim, lowerRCeqs, lowerRC, lowerR, lowerCube, MainPolys, out, Index, tRoots, answer, switch:
description "PCAD_WhichRCInSplitHasSolutionAtSP: Given a list of regular chains and an isolating cube, identify which entry in the list has a zero in the cube.":
if {op(map(X->X[type],LRCs))}<>{'regular_chain'} then
	error("first input should be a list of regular chains. This is an unexpected error message - please report to M.England@bath.ac.uk"):
fi:
if R[type]<>'polynomial_ring' then
	error("third input should be a polynomial ring. This is an unexpected error message - please report to M.England@bath.ac.uk"):
fi:
if kernelopts('opaquemodules')=true then
	switch:=0:
	kernelopts('opaquemodules'=false):
fi:
lowerRCeqs:=map(X -> PCAD_SortEqsRC(RegularChains:-Equations(X,R),R)[1..-2], LRCs):
lowerRCeqs:=map(X -> map(Y -> PCAD_MakeMonic(Y,R), X), lowerRCeqs):
if nops(convert(lowerRCeqs,set))<>1 then
	error("the regular chains inputted differ at a lower level. This is an unexpected error message - please report to M.England@bath.ac.uk"):
else
	lowerRCeqs:=op(1,lowerRCeqs):
fi:
dim:=nops(R['variables']):
if dim=1 then
	lowerRC:=RegularChains:-ChainTools:-Empty(R):
	lowerCube:=[]:
	MainPolys:=map(X->RegularChains:-Equations(X,R)[1], LRCs):
	out:=RegularChains:-TRDisolate_real_zeros( [lowerRC,lowerCube], MainPolys, R ):
else
	lowerR:=RegularChains:-PolynomialRing( R['variables'][2..-1] ): Display(%,R);
	lowerRC:=RegularChains:-ChainTools:-Chain( lowerRCeqs, RegularChains:-ChainTools:-Empty(lowerR), lowerR):
	lowerCube:=Cube[1..-2]:
	MainPolys:=map(X->RegularChains:-Equations(X,R)[1], LRCs):
	out:=RegularChains:-TRDisolate_real_zeros( [lowerRC,lowerCube], MainPolys, R ):
fi:
Index:=op(2,out):
tRoots:=map(X->op(-1,X),op(1,out)):
tRoots:=map(X->PCAD_DoesIntervalIntersect(X,Cube[-1]), tRoots):
if nops(select(X->X=true,tRoots))<>1 then
	error("the isolating cubes overlap. This is an unexpected error message - please report to M.England@bath.ac.uk"):
fi:
for i from 1 to nops(tRoots) do
	if op(i,tRoots)=true then
		answer:=Index[i]:
	fi:
od:
if switch=0 then
	kernelopts('opaquemodules'=true):
fi:
return(answer):
end proc:

#####################################################################

PCAD_MakeSquareFreeOverRC:=proc( rc::table, extraeqs::list, SPcube::list, pset::list, R, $ ) :: list:
local i, j, ret, out, newpol, possRCs, rightindex, newrc:
description "PCAD_MakeSquareFreeOverRC: Given a real point encoded by regular chain and isolating cube, and a set of polynomials, produce a set of polynomials with equivalent zeros which are squarefree over the real point."
			"This may result in a simpler RC being calculated, but we no longer allow this to be outputted as the chain may be simpler than one required for a sample point.":

if pset=[] then
	return(pset):
fi:
ret:=[]:
for i from 1 to nops(pset) do
	if RegularChains:-MainVariable(pset[i],R)<>R['variables'][1] then
		ret:=[op(ret),pset[i]]:
	else
		out:=RegularChains:-ChainTools:-SquarefreeFactorization( pset[i], R['variables'][1], rc, R, 'method'='src'):
		if nops(out)<>1 then
			possRCs:=map(X -> X[-1],out):
			possRCs:=map(X -> RegularChains:-ChainTools:-Chain( PCAD_SortEqsRC( [op(extraeqs),op(RegularChains:-Equations(X,R))] ,R), RegularChains:-ChainTools:-Empty(R), R ), possRCs):
			rightindex:=PCAD_WhichRCInSplitHasSolutionAtSP( possRCs, SPcube, RegularChains:-PolynomialRing( R['variables'][2..-1] ) ):
			out:=out[rightindex]:
		else
			out:=op(out):
		fi:
		newrc:=op(2,out):
		if RegularChains:-ChainTools:-IsIncluded(newrc,rc,R)<>true then
			error("SquarefreeFactorization output is not as expected. This is an unexpected error message - please report to M.England@bath.ac.uk"):
		fi:
		out:=op(1,out):
		newpol:=1:
		for j from 1 to nops(out) do
			newpol:=newpol*op(1,op(j,out)):
		od:
		ret:=[op(ret), expand(newpol) ]:
	fi:
od:
return(ret):
end proc:

#####################################################################

PCAD_SPtoRootOf:=proc(rc::table, cube::list, R, $) :: list:
local i, dim, eqs, vars, pt:
description "PCAD_SPtoRootOf: Convert a sample point encoded by regular chain and isolating cube into a list of substitutions giving the variables as algebraic numbers.":

if rc[type]<>'regular_chain' then
	error("first input should be regular chain. This is an unexpected error message - please report to M.England@bath.ac.uk"):
fi:
if R[type]<>'polynomial_ring' then
	error("third input should be a polynomial ring. This is an unexpected error message - please report to M.England@bath.ac.uk"):
fi:
dim:=nops(cube):
vars:=[op(1..dim, ListTools:-Reverse( R['variables'] ))]:
eqs:=ListTools:-Reverse( RegularChains:-Equations(rc,R) ):
if nops(eqs)<>nops(cube) then
	error("cannot isolate a solution of regular chain. This is an unexpected error message - please report to M.England@bath.ac.uk"):
fi:
pt:=[]:
for i from 1 to dim do
	if op(i,cube)[1]=op(i,cube)[2] then
		pt:=[op(pt), vars[i]=op(i,cube)[1]]:
	else
		pt:=[op(pt), vars[i]=RootOf( subs(pt,eqs[i]), op(i,cube)[1]..op(i,cube)[2]) ]:
	fi:
od:
end proc:

#####################################################################

PCAD_SortCadCell:=proc( c1, c2, $ ) :: boolean:
local tmp:
description "PCAD_SortCadCell: A rule to decide the order of cadcells according to their index. For use as a rule in sort.":

tmp:=[op(1,c1),op(1,c2)]:
if sort( tmp ) = tmp then
	return(true):
else
	return(false):
fi:
end proc:

#####################################################################

PCAD_GenerateStack:=proc( cell::list, in_F::list, R, output, $ ) :: 'list':
local i, F, switch, cellIndex, cellSP, SPrc, SPrceqs, SPcube, retstack, RPrc, RPrceqs, extra:
description "PCAD_GenerateStack: Generate a stack over a cell given a set of polynomials. Uses TRDgenerate_stack but if cell is zero dim will first preconditions polynomials so they are of the right format.":

if kernelopts('opaquemodules')=true then
	switch:=0:
	kernelopts('opaquemodules'=false):
fi:
F := expand(in_F):
cellIndex:=cell[1]:
cellSP:=cell[-1]:
SPrc:=cellSP[1]:
SPcube:=cellSP[2]:
SPrceqs:=ListTools:-Reverse(RegularChains:-Equations(SPrc,R)):
if nops(SPrceqs)<>nops(R['variables'])-1 then
	error("the sample point is not of the expected dimension. This is an unexpected error message - please report to M.England@bath.ac.uk"):
fi:
RPrceqs:=[]:
for i from 1 to nops(cellIndex) do
	if cellIndex[i]::'even' then
		RPrceqs:=[op(RPrceqs), SPrceqs[i]]:
	fi:
od:
extra:=convert(convert(SPrceqs,'set') minus convert(RPrceqs,'set'),'list'):
if RPrceqs=[] then
	if not PCAD_IsCellFullDim(cellIndex) then
		error("the regular chain was not as expected. This is an unexpected error message - please report to M.England@bath.ac.uk"):
	fi:
else
	RPrc:=RegularChains:-ChainTools:-Chain( PCAD_SortEqsRC(RPrceqs,R), RegularChains:-ChainTools:-Empty(R), R):
	F:=PCAD_MakeCoprimeOverRC( RPrc, F, R ):
	F:=PCAD_MakeSquareFreeOverRC( RPrc, extra, SPcube, F, R ):
fi:
if output='list' then
	retstack:=RegularChains:-TRDgenerate_stack( [ cell, cell[1] ], F, R ):
else
	retstack:=RegularChains:-TRDgenerate_stack_general( [ cell, cell[1] ], F, R):
fi:
if switch=0 then
	kernelopts('opaquemodules'=true):
fi:
retstack:
end proc:

#####################################################################

PCAD_ProjCADLift:=proc( ProjPolys::set, vars::list, method, finalCAD, retcad, out, failure, $ )
local i, j, k, num, switch, dim, R, contset, pset, cad, tmpcad, tmppset, cell, SPrc, SPcube, cellIndex, alpha, falpha, DP, ICZD:
description "PCAD_ProjCADLift: Given a list of projection polynomials and a list of variables, compute a CAD.":

if kernelopts('opaquemodules')=true then
	switch:=0:
	kernelopts('opaquemodules'=false):
fi:
dim:=nops(vars):
contset:={}:
for i from 1 to dim do
	R[i]:=RegularChains:-PolynomialRing( vars[-i..-1] ):
	num:=dim+1-i:
	pset[num]:=select(has, remove(has,ProjPolys,vars[1..i-1]), vars[i]):
	pset[num]:=pset[num] union contset:
	contset:=PCAD_ContSet(pset[num], vars[i]):
	pset[num]:=PCAD_PrimSet(pset[num], vars[i]):
	pset[num]:=convert(pset[num],'list'):
	pset[num]:=PCAD_SetFactors(pset[num]):
	pset[num]:=convert(pset[num],'list'):
	pset[num]:=sort(pset[num],'length'):
	if pset[i]=[] then WARNING("there are no projection polynomials in %1", R[i]['variables']): fi:
od:
pset[1]:=remove(X -> sturm(X, vars[-1], -infinity, infinity) = 0, pset[1]):
if out='list' then
	cad[1]:=RegularChains:-TRDgenerate_stack([], pset[1], R[1]):
else
	cad[1]:=RegularChains:-TRDgenerate_stack_general( [], pset[1], R[1] ):
fi:
cad[1]:=map(X->X[1], cad[1]):
userinfo(2, {'ProjectionCAD', 'PCAD_ProjCADLift', 'CADLifting', 'CADFull'},"produced CAD of", R[1]['variables'], "-space with", nops(cad[1]), "cells"):
if retcad=1 then
	return(cad[1]):
fi:
for i from 2 to dim do
	tmpcad:=table():
	for j from 1 to nops(cad[i-1]) do
		cell:=cad[i-1][j]:
		cellIndex:=cell[1]:
		SPrc:=op(1,op(-1,cell)):
		SPcube:=op(2,op(-1,cell)):
		alpha:=PCAD_SPtoRootOf( SPrc,SPcube,R[i-1] ):
		tmppset:=[]:
		for k from 1 to nops(pset[i]) do
			falpha:=subs(alpha,op(k,pset[i])):
			falpha:=is(falpha=0):
			if falpha=true then
				userinfo(3, {'ProjectionCAD', 'PCAD_ProjCADLift', 'CADLifting', 'CADFull'},"a projection polynomial was nullified on cell", cellIndex):
				ICZD:=PCAD_IsCellZeroDim( cellIndex ):
				if i=dim then
					if finalCAD='SI' then
						tmppset:=[op(tmppset)]:
						userinfo(3, {'ProjectionCAD', 'PCAD_ProjCADLift', 'CADLifting', 'CADFull'},"the nullified polynomial was discarded since this is the final lift and only sign invariance is required."):
					else
						if ICZD=true then
							DP:=PCAD_MinimalDelineatingPolynomial(op(k,pset[i]),alpha,R[i]):
							if DP::constant then
								tmppset:=[op(tmppset)]:
								userinfo(3, {'ProjectionCAD', 'PCAD_ProjCADLift', 'CADLifting', 'CADFull'},"the nullified polynomial was discarded since cell", cellIndex, "is zero-dim and the minimal delineating polynomial is a constant."):
							else
								tmppset:=[op(tmppset), DP]:
								userinfo(3, {'ProjectionCAD', 'PCAD_ProjCADLift', 'CADLifting', 'CADFull'}, "cell", cellIndex, "is zero-dim so a delineating polynomial was used."):
								userinfo(4, {'ProjectionCAD', 'PCAD_ProjCADLift', 'CADLifting', 'CADFull'}, "the delineating polynomial used is", DP):
							fi:
						else
							userinfo(2, {'ProjectionCAD', 'PCAD_ProjCADLift', 'CADLifting', 'CADFull'}, "the polynomial", op(k,pset[i]), "is nullified by", alpha, "which is a cell of dim>0."):
							if failure='err' then
								error("there is nullification on cell %1 which has dim>0. The outputted CAD cannot be guaranteed order-invariant.", cellIndex):
							elif failure='giveFAIL' then
								return(FAIL):
							else
								WARNING("there is nullification on cell %1 which has dim>0. The nullified polynomial was discarded. The outputted CAD may not be order-invariant.", cellIndex):
								tmppset:=[op(tmppset)]:
							fi:
						fi:
					fi:
				else
					if method='McCallum' then
						if ICZD=true then
							DP:=PCAD_MinimalDelineatingPolynomial(op(k,pset[i]),alpha,R[i]):
							if DP::constant then
								tmppset:=[op(tmppset)]:
								userinfo(3, {'ProjectionCAD', 'PCAD_ProjCADLift', 'CADLifting', 'CADFull'},"the nullified polynomial was discarded since cell", cellIndex, "is zero-dim and the minimal delineating polynomial is a constant."):
							else
								tmppset:=[op(tmppset), DP]:
								userinfo(3, {'ProjectionCAD', 'PCAD_ProjCADLift', 'CADLifting', 'CADFull'},"cell", cellIndex, "is zero-dim a delineating polynomial was used."):
								userinfo(4, {'ProjectionCAD', 'PCAD_ProjCADLift', 'CADLifting', 'CADFull'},"the delineating polynomial used is", DP):
							fi:
						else
							userinfo(2, {'ProjectionCAD', 'PCAD_ProjCADLift', 'CADLifting', 'CADFull'}, "the polynomial", op(k,pset[i]), "is nullified by", alpha, "which is a cell of dim>0."):
							if failure='err' then
								error("there is nullification on cell %1 which has dim>0. The outputted CAD cannot be guaranteed sign-invariant.", cellIndex):
							elif failure='giveFAIL' then
								return(FAIL):
							else
								WARNING("there is nullification on cell %1 which has dim>0. The nullified polynomial was discarded. The outputted CAD may not be sign-invariant.", cellIndex):
								tmppset:=[op(tmppset)]:
							fi:
						fi:
					else
						tmppset:=[op(tmppset)]:
						userinfo(3, {'ProjectionCAD', 'PCAD_ProjCADLift', 'CADLifting', 'CADFull'},"the nullified polynomial on cell", cellIndex, "was discarded since we are using Collins lifting."):
					fi:
				fi:
			else tmppset:=[op(tmppset),op(k,pset[i])]:
			fi:
		od:
		tmpcad[j]:=op(PCAD_GenerateStack( cell, tmppset, R[i], out )):
	od:
	cad[i]:=[seq( tmpcad[k], k=1..nops(cad[i-1]) )]:
	cad[i]:=map(X->op(1,X),cad[i]):
	if cad[i]<>sort(cad[i], 'PCAD_SortCadCell') then error("the cells are not in order. This is an unexpected error message - please report to M.England@bath.ac.uk"); fi:
	userinfo(2, {'ProjectionCAD', 'PCAD_ProjCADLift', 'CADLifting', 'CADFull'},"produced CAD of", R[i]['variables'], "-space with", nops(cad[i]), "cells"):
	if retcad=i then
		return(cad[i]):
	fi:
od:
if switch=0 then
	kernelopts('opaquemodules'=true):
fi:
if out='list' or out='listwithrep' then
	return( cad[dim] ):
elif out='rootof' then
	tmpcad:=map(X -> X[2],cad[dim]):
	tmpcad:=map(Y-> map(X -> `if`(op(0,X)=And, op(X), X), Y), tmpcad);
	return(tmpcad):
elif out='piecewise' then
	return( PCAD_LWRCADtoPWCAD( cad[dim] ) ):
fi:
end proc:

#####################################################################

PCAD_LWRCADtoPWCAD:=proc( LCAD::list ) :: 'list':
local dim, Tree, Leaf, i, NumRoots, SubCAD, Rep:
description "PCAD_LWRCADtoPWCAD: Convert a Cad in listwithrep output format to a piecewise CAD.":

dim:=convert(map(X->nops(X[1]),LCAD),set):
if nops(dim)<>1 then
	error("cells have indices of different length. This is an unexpected error message - please report to M.England@bath.ac.uk"):
else
	dim:=op(dim):
fi:
if dim=0 then
	if nops(LCAD)<>1 or LCAD[1][1]<>[] or LCAD[1][2]<>[] then
		error("converting to piecewise did not go as expected. This is an unexpected error message - please report to M.England@bath.ac.uk"):
	fi:
	Leaf:=LCAD[1][3]:
	return(Leaf):
fi:
NumRoots:=max(map(X->X[1,1],LCAD)):
Tree:=[]:
for i from 1 to NumRoots do
	SubCAD:=select(X->X[1][1]=i, LCAD):
	Rep:=convert(map(X->X[2,1],SubCAD),set):
	if nops(Rep)<>1 then
		error("cells on same branch have different representations. This is an unexpected error message - please report to M.England@bath.ac.uk"):
	else
		Rep:=op(Rep):
	fi:
	SubCAD:=map(X->[X[1][2..dim], X[2][2..dim], X[3]], SubCAD):
	Tree:=[op(Tree), Rep, PCAD_LWRCADtoPWCAD( SubCAD )]:
od:
piecewise(op(Tree)):
end proc:

#####################################################################
#  Section 6 - TTICAD and ECCAD
#####################################################################

TTI_TTIProjectionOperator := proc( E::table, A::table, t::posint, mvar::symbol, lvars::list, $ ) :: set:
local i, j, BigE, RESX, P, BigP:
description "TTI_TTIProjectionOperator: Computes the application of the TTICAD Projection operator.",
			"Input: Tables E and A, an integer t, the main variable for the application and a list of any other variables.  The integr t is the number of QFFs in Phi.",
			"The tables have entries for 1..t with each entry a set of polynomials.",
			"Output: The set of polynomials obtained by applying the TTI projection operator.":

BigE := map(X -> op(X), {entries(E, 'nolist')}):
RESX := {}:
for i from 1 to nops(BigE) do
	for j from i+1 to nops(BigE) do:
		RESX:={op(RESX), resultant(op(i,BigE), op(j,BigE), mvar)}:
	od:
od:
P := table():
for i from 1 to t do
	P[i]:=TTI_ECReducedProjectionOperator( E[i], A[i], mvar, lvars):
od:
BigP := `union`(seq(P[i], i=1..t)):
BigP := BigP union RESX:
BigP := remove(X -> X::'constant', BigP):
RETURN(BigP):
end proc:

#####################################################################

TTI_ECReducedProjectionOperator := proc( E::set, A::set, mvar::symbol, lvars::list, $ ) :: set:
local j, k, numE, numA, P:
description "TTI_ECReducedProjectionOperator: Computes the application of the reduced projection operator for equational constraints.",
			"Input: Sets of polynomials E and A, the main variable for the application and a list of any other variables. ",
			"Output: The set of polynomials obtained by applying the reduced projection operator.":

numE:=nops(E):
numA:=nops(A):
P := PCAD_McCallumProj(E, mvar, lvars):
for j from 1 to numE do
	for k from 1 to numA do:
		P:={op(P), resultant(op(j,E), op(k,A), mvar)}:
	od:
od:
P := remove(X->X::'constant', P):
end proc:

#####################################################################

TTI_ECCAD:=proc( IN::list, vars::list(symbol), LiftAll::boolean, output::symbol, retcad::nonnegint, $ )
local f, gs, i, n, mvar, lvars, E, F, A, B, C, P, lowercad, cad, cell, FinalCAD:
description "TTI_ECCAD: Compute the CAD using equational constraint at first level.",
			"Input: f, a polynomial representing an equational constraint and gs, a list of polynomials representing the other constraints.",
			"Also a list of ordered variables and optionally, an ouput choice and retcad level."
			"Output: A CAD which is sign invariant for f and also for g on those cells where f=0.":

f:=IN[1]:
gs:=IN[2]:
n := nops(vars):
mvar := vars[1]:
lvars := remove(X->X=mvar,vars):
E := {f}:
F := PCAD_PrimSet(E, mvar):
F := PCAD_SFBasis(F, mvar):
if n=1 then
	return( TTI_UnivariateCase(F, mvar, output) ):
fi:
A := {f, op(gs)}:
B := PCAD_PrimSet(A, mvar):
C := PCAD_ContSet(A, mvar):
P := TTI_ECReducedProjectionOperator( F, B, mvar, lvars):
userinfo(4, {'ProjectionCAD'},"applying the reduced projection operator gave the following projection polynomials:", P):
lowercad := TTI_LowerDimCAD(P union  C, lvars, output, retcad):
if lowercad=FAIL then
	print("The reduced projection set is not well oriented.  Hence CADW fails and the ECCAD cannot be produced"):
	return(FAIL):
fi:
if retcad=n or retcad=0 then
	cad:=table():
	for i from 1 to nops(lowercad) do
		cell := lowercad[i]:
		cad[i] := TTI_TTIGenerateStack( cell, 1, mvar, lvars, table([(1)=gs]), table([(1)=B]), table([(1)=E]), LiftAll, output):
		if cad[i]=FAIL then
			print("The equational constraint is nullified on a cell of positive dimension and the excluded polynomials are not constant.  Hence this implementation of ECCAD fails."):
			return(FAIL):
		fi:
	od:
	FinalCAD := [entries(cad, 'nolist')]:
	FinalCAD := map(X->op(X), FinalCAD):
	userinfo(2, {'ProjectionCAD'},"produced CAD of", vars, "-space with", nops(FinalCAD), "cells"):
else
	FinalCAD := lowercad:
fi:
if output='list' or output='listwithrep' then
	return(FinalCAD):
elif output='rootof' then
	return( map(X->X[2], FinalCAD) ):
elif output='piecewise' then
	return( PCAD_LWRCADtoPWCAD(FinalCAD) ):
else
	error("unexpected output format.  This is an unexpected error, please report to M.England@bath.ac.uk"):
fi:
end proc:

#####################################################################

TTI_ECPP:=proc( IN::list, vars::list(symbol), $ ) :: set:
local f, gs, inpols, n, mvar, lvars, E, F, A, B, C, topP, lowP, PP:
description "TTI_ECPP: Compute the set of polynomials used by ECCAD for pojection.",
			"Input: f, a polynomial representing an equational constraint and gs, a list of polynomials representing the other constraints.",
			"Also a list of ordered variables.",
			"Output: The set of projection polynomials.":

f := IN[1]:
gs := IN[2]:
n := nops(vars):
inpols := {f,op(gs)}:
mvar := vars[1]:
lvars := remove(X->X=mvar,vars):
E := {f}:
F := PCAD_PrimSet(E, mvar):
F := PCAD_SFBasis(F, mvar):
if n=1 then
	PP:=E:
else
	A := {f, op(gs)}:
	B := PCAD_PrimSet(A, mvar):
	C := PCAD_ContSet(A, mvar):
	topP := TTI_ECReducedProjectionOperator( F, B, mvar, lvars):
	lowP := PCAD_McCProjPolys(topP union C, lvars):
	PP := topP union lowP union inpols:
fi:
PCAD_SetFactors(PP):
end proc:

#####################################################################

TTI_TTICAD:=proc( PHI::list, vars::list, LiftAll::boolean, output::symbol, retcad::nonnegint, $ )
local i, t, n, mvar, lvars, phi, f, gs, E, F, A, B, C, BigF, BigC, P, lowercad, cad, cell, FinalCAD:
description "TTI_TTICAD: Compute the TTICAD for a list of QFFs.",
			"Input: PHI, a list of lists phi[i], each of which represent a QFF.  Each phi[i] has two arguments; the first a polynomial (equational constraint) and the second a list of polynomials (from the other constraints).",
			"Also a list of ordered variables and optionally, an ouput choice and retcad level."
			"Output: A truth-table invariant CAD for PHI or failure if the polynomials are not well oriented.":

t := nops(PHI):
n := nops(vars):
mvar := vars[1]:
lvars := remove(X->X=mvar,vars):
phi := table():
f := table():
gs := table():
E := table():
F := table():
for i from 1 to t do
	phi[i] := PHI[i]:
	f[i] := expand(phi[i][1]):
	gs[i] := expand(phi[i][2]):
	if f[i]=false then
		E[i] := {op(gs[i])}:
	else
		E[i] := {f[i]}:
	fi:
	F[i] := PCAD_PrimSet(E[i], mvar):
	F[i] := PCAD_SFBasis(F[i], mvar):
od:
BigF := `union`(seq(F[i], i=1..t)):
if n=1 then
	return( TTI_UnivariateCase(BigF, mvar, output) ):
fi:
A := table():
B := table():
C := table():
for i from 1 to t do
	A[i] := {f[i], op(gs[i])}:
	B[i] := PCAD_PrimSet(A[i], mvar):
	C[i] := PCAD_ContSet(A[i], mvar):
od:
BigC := `union`(seq(C[i], i=1..t)):
P := TTI_TTIProjectionOperator(F, B, t, mvar, lvars):
P := P union  BigC:
userinfo(4, {'ProjectionCAD'},"applying the reduced projection operator gave the following projection polynomials:", P):
lowercad := TTI_LowerDimCAD(P, lvars, output, retcad):
if lowercad=FAIL then
	print("The reduced projection set is not well oriented.  Hence CADW fails and the TTICAD cannot be produced"):
	return(FAIL):
fi:
if retcad=0 or retcad=n then
	cad:=table():
	for i from 1 to nops(lowercad) do
		cell := lowercad[i]:
		cad[i] := TTI_TTIGenerateStack( cell, t, mvar, lvars, gs, B, E, LiftAll, output):
		if cad[i]=FAIL then
			print("An equational constraint is nullified on a cell of positive dimension and the excluded polynomials are not constant.  Hence this implementation of TTICAD fails."):
			return(FAIL):
		fi:
	od:
	FinalCAD := [entries(cad, 'nolist')]:
	FinalCAD := map(X->op(X), FinalCAD):
	userinfo(2, {'ProjectionCAD'},"produced CAD of", vars, "-space with", nops(FinalCAD), "cells"):
else
	FinalCAD := lowercad:
fi:
if output='list' or output='listwithrep' then
	return(FinalCAD):
elif output='rootof' then
	return( map(X->X[2], FinalCAD) ):
elif output='piecewise' then
	return( PCAD_LWRCADtoPWCAD(FinalCAD) ):
else
	error("unexpected output format.  This is an unexpected error, please report to M.England@bath.ac.uk"):
fi:
end proc:

#####################################################################

TTI_TTIPP:=proc( PHI::list, vars::list, $ ) :: set:
local i, t, n, mvar, lvars, phi, f, gs, E, F, A, B, C, BigF, BigC, BigA, topP, lowP, PP:
description "TTI_TTIPP: Compute the projection polynomials used in TTICAD for a list of QFFs.",
			"Input: PHI, a list of lists phi[i], each of which represent a QFF.  Each phi[i] has two arguments; the first a polynomial (equational constraint) and the second a list of polynomials (from the other constraints).",
			"Also a list of ordered variables."
			"Output: The set of projection polynomials.":

t := nops(PHI):
n := nops(vars):
mvar := vars[1]:
lvars := remove(X->X=mvar,vars):
phi := table():
f := table():
gs := table():
E := table():
F := table():
for i from 1 to t do
	phi[i] := PHI[i]:
	f[i] := expand(phi[i][1]):
	gs[i] := expand(phi[i][2]):
	E[i] := {f[i]}:
	F[i] := PCAD_PrimSet(E[i], mvar):
	F[i] := PCAD_SFBasis(F[i], mvar):
od:
BigF := `union`(seq(F[i], i=1..t)):
if n=1 then
	PP:=BigF:
else
	A := table():
	B := table():
	C := table():
	for i from 1 to t do
		A[i] := {f[i], op(gs[i])}:
		B[i] := PCAD_PrimSet(A[i], mvar):
		C[i] := PCAD_ContSet(A[i], mvar):
	od:
	BigA := `union`(seq(A[i], i=1..t)):
	BigC := `union`(seq(C[i], i=1..t)):
	topP := TTI_TTIProjectionOperator(F, B, t, mvar, lvars):
	lowP := PCAD_McCProjPolys(topP union BigC, lvars):
	PP:= topP union lowP union BigA
fi:
PCAD_SetFactors(PP):
end proc:

#####################################################################

TTI_UnivariateCase := proc( F::set(polynom), var::symbol, output::symbol, $ )
local cad:
description "TTI_UnivariateCase: Dealing with the case where there is only one variable.":

if output='list' then
	cad := RegularChains:-TRDgenerate_stack( [], convert(F,'list'), RegularChains:-PolynomialRing([var]) ):
	cad:=map(X->X[1], cad):
	return(cad):
else
	cad := RegularChains:-TRDgenerate_stack_general( [], convert(F,'list'), RegularChains:-PolynomialRing([var]) ):
fi:
cad:=map(X->X[1], cad):
if output='listwithrep' then
	return(cad):
elif output='piecewise' then
	return( PCAD_LWRCADtoPWCAD(cad) ):
elif output='rootof' then
	return( map(X->X[2], cad) ):
else error("unexpected output format.  This is an unexpected error, please report to M.England@bath.ac.uk"):
fi:
end proc:

#####################################################################

TTI_LowerDimCAD := proc( P::set(polynom), lvars::list(symbol), output::symbol, retcad::nonnegint, $ )
local pset, cad:
description "TTI_LowerDimCAD: Constructing the lower dimension CAD using CADW via PCAD.":

pset := PCAD_McCProjPolys(P, lvars):
userinfo(2, {'ProjectionCAD'},nops(pset), "projection factors were calculated for the lower dimensional CAD."):
userinfo(4, {'ProjectionCAD'},"the factors are:", pset):
if output='list' then
  cad:=PCAD_ProjCADLift( pset, lvars, 'McCallum', 'OI', retcad, 'list', 'giveFAIL' ):
else
  cad:=PCAD_ProjCADLift( pset, lvars, 'McCallum', 'OI', retcad, 'listwithrep', 'giveFAIL' ):
fi:
cad:
end proc:

#####################################################################

TTI_TTIGenerateStack := proc( cell::list, t::posint, mvar::symbol, lvars::list(symbol), gs::table, B::table, E::table, LiftAll::boolean, output::symbol, $ ) :: list:
local j, vars, Stack, SPrc, SPcube, alpha, beta, falpha, L, cellIndex, excl, entry, nullification:
description "TTI_TTIGenerateStack: Creating the lifting set and then stack over a cell in the lower dimensional CAD.":

cellIndex := cell[1]:
vars:=[mvar, op(lvars)]:
SPrc := cell[-1][1]:
SPcube := cell[-1][2]:
alpha := PCAD_SPtoRootOf( SPrc, SPcube, RegularChains:-PolynomialRing(lvars) ):
beta := []:
for j from 1 to nops(cellIndex) do
	if cellIndex[j]::'even' then
		beta:=[op(beta), alpha[j]]:
	fi:
od:
L:={}:
for j from 1 to t do
	nullification := false:
	for entry in E[j] do
		falpha := subs(alpha, entry):
		if is(falpha=0)=true then
			userinfo(3, {'ProjectionCAD', 'TTICAD'},"the equational constraint factor", entry, " is nullified on the cell ", cell):
			nullification := true:
		fi:
	od:
	if nullification=true then
		if PCAD_IsCellZeroDim(cellIndex)=true then
			L := L union B[j]:
			L := remove(X -> evalb(is(subs(alpha,X)=0)), L):
			userinfo(3, {'ProjectionCAD', 'TTICAD'},"the cell is zero-dimensional so we can continue by expanding the lifting set on this cell."):
		else
			excl := TTI_ExclProj_phi(gs[j], vars):
			excl := remove(X->X::'constant', subs(beta, excl)):
			userinfo(4, {'ProjectionCAD', 'TTICAD'},"the excluded polynomials are ", excl, " on the cell."):
			if excl = {} then
				L := L union B[j]:
				L := remove(X -> evalb(is(subs(alpha,X)=0)), L):
				userinfo(3, {'ProjectionCAD', 'TTICAD'},"the relevant excluded polynomials are constants, hence the non-equational constraints are delineable on the cell and we can continue by expanding the lifting set on this cell."):
			else
				userinfo(3, {'ProjectionCAD', 'TTICAD'},"the cell has positive dimension and the relevant excluded polynomials are non-constants.  Hence the algorithm fails."):
				return(FAIL):
			fi:
		fi:
	else
		if LiftAll=true then
			userinfo(3, {'ProjectionCAD', 'TTICAD'},"we only need to lift with respect to the ECs, but since the LiftAll option was given we expand the lifting set."):
			L := L union B[j]:
			L := remove(X -> evalb(is(subs(alpha,X)=0)), L):
		else
			L := L union E[j]:
		fi:
	fi:
od:
L := PCAD_SetFactors(L):
L := convert(L, 'list'):
if output=list then
	Stack := PCAD_GenerateStack(cell, L, RegularChains:-PolynomialRing(vars), 'list'):
else
	Stack := PCAD_GenerateStack(cell, L, RegularChains:-PolynomialRing(vars), 'listwithrep'):
fi:
Stack := map(X->X[1],Stack):
end proc:

#####################################################################

TTI_ResCADSet := proc( PHI::list, vars::list(symbol), $ ) :: list:
local i, j, k, t, n, mvar, phi, f, gs, E, F, A, B, C, R, numE, numA, BigE:
description "TTI_ResCADSet: Compute the ResCAD set.",
			"Input: PHI, a list of lists phi[i], each of which represent a QFF.  Each phi[i] has two arguments; the first a polynomial (equational constraint) and the second a list of polynomials (from the other constraints).",
			"Also a list of ordered variables."
			"Output: The ResCAD set.":

t := nops(PHI):
n := nops(vars):
if n=1 then
	WARNING("There is only one variable and hence no projection and no ResCADSet."):
	return([]):
fi:
mvar := vars[1]:
phi := table():
f := table():
gs := table():
A := table():
B := table():
C := table():
E := table():
F := table():
for i from 1 to t do
	phi[i] := PHI[i]:
	f[i] := expand(phi[i][1]):
	gs[i] := expand(phi[i][2]):
	if f[i]=false then
		E[i] := {op(gs[i])}:
		A[i] := {op(gs[i])}:
	else
		E[i] := {f[i]}:
		A[i] := {f[i], op(gs[i])}:
	fi:
	if has(map(X->has(X, mvar), E[i]), false) then
		WARNING("An equational constraint will certainly be nullified since %1 does not contain the main variable %2.", f[i], mvar):
	fi:
	F[i] := PCAD_PrimSet(E[i], mvar):
od:
BigE := `union`(seq(E[i], i=1..t)):
for i from 1 to t do
	B[i] := PCAD_PrimSet(A[i], mvar):
	C[i] := PCAD_ContSet(A[i], mvar):
od:
R:={}:
for i from 1 to t do
	numE := nops(E[i]):
	numA := nops(A[i]):
	for j from 1 to numE do
		for k from 1 to numA do:
			R := {op(R), resultant(op(j, E[i]), op(k, A[i]), mvar)}:
		od:
	od:
od:
R := R union BigE:
PCAD_RemoveConstantMultiples( remove(X->X::'constant',R) ):
end proc:

#####################################################################

TTI_ResCAD := proc( PHI::list, vars::list(symbol), output, $ ) :: list:
local i, t, n, mvar, phi, f, gs, E, F, R, BigF, pset, cad:
description "TTI_ResCAD: Compute a TTICAD for a list of QFFs using the RESCAD method.",
			"Input: PHI, a list of lists phi[i], each of which represent a QFF.  Each phi[i] has two arguments; the first a polynomial (equational constraint) and the second a list of polynomials (from the other constraints).",
			"Also a list of ordered variables and optionally, an ouput choice."
			"Output: Failure if the reduced projection set is not well oriented.  Otherwise a CAD for PHI which is guarenteed truth table invariant provided that no equational constraint is nullified.":

n := nops(vars):
if n=1 then
	t := nops(PHI):
	mvar := vars[1]:
	phi := table():
	f := table():
	gs := table():
	E := table():
	F := table():
	for i from 1 to t do
		phi[i] := PHI[i]:
		f[i] := expand(phi[i][1]):
		if has(f[i], mvar)=false then
			WARNING("An equational constraint will certainly be nullified since %1 does not contain the main variable %2.", f[i], mvar):
		fi:
		gs[i] := expand(phi[i][2]):
		E[i] := {f[i]}:
		F[i] := PCAD_PrimSet(E[i], mvar):
	od:
	BigF := `union`(seq(F[i], i=1..t)):
	return( TTI_UnivariateCase(BigF, mvar, output) ):
fi:
R := TTI_ResCADSet(PHI, vars):
pset := PCAD_McCProjPolys(R, vars):
cad := PCAD_ProjCADLift(pset, vars, 'McCallum', 'SI', 0, output, 'giveFAIL' ):
if cad=FAIL then
	print("The reduced projection set is not well oriented.  Hence CADW fails and the ResCAD cannot be produced"):
	return(FAIL):
fi:
cad:
end proc:

#####################################################################

TTI_ExclProj_phi := proc(gs::list, vars::list(symbol)) :: set:
local mvar, excl, ent:
description "TTI_ExclProj_phi: Compute the set of polynomials excluded from the full projection set for a QFF by the redcued operator.",
			"Input: gs the set of non equational constraints for a QFF and the list of variables.",
			"Output: Set of elculded polynomials.":

mvar := vars[1]:
excl := {}:
for ent in gs do
	excl := {op(excl), discrim(ent, mvar), coeffs(expand(ent), mvar) }:
od:
excl := remove(X->X::'constant', excl):
excl := PCAD_RemoveConstantMultiples(excl):
PCAD_SFBasis(excl, vars[2]);
end proc:

#####################################################################
#  Section 7 - Formulation Commands
#####################################################################

Formulation_VarOrdFree :=  proc( vars::list(symbol), $ ) :: list:
description "Formulation_VarOrdFree: Finds all the possible variable orderings for a list of variables.",
			"Input: A list of variables.",
			"Ouput: A list of all posible acceptable variable orderings (each a list of variables).":

eval('combinat'['permute'](vars));
end proc:

#####################################################################

Formulation_VarOrdRestricted :=  proc( vars::list(list(symbol)), $ ) :: list:
local i, nBlocks, Parts, Poss, nPoss, num:
description "Formulation_VarOrdFree: Finds all the possible variable orderings for a list of variables in blocks.",
			"Input: A list of lists of variables.",
			"Ouput: A list of all posible acceptable variable orderings (each a list of variables).":

nBlocks:=nops(vars):
Parts:=table():
for i from 1 to nBlocks do
	Parts[i] := Formulation_VarOrdFree( vars[i] ):
od:
Poss:=Parts[1]:
num:=2:
while num<nBlocks+1 do
	nPoss:=table():
	for i from 1 to nops(Parts[num]) do
		 nPoss[i] := map(X -> [op(X), op(Parts[num][i])], Poss):
	od:
	Poss := map(X->op(X), [entries(nPoss, 'nolist')]):
	num:=num+1:
od:
Poss:
end proc:

#####################################################################

Formulations_RGS := proc(N::posint, i::posint) :: list:
local j, s, f, L, next_str:
description "Formulations_RGS: Creates the restricted growth strings for given input.",
			"Input: N, the length of the strings and i, the maximum permitted block size.",
			"Ouput: List of possible restricted growth strings.  When i=1 we get the rgs' for possible partitions of a set of size N.":

next_str := proc()
local k, j, sk, m1, mp:
# Returns index of first changed element in s[] and zero if current string is the last
k := N:
do
	k := k-1:
	if k = 0 then
		return(0):
	fi:
	sk := s[k] + 1;
	m1 := f[k - 1];
	mp := m1 + i;
	if sk > mp then
		s[k] := 0;
	else
		s[k] := sk:
		if sk = mp then
			m1 := m1 + i:
		fi:
		for j from k to N-1 do
			f[j] := m1:
		od:
		return(k):
	fi
od
end proc:
s := array(0..N-1);
f := array(0..N-1);
for j from 0 to N-1 do
	s[j] := 0:
od:
for j from 0 to N-1 do
	f[j] := 0:
od:
L := NULL:
do
	L := L, convert(s,list);
	if next_str() = 0 then
		break
	fi:
od:
[L]:
end proc:

#####################################################################

Formulations_Classification := proc(s::list) :: list:
local L, i, m:
description "Formulations_Classification: Turns a restricted growth string (in the case i=1) to a set partition.",
			"Input: s, a restricted growth string.",
			"Ouput: The set partition corresponding to s.":

m := 1 + max(op(s));
L:=table():
for i from 1 to m do
	L[i]:=[]:
od:
L:=[entries(L, 'nolist')]:
for i from 1 to nops(s) do
	L[s[i]+1] := [op(L[s[i]+1]),i];
od;
L:
end proc:

#####################################################################

Formulations_SetPartitions := proc(n::posint) :: list:
local P, S, s, i:
description "Formulations_SetPartitions: Create the possible set partitions of an integer.",
			"Input: n, a positive integer.",
			"Ouput: The list of all set partitions of n.":
P := table():
S := Formulations_RGS(n,1);
for i from 1 to nops(S) do
  s:=S[i]:
  P[i]:=Formulations_Classification(s):
od:
[entries(P, 'nolist')]:
end proc:

#####################################################################

Formulations_FunctionPartitions := proc(L::list) :: list:
local SP, entry, sets, i, count, s:
description "Formulations_FunctionPartitions: Create the possible set partitions of a list of functions.",
			"Input: L, a list of functions.",
			"Ouput: The list of all set partitions (stored as lists) of the functions.":

SP := Formulations_SetPartitions(nops(L)):
sets := table(): count:=1:
for entry in SP do
	for i from 1 to nops(entry) do
		s[i]:=L[op(i,entry)]:
	od:
	sets[count] := [seq(s[i], i=1..nops(entry))]:
	count := count+1:
od:
[entries(sets, 'nolist')]:
end proc:

#####################################################################

Formulations_ECsForTTI := proc(L::list) :: list:
local fparts, ECparts, entry, i, SS, entrysets, j, newentrysets, k, l, ent, count, COUNT;
description "Formulations_ECsForTTI: Create the list of partitions of equational constraints for use in TTICAD.",
			"Input: L, a list of polynomials (assumed equational constraints).",
			"Ouput: The list of all possible partitions (stored as lists) in which the first entry is a designated equational constraint.":

fparts := Formulations_FunctionPartitions(L):
ECparts := table():
COUNT:=1:
for entry in fparts do
	for i from 1 to nops(entry) do
		SS[i] := [seq( [entry[i][j], op(remove(has,entry[i],entry[i][j]))], j=1..nops(entry[i]) )]:
	od;
	entrysets := [[]]:
	for j from 1 to nops(entry) do
		newentrysets := table():
		count:=1:
		for k from 1 to nops(SS[j]) do
			for l from 1 to nops(entrysets) do
				ent := [op(entrysets[l]), SS[j][k]]:
				newentrysets[count] := ent:
				count := count+1:
			od:
		od:
	entrysets := [entries(newentrysets, 'nolist')]:
	od:
	ECparts[COUNT] := [op(entrysets)]:
	COUNT := COUNT+1:
od:
map(X->op(X), [entries(ECparts, 'nolist')]):
end proc:

#####################################################################

Formulations_ECCAD := proc( IN::list ) :: list:
local i, ecs, necs:
description "Formulation_ECCAD: Create the list of choices for entry into ECCAD.",
			"Input: Two lists of polynomials, the first assumed equational constraints.",
			"Ouput: A list of possible formulations for ECCAD.  Each list has a polynomial (designated EC) and a list of other constraints.":

ecs := IN[1]:
necs := IN[2]:
[seq( [ecs[i],[op(remove(X->X=ecs[i],ecs)),op(necs)]], i=1..nops(ecs))]:
end proc:

#####################################################################

Formulations_QFF := proc( IN::list(list(polynom)), $ ) :: list:
local F, G, count, Parts, i, j, k, tempParts:
description "Formulations_Claise: Create the list of possible formulations for a TTICAD QFF.",
			"Input: Two lists of polynomials, the first assumed the equational constraints.",
			"Ouput: The list of all possible formulations for TTICAD (inc splitting).":

F := IN[1]:
G := IN[2]:
Parts := Formulations_ECsForTTI(F):
for i from 1 to nops(G) do
	tempParts := table():
	count := 1:
	for j from 1 to nops(Parts) do
		for k from 1 to nops(Parts[j]) do
			tempParts[count] := [op(1..k-1, Parts[j]), [op(Parts[j][k]), G[i]], op(k+1..nops(Parts[j]), Parts[j])]:
			count := count+1:
		end do:
	end do:
	Parts := [entries(tempParts, 'nolist')]:
od:
map(X-> map(Y-> [Y[1],Y[2..nops(Y)]] ,X), Parts):
end proc:

#####################################################################

Formulations_TTICAD := proc( LL::list(list(list(polynom))), splitting::boolean, $ ) :: list:
local i, N, F, G, Parts, Poss, nPoss, num:
description "Formulations_TTICAD: Create the list of possible formulations for TTICAD.",
			"Input: A list of lists each representing a QFF.  Each QFF has two lists of polynomials, the first assumed the equational constraints.  Optionally, the choice of whether to consider splitting.",
					"Note: it is assumed that the QFFs come from a disjunctive normal form.  I.e. trivial merging is not possible.",
			"Ouput: The list of all possible formulations for TTICAD, by default including splitting.":

N := nops(LL):
if splitting=false then
	for i from 1 to N do
		F:=LL[i][1]:
		G:=LL[i][2]:
		Parts[i] := map(X->[X], Formulations_ECCAD([F,G])):
	od:
else
	for i from 1 to N do
		F:=LL[i][1]:
		G:=LL[i][2]:
		Parts[i] := Formulations_QFF( [expand(F), expand(G)] ):
	od:
fi:
Poss:=Parts[1]:
num:=2:
while num<N+1 do
	nPoss:=table():
	for i from 1 to nops(Parts[num]) do
		 nPoss[i]:=map(X->[op(X),op(Parts[num][i])], Poss):
	od:
	Poss:=map(X->op(X), [entries(nPoss, 'nolist')]):
	num:=num+1:
od:
Poss:
end proc:

#####################################################################
#  Section 8 - Heuristics
#####################################################################

Heuristics_sotdP := proc(g::polynom) :: posint:
local G:
description "Heuristics_sotdP: Calculate the sotd (sum of total degree) of a polynomial.":

G:=expand(g):
if G::`+` then
	return( map(degree, expand(g)) );
else
	return( degree(g) ):
fi:
end proc:

#####################################################################

Heuristics_sotdL := proc(IN) :: posint:
local L, f:
description "Heuristics_sotdL: Calculate the sotd (sum of total degree) of a list / set of polynomials.":

if IN::'set' then
	L := convert(IN, 'list'):
elif IN::'list' then
	L := IN:
else
	error("invalid input:  sotdL expects its input to be a list or set of polynomials.  This is an unexpected erroor - please report to M.England@bath.ac.uk"):
fi:
add( Heuristics_sotdP(f), f in L):
end proc:

#####################################################################

Heuristics_ndrr := proc(INN, var::symbol, method::symbol, naivedeg::posint, Quiet::boolean, $) :: posint:
local i, othervars, Uni, pol, deg, SFUni:
description "Heuristics_ndrr: Calculate the ndrr (number of distinct real roots) of a polynomial or list / set of polynomials with respect to a variable.  Note: if multivariate polynomials are included then they are ignored.",
			"Input: Either a polynomial of list / set of polynomials, and a variable.",
			"Ouput: The ndrr.":

if INN::'polynom' then
	if indets(INN, 'name') <> {var} then
		return(0):
	else
		return( sturm(INN, var, -infinity, infinity) ):
	fi:
elif INN::'set' or INN::'list' then
	othervars := remove(X->X=var,indets(INN,'name')):
	Uni := convert(remove(has,INN,othervars),'set'):
	if Uni={} then
		return(0):
	fi:
	if method='naive' then
		return( add( sturm(Uni[i], var, -infinity, infinity), i=1..nops(Uni) ) ):
	elif method='SFnaive' then
		SFUni := PCAD_SFBasis(Uni, var):
		return( add( sturm(SFUni[i], var, -infinity, infinity), i=1..nops(Uni) ) ):
	else
		pol := mul(Uni[i], i=1..nops(Uni)):
		deg := degree(pol):
		if method='full' then
			return( sturm( pol, var, -infinity, infinity) ):
		elif method='vary' then
			if deg < naivedeg then
				return( sturm( pol, var, -infinity, infinity) ):
			else
				if Quiet=false then
					WARNING("The method is %1 and degree of the product of the polynomials is greater that naivedeg=%2.  Hence the ndrr for the individual polynomials will been returned, which is an overestimate.  To avoid this use the optional argument method=full or naivedeg=int where int>80", method, naivedeg):
				fi:
				return( add( sturm(Uni[i], var, -infinity, infinity), i=1..nops(Uni) ) ):
			fi:
		else
			error("this is an unexpected error.  Please report to M.England@bath.ac.uk"):
		fi:
	fi:
else
	error("this is an unexpected error.  Please report to M.England@bath.ac.uk"):
fi:
end proc:

#####################################################################

Heuristics_PickSuggestions := proc( Poss::list, Hvalues::list, {pick::symbol:='minimise'}, $) :: list:
local bestH, Suggestions, count, i:
description "Heuristics_PickSuggestions: Selects suggested formulations based on heuristic values.",
			"Input: Two lists, the first a list of possible formulations, the second a list of heuristic values.  It is assumed we are picking the formulations to minimise the heuristic, if maximise then specify as an option.",
			"Ouput: A list of those formulations with the min / masx heuristic value.":

if nops(Poss)<>nops(Hvalues) then
	error("unexpected error - please report to M.England@bath.ac.uk"):
fi:
if 'pick'='minimise' then
	bestH := min(Hvalues):
elif 'pick'='maximise' then
	bestH := max(Hvalues):
else
	error("the optional input pick should be minimise or maximise"):
fi:
Suggestions := table():
count:=1:
for i from 1 to nops(Poss) do
	if Hvalues[i]=bestH then
		Suggestions[count] := Poss[i]:
		count:=count+1:
	fi:
od:
map(X->rhs(X), sort([entries(Suggestions, 'pairs')]));
end proc:

#####################################################################

Heuristics_RunOnPPs := proc(Poss::list, PPs::table, vars::list(symbol), heuristic::symbol, ndrrOptions::list, SeeAll::boolean, Wratio::list(posint), VarOrds::boolean, $) :: list(list):
local i, nPoss, Ns, Ss, Suggestions, SuggInd, Nsort, Ssort, Ws, num:
description "Heuristics_RunOnPPs: Given a table of projection polynomials and heuristic choice returns the suggestion.",
			"Input: A list of possible formulations, table of projection polynomials, choice of heuristic and whether to return one or all.",
			"Ouput: A single formulation or list of formulations suggested by the heuristic.":

nPoss := nops(Poss):
if heuristic in {'N','SN','NS','W'} then
	if VarOrds=false then
		Ns := [seq( ndrr(PPs[i], vars[-1], 'method'=ndrrOptions[1], 'naivedeg'=ndrrOptions[2]), i=1..nPoss)]:
	else
		Ns := [seq( ndrr(PPs[i], Poss[i][-1], 'method'=ndrrOptions[1], 'naivedeg'=ndrrOptions[2]), i=1..nPoss)]:
	fi:
fi:
if heuristic in {'S','SN','NS','W'} then
	Ss := [seq( sotd(PPs[i]), i=1..nPoss)]:
fi:
if heuristic in {'N','NS'} then
	Suggestions:=Heuristics_PickSuggestions( Poss, Ns, 'pick'='minimise' ):
	num:=nops(Suggestions):
	userinfo(2, {'ProjectionCAD'}, "the fomulations have been compared for lowest ndrr value, identifying ", num):
	if nops(Suggestions)=1 then
		return( op(Suggestions) ):
	elif heuristic='N' then
		return( Heuristics_Display(Suggestions, SeeAll, heuristic) ):
	else
		SuggInd := select( X -> has( map(Y->is(Y=Poss[X]),Suggestions) ,true)=true, [seq(1..nPoss)] ):
#		SuggInd := select( X -> has(Suggestions,Poss[X])=true, [seq(1..nPoss)] ):
		Ss := Ss[SuggInd]:
		Suggestions := Heuristics_PickSuggestions( Suggestions, Ss, 'pick'='minimise' ):
		num := nops(Suggestions):
		userinfo(2, {'ProjectionCAD'}, "the formulations with lowest ndrr value have had their sotd values compared for the lowest, identifying, ", num):
		return( Heuristics_Display(Suggestions, SeeAll, heuristic) ):
	fi:
elif heuristic in {'S','SN'} then
	Suggestions := Heuristics_PickSuggestions( Poss, Ss, 'pick'='minimise' ):
	num := nops(Suggestions):
	userinfo(2, {'ProjectionCAD'}, "the fomulations have been compared for lowest sotd value, identifying ", num):
	if nops(Suggestions)=1 then
		return( op(Suggestions) ):
	elif heuristic='S' then
		return( Heuristics_Display(Suggestions, SeeAll, heuristic) ):
	else
		SuggInd := select( X -> has( map(Y->is(Y=Poss[X]),Suggestions) ,true)=true, [seq(1..nPoss)] );
#		SuggInd := select( X -> has(Suggestions,Poss[X])=true, [seq(1..nPoss)] ):
		Ns := Ns[SuggInd]:
		Suggestions:=Heuristics_PickSuggestions( Suggestions, Ns, 'pick'='minimise' ):
		num:=nops(Suggestions):
		userinfo(2, {'ProjectionCAD'}, "the formulations with lowest sotd value have had their ndrr values compared for the lowest, identifying, ", num):
		return( Heuristics_Display(Suggestions, SeeAll, heuristic) ):
	fi:
else
	Ws := table():
	Nsort := sort(Ns):
	Ssort := sort(Ss):
	Ws := [seq( Heuristics_FindPos(Nsort,Ns[i])*Wratio[1] + Heuristics_FindPos(Ssort,Ss[i])*Wratio[2], i=1..nPoss)]:
	Suggestions := Heuristics_PickSuggestions( Poss, Ws, 'pick'='minimise' ):
	num:=nops(Suggestions):
	userinfo(2, {'ProjectionCAD'}, "the formulations have been ranked with respect to ndrr and sotd values with a weighted average taken according to ratio N:S = ", Wratio, ".  Picking the lowest value of this identifies ", num):
	return( Heuristics_Display(Suggestions, SeeAll, heuristic) ):
fi:
end proc:

#####################################################################

Heuristics_FindPos := proc( Hvalues::list(posint), H::posint ) :: posint:
local i:
description "Heuristics_FindPos: Find where a value is in a list.",
			"Input: A list of values andone value from that list.",
			"Ouput: The position of that value in the list.":

for i from 1 to nops(Hvalues) do
	if Hvalues[i]=H then
		return(i):
	fi:
od:
error("unexpected error - please report to M.England@bath.ac.uk"):
end proc:

#####################################################################

Heuristics_Display := proc( Suggestions::list, SeeAll::boolean, heuristic::symbol )
local N:
description "Heuristics_Display: Given the formulations suggested by a heuristic decides whether to display one or all.",
			"Input: A list of formulations, the choice of how many to see and the heuristic used.",
			"Ouput: A single formulation or a list of formulations.":

N:=nops(Suggestions):
if N=1 then
	return(op(Suggestions)):
else
	if SeeAll=true then
		WARNING("There are %1 formulations which heuristic %2 cannot differentiate.", N, heuristic):
		return(Suggestions):
	elif SeeAll=false then
		WARNING("There are %1 formulations which heuristic %2 cannot differentiate.  Only the first has been returned.  To display them all run with option SeeAll=true.", N, heuristic):
		return( op(1,Suggestions) ):
	else
		return( op(1,Suggestions) ):
	fi:
fi:
end proc:

#####################################################################

Heuristics_ECCAD := proc(ecs::list(polynom), necs::list(polynom), vars::list(symbol), heuristic::symbol, ndrrOptions::list, SeeAll::boolean, Wratio, $) :: list(list):
local i, Poss, nPoss, PPs:
description "Heuristics_EC: Selects a formulation for ECCAD using heuristics.",
			"Input: Two lists of polynomials (the first assumed equational constraints) and a variable ordering.  Optionally a heuristic choice (S, N, NS or SN) for which the default is NS.",
			"Ouput: A single ECCAD formulation or list of the ECCAD formulations suggested by the heuristic.":

Poss := Formulations_ECCAD( [ecs, necs] ):
nPoss := nops(Poss):
userinfo(2, {'ProjectionCAD'}, nPoss, "possible formulations have been identified"):
PPs := table():
for i from 1 to nPoss do
	PPs[i] := TTI_ECPP( Poss[i], vars ):
od:
Heuristics_RunOnPPs( Poss, PPs, vars, heuristic, ndrrOptions, SeeAll, Wratio, false):
end proc:

#####################################################################

Heuristics_TTICADQFF := proc( IN::list(list(polynom)), vars::list(symbol), heuristic::symbol, ndrrOptions::list, SeeAll::boolean, splitting::boolean, Wratio, $) :: list(list):
local F, G, i, Poss, nPoss, PPs:
description "Heuristics_TTICADQFF : Selects formulation(s) for a TTICADQFF using heuristics.",
			"Input: Two lists of polynomials (the first assumed equational constraints) and a variable ordering.  Optionally a heuristic choice (S, N, NS or SN) for which the default is NS and the choice to consider splittings (for which the default is true).",
			"Ouput: A single TTICAD formulation or list of the TTICAD formulations suggested by the heuristicselected partitions.":

F := IN[1]:
G := IN[2]:
if splitting=false then
	Poss := ( map(X->[X],Formulations_ECCAD([F,G])) ):
else
	Poss := ( Formulations_QFF( [expand(F), expand(G)] ) ):
fi:
nPoss := nops(Poss):
userinfo(2, {'ProjectionCAD'}, nPoss, "possible formulations have been identified"):
PPs := table():
for i from 1 to nPoss do
	PPs[i] := TTI_TTIPP( Poss[i], vars ):
od:
Heuristics_RunOnPPs( Poss, PPs, vars, heuristic, ndrrOptions, SeeAll, Wratio, false):
end proc:

#####################################################################

Heuristics_TTICAD := proc(LL::list(list(list(polynom))), vars::list(symbol), modular::boolean, heuristic::symbol, ndrrOptions::list, SeeAll::boolean, splitting::boolean, Wratio, $) :: list(list):
local i, Poss, nPoss, PPs, F, G, Suggestion:
description "Heuristics_TTICAD: Selects formulation(s) for a sequence of QFFs for TTICAD using heuristics.",
			"Input: A list of lists each representing a QFF.  Each QFF has two lists of polynomials, the first assumed the equational constraints.",
					"Optionally a heuristic choice (S, N, NS or SN) for which the default is NS and the choice to consider splittings (for which the default is true).",
					"Note: it is assumed that the QFFs come from a disjunctive normal form.  I.e. trivial merging is not possible.",
			"Ouput: A single TTICAD formulation or list of the TTICAD formulations suggested by the heuristicselected partitions.":

if modular=false then
	Poss := Formulations_TTICAD(LL, splitting):
	nPoss := nops(Poss):
	userinfo(2, {'ProjectionCAD'}, nPoss, "possible formulations have been identified"):
	PPs := table():
	for i from 1 to nPoss do
		PPs[i] := TTI_TTIPP( Poss[i], vars ):
	od:
	Heuristics_RunOnPPs( Poss, PPs, vars, heuristic, ndrrOptions, SeeAll, Wratio, false):
else
	userinfo(2, {'ProjectionCAD'}, "the modular algorithm selects the formulation of each QFF in turn."):
	Suggestion:=table():
	for i from 1 to nops(LL) do
		userinfo(2, {'ProjectionCAD'}, "now considering QFF", i):
		F:=LL[i][1]:
		G:=LL[i][2]:
		Suggestion[i] := op(Heuristics_TTICADQFF( [F, G], vars, heuristic, ndrrOptions, FAIL, splitting, Wratio)):
	od:
	return( [entries( Suggestion, 'nolist')] ):
fi:
end proc:

#####################################################################

Heuristics_VariableOrdering := proc( VarsList::list, IN, algorithm::symbol, greedy::boolean, heuristic::symbol, method::symbol, ndrrOptions::list, SeeAll, Wratio::list, $ ) :: list:
local PossVarOrds, i, k, t, nPoss, PPs, E, A, B, C, F, mvar, lvars, PS, Poss, selected, FirstBlock, allvars, newFirstBlock, newVarsList, BigC:
description "Heuristics_VariableOrdering: Pick a variable ordering based on a heuristic.",
			"Input: The input for a CAD algorithm, a variable list (possibly in blocks) and an algorithm selection.",
			"Ouput: Suggested variable ordering(s).":

if greedy=false then
	if VarsList::'list'(symbol) then
		PossVarOrds := Formulation_VarOrdFree(VarsList):
	elif VarsList::'list'('list'('symbol')) then
		PossVarOrds := Formulation_VarOrdRestricted(VarsList):
	fi:
	nPoss := nops(PossVarOrds):
	userinfo(2, {'ProjectionCAD'}, nPoss, "possible formulations have been identified"):
	PPs := table():
	if algorithm='algorithm_CADFull' then
		if method='McCallum' then
			for i from 1 to nPoss do
				PPs[i] := PCAD_McCProjPolys( IN, PossVarOrds[i] ):
			od:
		elif method='Collins' then
			for i from 1 to nPoss do
				PPs[i] := PCAD_CCADProjPolys( IN, PossVarOrds[i] ):
			od:
		else
			error("this is an unexpected error - please report to M.England@bath.ac.uk"):
		fi:
	elif algorithm='algorithm_ECCAD' then
		for i from 1 to nPoss do
			PPs[i] := TTI_ECPP( IN, PossVarOrds[i] ):
		od:
	elif algorithm='algorithm_TTICAD' then
		for i from 1 to nPoss do
			PPs[i] := TTI_TTIPP( IN, PossVarOrds[i] ):
		od:
	else
		error("this is an unexpected error - please report to M.England@bath.ac.uk"):
	fi:
	return( Heuristics_RunOnPPs( PossVarOrds, PPs, [], heuristic, ndrrOptions, SeeAll, Wratio, true) ):
else
	if algorithm='algorithm_CADFull' then
		if VarsList::'list'(symbol) then
			return( Heuristics_GVOCADFull( VarsList, {}, IN, heuristic, ndrrOptions, Wratio, false) ):
		else
			return( Heuristics_GVOCADFullBlocks( VarsList, IN, heuristic, ndrrOptions, Wratio, false) ):
		fi:
	elif algorithm='algorithm_ECCAD' then
		E := expand({IN[1]}):
		A := expand({IN[1], op(IN[2])}):
		if VarsList::'list'(symbol) then
			PPs := table():
			for i from 1 to nops(VarsList) do
				mvar:=VarsList[i]:
				lvars := remove(X->X=mvar, VarsList):
				F := PCAD_PrimSet(E, mvar):
				F := PCAD_SFBasis(F, mvar):
				if nops(VarsList)=1 then
					PPs[i]:=E:
				else
					B := PCAD_PrimSet(A, mvar):
					C := PCAD_ContSet(A, mvar):
					PPs[i] := PCAD_SetFactors( TTI_ECReducedProjectionOperator( F, B, mvar, lvars) union C ):
				fi:
			od:
			Poss:=[seq( [PPs[i], VarsList[i]], i=1..nops(VarsList))]:
			selected:=Heuristics_RunOnPPs( Poss, PPs, [], heuristic, ndrrOptions, FAIL, Wratio, true):
			PS:=selected[1]:
			mvar := selected[2]:
			lvars := remove(X->X=mvar, VarsList):
			lvars := Heuristics_GVOCADFull( lvars, {}, PS, heuristic, ndrrOptions, Wratio, false):
			return([mvar,op(lvars)]):
		else
			FirstBlock := VarsList[1]:
			allvars:=map(X->op(X),VarsList):
			if nops(FirstBlock)=1 then
				mvar:=op(FirstBlock):
				lvars := remove(X->X=mvar, allvars):
				F := PCAD_PrimSet(E, mvar):
				F := PCAD_SFBasis(F, mvar):
				B := PCAD_PrimSet(A, mvar):
				C := PCAD_ContSet(A, mvar):
				PS := PCAD_SetFactors( TTI_ECReducedProjectionOperator( F, B, mvar, lvars) union C ):
				lvars := Heuristics_GVOCADFullBlocks( VarsList[2..nops(VarsList)], PS, heuristic, ndrrOptions, Wratio, false):
				return([mvar,op(lvars)]):
			else
				PPs := table():
				for i from 1 to nops(FirstBlock) do
					mvar:=FirstBlock[i]:
					lvars := remove(X->X=mvar, allvars):
					F := PCAD_PrimSet(E, mvar):
					F := PCAD_SFBasis(F, mvar):
					B := PCAD_PrimSet(A, mvar):
					C := PCAD_ContSet(A, mvar):
					PPs[i] := PCAD_SetFactors( TTI_ECReducedProjectionOperator( F, B, mvar, lvars) union C ):
				od:
				Poss:=[seq( [PPs[i], FirstBlock[i]], i=1..nops(FirstBlock))]:
				selected:=Heuristics_RunOnPPs( Poss, PPs, [], heuristic, ndrrOptions, FAIL, Wratio, true):
				PS:=selected[1]:
				mvar := selected[2]:
				newFirstBlock := remove(X->X=mvar, FirstBlock):
				newVarsList := [newFirstBlock, op(VarsList[2..nops(VarsList)])]:
				lvars := Heuristics_GVOCADFullBlocks( newVarsList, PS, heuristic, ndrrOptions, Wratio, false):
				return([mvar,op(lvars)]):
			fi:
		fi:
	elif algorithm='algorithm_TTICAD' then
		t := nops(IN):
		A := table():
		E := table():
		for k from 1 to t do
			E[k] := expand( {IN[k][1]} ):
			A[k] := expand( {IN[k][1], op(IN[k][2])} ):
		od:
		if VarsList::'list'(symbol) then
			PPs := table():
			for i from 1 to nops(VarsList) do
				mvar:=VarsList[i]:
				lvars := remove(X->X=mvar, VarsList):
				B := table():
				C := table():
				for k from 1 to t do
					B[k] := PCAD_PrimSet(A[k], mvar):
					C[k] := PCAD_ContSet(A[k], mvar):
					F[k] := PCAD_PrimSet(E[k], mvar):
					F[k] := PCAD_SFBasis(F[k], mvar):
				od:
				BigC := `union`(seq(C[k], k=1..t)):
				PPs[i] := PCAD_SetFactors( TTI_TTIProjectionOperator(F, B, t, mvar, lvars) union BigC ):
			od:
			Poss:=[seq( [PPs[i], VarsList[i]], i=1..nops(VarsList))]:
			selected:=Heuristics_RunOnPPs( Poss, PPs, [], heuristic, ndrrOptions, FAIL, Wratio, true):
			PS:=selected[1]:
			mvar := selected[2]:
			lvars := remove(X->X=mvar, VarsList):
			lvars := Heuristics_GVOCADFull( lvars, {}, PS, heuristic, ndrrOptions, Wratio, false):
			return([mvar,op(lvars)]):
		else
			FirstBlock := VarsList[1]:
			allvars:=map(X->op(X),VarsList):
			if nops(FirstBlock)=1 then
				mvar:=op(FirstBlock):
				lvars := remove(X->X=mvar, allvars):
				B := table():
				C := table():
				for k from 1 to t do
					B[k] := PCAD_PrimSet(A[k], mvar):
					C[k] := PCAD_ContSet(A[k], mvar):
					F[k] := PCAD_PrimSet(E[k], mvar):
					F[k] := PCAD_SFBasis(F[k], mvar):
				od:
				BigC := `union`(seq(C[k], k=1..t)):
				PS := PCAD_SetFactors( TTI_TTIProjectionOperator(F, B, t, mvar, lvars) union BigC ):
				lvars := Heuristics_GVOCADFullBlocks( VarsList[2..nops(VarsList)], PS, heuristic, ndrrOptions, Wratio, false):
				return([mvar,op(lvars)]):
			else
				PPs := table():
				for i from 1 to nops(FirstBlock) do
					mvar:=FirstBlock[i]:
					lvars := remove(X->X=mvar, allvars):
				B := table():
				C := table():
				for k from 1 to t do
					B[k] := PCAD_PrimSet(A[k], mvar):
					C[k] := PCAD_ContSet(A[k], mvar):
					F[k] := PCAD_PrimSet(E[k], mvar):
					F[k] := PCAD_SFBasis(F[k], mvar):
				od:
				BigC := `union`(seq(C[k], k=1..t)):
				PPs[i] := PCAD_SetFactors( TTI_TTIProjectionOperator(F, B, t, mvar, lvars) union BigC ):
				od:
				Poss:=[seq( [PPs[i], FirstBlock[i]], i=1..nops(FirstBlock))]:
				selected:=Heuristics_RunOnPPs( Poss, PPs, [], heuristic, ndrrOptions, FAIL, Wratio, true):
				PS:=selected[1]:
				mvar := selected[2]:
				newFirstBlock := remove(X->X=mvar, FirstBlock):
				newVarsList := [newFirstBlock, op(VarsList[2..nops(VarsList)])]:
				lvars := Heuristics_GVOCADFullBlocks( newVarsList, PS, heuristic, ndrrOptions, Wratio, false):
				return([mvar,op(lvars)]):
			fi:
		fi:
	else
		error("this is an unexpected error - please report to M.England@bath.ac.uk"):
	fi:
fi:
end proc:

#####################################################################

Heuristics_GVOCADFull := proc( vars::list(symbol), lvars::set(symbol), P::set(polynom), heuristic::symbol, ndrrOptions::list, Wratio::list, retpols::boolean, $ ) :: list:
local i, cont, othervars, Avars, UAvars, PPs, PS, var, pset, Poss, selected:
description "Heuristics_GVOBlock: Use the greedy method to choose a variable ordering (based on CADFull-McCallum).",
			"Input: Variables, polynomials, and the usual optional heuristic arguments.",
			"Ouput: Suggested variable ordering.":

PS:=P:
Avars:=[]:
UAvars:=vars:
while nops(UAvars)>1 do
	PPs := table():
	for i from 1 to nops(UAvars) do
		var:=UAvars[i]:
		othervars := convert(remove(X->X=var,UAvars), 'set') union lvars:
		cont:=PCAD_ContSet(PS,var):
		pset:=PCAD_McCallumProj( PS, var, convert(othervars, 'list') ):
		PPs[i]:=PCAD_SetFactors( pset union cont ):
	od:
	Poss:=[seq( [PPs[i], UAvars[i]], i=1..nops(UAvars))]:
	selected:=Heuristics_RunOnPPs( Poss, PPs, [], heuristic, ndrrOptions, FAIL, Wratio, true):
	Avars:=[op(Avars), selected[2]]:
	UAvars:=remove(X->X=selected[2], UAvars):
	PS:=selected[1]:
od:
if nops(UAvars)<>1 then
	error("This is an unexpected error.  Please report to M.England@bath.ac.uk"):
else
	var := op(UAvars):
	cont := PCAD_ContSet(PS,var):
	pset := PCAD_McCallumProj( PS, var, convert(lvars, 'list') ):
	PS := PCAD_SetFactors( pset union cont ):
	Avars:=[op(Avars), op(UAvars)]:
fi:
if retpols=false then
	return(Avars):
else
	return(Avars,PS):
fi:
end proc:

#####################################################################

Heuristics_GVOCADFullBlocks := proc( VarsBlocks::list(list(symbol)), P::set(polynom), heuristic::symbol, ndrrOptions::list, Wratio::list, retpols::boolean, $ ) :: list:
local i, k, numBlocks, PS, Avars, lvars, vars:
description "Heuristics_GVOBlock: Use the greedy method to choose a variable ordering (based on CADFull-McCallum) when variables in blocks.",
			"Input: Variables, polynomials, and the usual optional heuristic arguments.",
			"Ouput: Suggested variable ordering.":

numBlocks:=nops(VarsBlocks):
PS := P:
Avars := []:
for k from 1 to numBlocks do
	lvars:={seq(op(VarsBlocks[i]), i=(k+1)..numBlocks)}:
	vars,PS := Heuristics_GVOCADFull( VarsBlocks[k], lvars, PS, heuristic, ndrrOptions, Wratio, true):
	Avars:=[op(Avars), op(vars)]:
od:
if retpols=false then
	return(Avars):
else
	return(Avars,PS):
fi:
end proc:

#####################################################################

end module:
