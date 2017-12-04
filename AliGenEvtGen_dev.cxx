/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 ***************************************************************************/
///////////////////////////////////////////////////////////////////////////
//  Class to generate decays of particles generated by a                 //
//  previous generator. It works as a generator, but pratically it       //
//  performs only decays. It works with this scheme: first loops over    // 
//  particles on the stack, selects those to be decayed, decays them     //
//  and then pushes the decay products on the stack.                     //
//                                                                       //
//  Giuseppe E. Bruno            &    Fiorella Fionda                    //
//  (Giuseppe.Bruno@ba.infn.it)       (Fiorella.Fionda@ba.infn.it)       //
///////////////////////////////////////////////////////////////////////////

#include <AliStack.h>
#include <AliRun.h>
#include <AliLog.h>
#include <TParticle.h>
#include <iostream>
#include <vector>
#include <array>
#include <memory>

#include "AliGenEvtGen_dev.h"

ClassImp(AliGenEvtGen_dev)
///////////////////////////////////////////////////////////////////////////
AliGenEvtGen_dev::AliGenEvtGen_dev():
  fStack(0x0),
  fDecayer(0x0),
  fForceDecay(kAll),
  fSwitchOff(kBeautyPart),
  fUserDecay(kFALSE),
  fUserDecayTablePath(0x0)
{
  //
  // Default Construction
  //
}
///////////////////////////////////////////////////////////////////////////////////////////
AliGenEvtGen_dev::~AliGenEvtGen_dev()
{
  //
  // Standard Destructor
  //
  if (fDecayer) { delete fDecayer; }
  fDecayer = 0;
}
///////////////////////////////////////////////////////////////////////////////////////////

void AliGenEvtGen_dev::Init()
{
  //
  // Standard AliGenerator Initializer - no input 
  // 1) initialize EvtGen with default decay and particle table
  // 2) set the decay mode to force particle 
  // 3) set a user decay table if defined
  //
  if(fDecayer) {
    AliWarningStream() << "AliGenEvtGen_dev already initialized!!!" << std::endl;
    return;
  }
  fDecayer = new AliDecayerEvtGen();

  //if is defined a user decay table
  if (fUserDecay) {
    char* path = const_cast<char*>(fUserDecayTablePath.Data());
    fDecayer->SetDecayTablePath(path);
  }

  fDecayer->Init(); //read the default decay table DECAY.DEC and particle table

  //if is set a decay mode: default decay mode is kAll  
  fDecayer->SetForceDecay(fForceDecay);
  fDecayer->ForceDecay();
  fDecayer->ReadDecayTable();
}

/////////////////////////////////////////////////////////////////////////////////////////////
void AliGenEvtGen_dev::Generate()
{
  //
  //Generate method - Input none - Output none
  //For each event:
  //1)return the stack of the previous generator and select particles to be decayed by EvtGen
  //2)decay particles selected and put the decay products on the stack
  //
  //

  if (!fStack) fStack = AliRunLoader::Instance()->Stack();
  if (!fStack) {
    AliErrorStream() << "No stack found!" << std::endl;
    return;
  }

  static TClonesArray* particles = nullptr;
  if (!particles) particles = new TClonesArray("TParticle",1000);

  Int_t nPrimsPythia = fStack->GetNprimary();
  AliDebugStream(1) << "nPrimsPythia = " << nPrimsPythia << std::endl;

  for (Int_t iTrack = 0; iTrack < nPrimsPythia; ++iTrack) {
    TParticle* part = fStack->Particle(iTrack);
    Int_t pdg = part->GetPdgCode();
    Int_t flav = GetFlavour(pdg);

    AliDebugStream(1) << "GetFlavour = " << flav << " and pdg = " << pdg << std::endl;

    switch (fSwitchOff) {
    case kAllPart:
      break;
    case kBeautyPart:
      if (flav != 5) continue;
      break;
    case kCharmPart:
      if (flav !=4 ) continue;
      break;
    }

    //check if particle is already decayed by Pythia
    if (part->GetStatusCode() != 1 || part->GetNDaughters() > 0) {
      if (TMath::Abs(pdg) > 10) AliWarningStream() << "Attention: particle " << pdg << " is already decayed by Pythia!" << std::endl;
      continue;
    }

    part->SetStatusCode(11); //Set particle as decayed : change the status code
    part->SetBit(kDoneBit);
    part->ResetBit(kTransportBit);

    TLorentzVector mom;
    mom.SetPxPyPzE(part->Px(),part->Py(),part->Pz(),part->Energy());

    Int_t np = -1;
    do {
      fDecayer->Decay(part->GetPdgCode(), &mom);
      np = fDecayer->ImportParticles(particles);
    } while(np < 0);

    AliDebugStream(1) << "np = " << np << std::endl;

    std::vector<Int_t> trackIt(np);
    std::vector<Int_t> pParent(np);

    for (int i = 0; i < np; i++) {
      pParent[i] = -1;
      trackIt[i] =  0;
    }

    //select trackable particle
    if (np > 1) {
      TParticle* iparticle =  nullptr;
      for (int i = 1; i < np ; i++) {
        iparticle = static_cast<TParticle*>(particles->At(i));
        Int_t ks = iparticle->GetStatusCode();

        //track last decay products
        if(ks==1) trackIt[i] = 1;

      }//decay particles loop

    }// if decay products

    std::array<Float_t,3> origin0 = {0};         // Origin of the parent particle
    origin0[0] = part->Vx(); //[cm]
    origin0[1] = part->Vy(); //[cm]
    origin0[2] = part->Vz(); //[cm]
    //
    // Put decay products on the stack
    //
    AliDebugStream(2) << "Successfully decayed particle " << pdg << " into " << np << " decay products" << std::endl;
    for (int i = 1; i < np; i++) {
      TParticle* iparticle = static_cast<TParticle*>(particles->At(i));
      Int_t kf   = iparticle->GetPdgCode();
      Int_t ksc  = iparticle->GetStatusCode();
      Int_t jpa  = iparticle->GetFirstMother() - 1; //jpa = 0 for daughters of beauty particles
      Int_t iparent = (jpa > 0) ? pParent[jpa] : iTrack;

      std::array<Float_t,3> polar = {0};  // Polarisation of daughter particles

      // Momentum and origin of the children particles from EvtGen
      std::array<Float_t,3> pc = {0};
      std::array<Float_t,3> och = {0};

      och[0] = origin0[0]+iparticle->Vx(); //[cm]
      och[1] = origin0[1]+iparticle->Vy(); //[cm]
      och[2] = origin0[2]+iparticle->Vz(); //[cm]
      pc[0]  = iparticle->Px(); //[GeV/c]
      pc[1]  = iparticle->Py(); //[GeV/c]
      pc[2]  = iparticle->Pz(); //[GeV/c]
      Float_t tof = part->T()+iparticle->T();

      AliDebugStream(1) << "FirstMother = " <<  jpa << " and indicePart = " << i << " and pdg = " << kf << std::endl;

      Int_t nt = -1;
      PushTrack(trackIt[i], iparent, kf, pc.data(), och.data(), polar.data(), tof, kPDecay, nt, 1., ksc);
      if (nt < 0) {
        AliWarning(Form("Particle %i, pdg = %i, could not be pushed and will be skipped.", i, kf));
        continue;
      }
      if (trackIt[i] == 1) AliDebugStream(1) << "Trackable particles: " << i << " and pdg " << kf << std::endl;
      pParent[i] = nt;
      KeepTrack(nt);
      SetHighWaterMark(nt);
    }// Particle loop

    particles->Clear();
  }
  AliInfoStream() << "AliGenEvtGen_dev DONE" << std::endl;
}

//////////////////////////////////////////////////////////////////////////////////////////
Int_t AliGenEvtGen_dev::GetFlavour(Int_t pdgCode)
{
  //
  // return the flavour of a particle
  // input: pdg code of the particle
  // output: Int_t 
  //         3 in case of strange (open and hidden)
  //         4 in case of charm (")
  //         5 in case of beauty (")
  //
  Int_t pdg = TMath::Abs(pdgCode);
  //Resonance
  if (pdg > 100000) pdg %= 100000;
  if (pdg > 10000)  pdg %= 10000;
  // meson ?
  if (pdg > 10) pdg /= 100;
  // baryon ?
  if (pdg > 10) pdg /= 10;
  return pdg;
}

/////////////////////////////////////////////////////////////////////////////////////////
Bool_t AliGenEvtGen_dev::SetUserDecayTable(const char* path)
{
  //
  //Set the path of user decay table if it is defined
  //
  //put a comment to control if path exists 

  if (!path || strlen(path) == 0) {
    fUserDecay = kFALSE;
    fUserDecayTablePath = "";
  }
  else {
    if (gSystem->AccessPathName(path)) {
      AliErrorStream() << "Attention: This path not exist: " << path << std::endl;
      return kFALSE;
    }
    fUserDecayTablePath = path;
    fUserDecay = kTRUE;
  }
  return kTRUE;
}
