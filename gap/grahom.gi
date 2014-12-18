#############################################################################
##
#W  grahom.gi
#Y  Copyright (C) 2014                                     Julius Jonusas
##                                                         James Mitchell
##
##  Copyright (C) 2014 - Julius Jonusas and James Mitchell.
##
##  Licensing information can be found in the README file of this package.
##
#############################################################################
##

# Wrapper for C function GRAPH_HOMOS

InstallGlobalFunction(HomomorphismGraphsFinder, 
function(gr1, gr2, hook, user_param, limit, hint, isinjective, image) 
  local out;

  if not (IsDigraph(gr1) and IsDigraph(gr2) and IsSymmetricDigraph(gr1)
    and IsSymmetricDigraph(gr2)) then 
    Error("not yet implemented");
  fi;

  if hook <> fail and not IsFunction(hook) then
    Error("Graphs: HomomorphismGraphsFinder: usage,\n",
          "<hook> has to be a function,");
    return;
  fi;

  if limit = infinity then
    limit := fail;
  elif limit <> fail and not IsPosInt(limit) then
    Error("Graphs: HomomorphismGraphsFinder: usage,\n",
          "<limit> has to be a positive integer or infinity,");
    return;
  fi;

  if hint = infinity then
    hint := fail;
  elif hint <> fail and not IsPosInt(hint) then
    Error("Graphs: HomomorphismGraphsFinder: usage,\n",
          "<hint> has to be a positive integer or infinity,");
    return;
  fi;

  if not IsBool(isinjective) then
    Error("Graphs: HomomorphismGraphsFinder: usage,\n",
          "<isinjective> has to be a bool,");
    return;
  fi;

  if not (image = fail or IsList(image))  then
    Error("Graphs: HomomorphismGraphsFinder: usage,\n",
          "<image> has to be a plist,");
    return;
  fi;
  
  if DigraphNrVertices(gr1) <= 512 and DigraphNrVertices(gr2) <= 512 then
    out := GRAPH_HOMOS(gr1, gr2, hook, user_param, limit, hint, isinjective, image,
	   fail, fail);
    return out;
  else 
    Error("not yet implemented");
    return;
  fi;
end);

#

InstallGlobalFunction(GeneratorsOfEndomorphismMonoid, 
function(arg)
  local digraph, limit, gens, out;

  digraph := arg[1];

  if HasGeneratorsOfEndomorphismMonoidAttr(digraph) then 
    return GeneratorsOfEndomorphismMonoidAttr(digraph);
  fi;
  
  if not (IsDigraph(digraph) and IsSymmetricDigraph(digraph)) then 
    Error("not yet implemented for non-symmetric digraphs,");
  fi;

  if IsDigraph(digraph) and DigraphHasLoops(digraph) then
    Error("not yet implemented for digraphs with loops,");
  fi;

  if IsBound(arg[2]) and (IsPosInt(arg[2]) or arg[2] = infinity) then 
    limit := arg[2];
  else 
    limit := infinity;
  fi;

  gens := List(GeneratorsOfGroup(AutomorphismGroup(digraph)), AsTransformation);
  if limit = infinity then 
    limit := fail;
  else 
    limit := limit - Length(gens);
  fi;

  if limit <= 0 then 
    return gens;
  fi;
  
  out := HomomorphismGraphsFinder(digraph, digraph, fail, gens, limit, fail, false, fail);

  if limit = fail then 
    SetGeneratorsOfEndomorphismMonoidAttr(digraph, out);
  fi;
  return out;
end);

#

InstallMethod(DigraphColoring, "for a digraph and pos int",
[IsDigraph, IsPosInt],
function(digraph, n)
  if IsMultiDigraph(digraph) then 
    Error("Graphs: DigraphColoring: usage,\n",
    "the argument <digraph> must not be a  multigraph,");
    return;
  fi;
  return DigraphHomomorphism(digraph, CompleteDigraph(n)); 
end);

#

InstallMethod(GeneratorsOfEndomorphismMonoidAttr, "for a digraph",
[IsDigraph], 
function(digraph)
  return GeneratorsOfEndomorphismMonoid(digraph);
end);


# Finds a single homomorphism of highest rank from graph1 to graph2

InstallMethod(HomomorphismGraphs, "for a digraph and a digraph",
[IsDigraph, IsDigraph],
function(gr1, gr2)
  local out;

  out := HomomorphismGraphsFinder(gr1, gr2, fail, fail, 1,
   DigraphNrVertices(gr2), false, fail);

  if IsEmpty(out) then
    return fail;
  else
    return out[1];
  fi;
end);

# Finds a set S of homomorphism from graph1 to graph2 such that every
# homomorphism between the two graphs can expressed as a composition of an
# element in S and an automorphism of graph2.

InstallMethod(HomomorphismsGraphsRepresentatives, "for a digraph and a digraph",
[IsDigraph, IsDigraph],
function(gr1, gr2) 
  return HomomorphismGraphsFinder(gr1, gr2, fail, fail, fail,
   DigraphNrVertices(gr2), false, fail);
end);

InstallMethod(HomomorphismsDigraphs, "for a digraph and a digraph",
[IsDigraph, IsDigraph],
function(gr1, gr2) 
  local hom, aut;

  hom := HomomorphismsGraphsRepresentatives(gr1, gr2);
  aut := List(GeneratorsOfGroup(AutomorphismGroup(gr2)), AsTransformation);
  return Union(List(aut,  x -> hom * x));
end);


# Finds a single embedding of graph1 into graph2

InstallMethod(MonomorphismGraphs, "for a digraph and a digraph",
[IsDigraph, IsDigraph],
function(gr1, gr2)
  local out;
  
  if DigraphNrVertices(gr1) = DigraphNrVertices(gr2) then
    return IsomorphismDigraphs(gr1, gr2);
  fi;

  out := HomomorphismGraphsFinder(gr1, gr2, fail, fail, 1, DigraphNrVertices(gr2), true,
	 fail);

  if IsEmpty(out) then
    return fail;
  else
    return out[1];
  fi;
end);


