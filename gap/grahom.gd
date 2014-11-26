#############################################################################
##
#W  grahom.gd
#Y  Copyright (C) 2014                                   James D. Mitchell
##
##  Licensing information can be found in the README file of this package.
##
#############################################################################
##

DeclareGlobalFunction("GeneratorsOfEndomorphismMonoid");
DeclareAttribute("GeneratorsOfEndomorphismMonoidAttr", IsDigraph);

DeclareOperation("HomomorphismDigraphs", [IsDigraph, IsDigraph]);
DeclareOperation("DigraphHomomorphism", [IsDigraph, IsDigraph]);
DeclareOperation("DigraphColoring", [IsDigraph, IsPosInt]);
DeclareOperation("HomomorphismGraphs", [IsDigraph, IsDigraph]);
DeclareOperation("MonomorphismGraphs", [IsDigraph, IsDigraph]);
DeclareOperation("HomomorphismGraphsRepresetatives", [IsDigraph, IsDigraph]);

DeclareGlobalFunction("HomomorphismGraphsFinder");
