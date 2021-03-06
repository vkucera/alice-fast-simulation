//start_simulation.C

#ifndef __CINT__
#include <TSystem.h>
#include <TInterpreter.h>
#include <TROOT.h>
#include <AliLog.h>
#endif

#include <cstdio>
#include <iostream>

void start_simulation(TString name, Int_t pythiaEvents, TString procStr, TString gen, UInt_t seed, TString lhe, TString hep,
    TString beamType, Double_t ebeam1, Double_t ebeam2, Bool_t always_d_mesons, Bool_t extended_event_info, Double_t minPtHard = -1, Double_t maxPtHard = -1,
    UInt_t debug_level = 0)
{
  gInterpreter->AddIncludePath("$ALICE_ROOT/include");
  gInterpreter->AddIncludePath("$ALICE_PHYSICS/include");
  gInterpreter->AddIncludePath("$FASTJET/include");

  //load fastjet libraries 3.x
  gSystem->Load("libCGAL");

  gSystem->Load("libfastjet");
  gSystem->Load("libsiscone");
  gSystem->Load("libsiscone_spherical");
  gSystem->Load("libfastjetplugins");
  gSystem->Load("libfastjettools");
  gSystem->Load("libfastjetcontribfragile");

  gSystem->Load("liblhapdf_5_9_1.so");

  std::cout << "Loading libraries for PYTHIA6" << std::endl;
  gSystem->Load("libpythia6_4_28.so");

  std::cout << "Loading libraries for PYTHIA8" << std::endl;
  gSystem->Load("libpythia8210dev.so");
  gSystem->Load("libAliPythia8.so");
  gSystem->Setenv("PYTHIA8DATA", gSystem->ExpandPathName("$ALICE_ROOT/PYTHIA8/pythia8/xmldoc"));
  gSystem->Setenv("LHAPDF",      gSystem->ExpandPathName("$ALICE_ROOT/LHAPDF"));
  gSystem->Setenv("LHAPATH",     gSystem->ExpandPathName("$ALICE_ROOT/LHAPDF/PDFsets"));

  if (gen.Contains("evtgen")) {
    std::cout << "Loading libraries for EvtGen" << std::endl;

    gSystem->Load("libPhotos");
    gSystem->Load("libEvtGen");
    gSystem->Load("libTEvtGen");
  }

  gSystem->Load("libSTEER");
  gSystem->Load("libAOD");
  gSystem->Load("libPWGEMCALtasks");
  gSystem->Load("libPWGJEEMCALJetTasks");

  gSystem->Load("AnalysisCode.so");

  TString command = TString::Format(".x runJetSimulation.C+g(\"%s\", %d, \"%s\", \"%s\", %d, \"%s\", \"%s\", \"%s\", %f, %f, %d, %d, %f, %f, %d)",
      name.Data(), pythiaEvents, procStr.Data(), gen.Data(), seed, lhe.Data(), hep.Data(),
      beamType.Data(), ebeam1, ebeam2, always_d_mesons, extended_event_info, minPtHard, maxPtHard, debug_level);

  gROOT->ProcessLine(command.Data());
}
