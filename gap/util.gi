#############################################################################
##
#W  utils.gi
#Y  Copyright (C) 2013-14                                James D. Mitchell
##
##  Licensing information can be found in the README file of this package.
##
#############################################################################
##
## this file contains utilies for use with the Graphs package.

BindGlobal("GraphsDocXMLFiles", [ "attr.xml", "bliss.xml",
"graph.xml", "display.xml", "grape.xml", "io.xml", "oper.xml", "prop.xml", 
"util.xml", "../PackageInfo.g"]);

BindGlobal("GraphsTestRec", rec());
MakeReadWriteGlobal("GraphsTestRec");

InstallGlobalFunction(GraphsStartTest,
function()
  local record;

  record:=GraphsTestRec;
  
  record.timeofday := IO_gettimeofday();
  record.InfoLevelInfoWarning:=InfoLevel(InfoWarning);;
  record.InfoLevelInfoGraphs:=InfoLevel(InfoGraphs);;

  SetInfoLevel(InfoWarning, 0);;
  SetInfoLevel(InfoGraphs, 0);

  record.STOP_TEST := STOP_TEST;

  MakeReadWriteGlobal("STOP_TEST");
  UnbindGlobal("STOP_TEST");
  BindGlobal("STOP_TEST", GraphsStopTest);

  return;
end);

InstallGlobalFunction(GraphsStopTest,
function(file)
  local timeofday, record, elapsed, str;

  timeofday := IO_gettimeofday();
  
  record:=GraphsTestRec;
 
  elapsed := (timeofday.tv_sec - record.timeofday.tv_sec) * 1000 
   + Int((timeofday.tv_usec - record.timeofday.tv_usec) / 1000);

  str := "elapsed time: ";
  Append(str, String(elapsed));
  Append(str, "ms\n");
  
  SetInfoLevel(InfoWarning, record.InfoLevelInfoWarning);;
  SetInfoLevel(InfoGraphs, record.InfoLevelInfoGraphs);

  if not IsBound( GAPInfo.TestData.START_TIME )  then
      Error( "`STOP_TEST' command without `START_TEST' command for `", 
       file, "'" );
  fi;
  Print( GAPInfo.TestData.START_NAME, "\n" );
  
  SetAssertionLevel( GAPInfo.TestData.AssertionLevel );
  Unbind( GAPInfo.TestData.AssertionLevel );
  Unbind( GAPInfo.TestData.START_TIME );
  Unbind( GAPInfo.TestData.START_NAME );
  Print(str);
  MakeReadWriteGlobal("STOP_TEST");
  UnbindGlobal("STOP_TEST");
  BindGlobal("STOP_TEST", record.STOP_TEST);
  return; 
end);

InstallGlobalFunction(GraphsMakeDoc,
function()
  MakeGAPDocDoc(Concatenation(PackageInfo("graphs")[1]!.
   InstallationPath, "/doc"), "main.xml", GraphsDocXMLFiles, "graphs",
   "MathJax", "../../..");;
  return;
end);

InstallGlobalFunction(GraphsTestAll,
function()
  local dir_str, tst, dir, passed, filesplit, test, stringfile, filename;

  Print("Reading all .tst files in the directory graphs/tst/...\n\n");
  dir_str:=Concatenation(PackageInfo("graphs")[1]!.InstallationPath,"/tst");
  tst:=DirectoryContents(dir_str);
  dir:=Directory(dir_str);
  
  passed := true;

  for filename in tst do
    filesplit:=SplitString(filename, ".");
    if Length(filesplit)>=2 and filesplit[Length(filesplit)]="tst" then
      test:=true;
      stringfile:=StringFile(Concatenation(dir_str, "/", filename));
      if test then
        Print("reading ", dir_str,"/", filename, " . . .\n");
        passed := passed and Test(Filename(dir, filename));
        Print("\n");
      fi;
    fi;
  od;
  return passed;
end);

InstallGlobalFunction(GraphsTestInstall,
function()
  return Test(Filename(DirectoriesPackageLibrary("graphs","tst"),
   "testinstall.tst"));;
end);

InstallGlobalFunction(GraphsManualExamples,
function()
return
  ExtractExamples(DirectoriesPackageLibrary("graphs","doc"),
  "main.xml",  GraphsDocXMLFiles, "Single" );
end);

InstallGlobalFunction(GraphsTestManualExamples,
function()
  local ex, omit, str;

  ex:=GraphsManualExamples();
  omit:=GRAPHS_OmitFromTestManualExamples;
  if Length(omit)>0 then 
    Print("# not testing examples containing the strings");
    for str in omit do 
      ex:=Filtered(ex, x-> PositionSublist(x[1][1], str)=fail);
      Print(", \"", str, "\"");
    od;
    Print(" . . .\n");
  fi;

  GraphsStartTest();
  RunExamples(ex);
  GraphsStopTest("");
  return;
end);

InstallGlobalFunction(GraphsDir,
function()
  return PackageInfo("graphs")[1]!.InstallationPath;
end);

InstallGlobalFunction(GraphsTestEverything, 
function()

  GraphsMakeDoc();
  Print("\n");

  if not GraphsTestInstall() then 
    Print("Abort: testinstall.tst failed . . . \n");
    return false;
  fi;
  Print("\n");

  if not GraphsTestAll() then 
    Print("Abort: GraphsTestAll failed . . . \n");
    return false;
  fi;
  
  GraphsTestManualExamples();
  return;
end);

InstallGlobalFunction(IsSemigroupsLoaded,
function()
  return IsPackageMarkedForLoading("semigroups", "2.1");
end);
#EOF
