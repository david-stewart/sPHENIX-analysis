

// Modified version of JetValidation.cc for the purpose of subjet analysis -- Jennifer James jennifer.l.james@vanderbilt.edu
//module for producing a TTree with jet information for doing jet validation studies
// for questions/bugs please contact Virginia Bailey vbailey13@gsu.edu and myself

#include <fun4all/Fun4AllBase.h>
#include <EMJetVal.h>
#include <fun4all/Fun4AllReturnCodes.h>
#include <fun4all/PHTFileServer.h>
#include <phool/PHCompositeNode.h>
#include <phool/getClass.h>
#include <jetbase/JetMap.h>
#include <jetbase/Jetv1.h>

#include <centrality/CentralityInfo.h>

#include <calobase/RawTower.h>
#include <calobase/RawTowerContainer.h>
#include <calobase/RawTowerGeom.h>
#include <calobase/RawTowerGeomContainer.h>
#include <calobase/TowerInfoContainer.h>
#include <calobase/TowerInfo.h>

#include <Pythia8/Pythia.h> // Include the Pythia header
#include <jetbackground/TowerBackground.h>

#include <iostream>
#include <fastjet/PseudoJet.hh>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <vector>
#include "fastjet/ClusterSequence.hh"
#include "fastjet/contrib/SoftDrop.hh" // In external code, this should be fastjet/contrib/SoftDrop.hh

using namespace fastjet;

// ROOT, for histogramming. 
#include "TH1.h"
#include "TH2.h"

// ROOT, for interactive graphics.
#include "TVirtualPad.h"
#include "TApplication.h"

// ROOT, for saving file.
#include "TFile.h"
#include <TTree.h>

using std::cout;
using std::endl;

//____________________________________________________________________________..
EMJetVal::EMJetVal(const std::string& recojetname, const std::string& truthjetname, const std::string& outputfilename)
  :SubsysReco("EMJetVal_" + recojetname + "_" + truthjetname)
  , m_recoJetName(recojetname)
  , m_truthJetName(truthjetname)
  , m_outputFileName(outputfilename)
  , m_etaRange(-1, 1)
  , m_ptRange(5, 100)
  , m_doTruthJets(0)
  , m_doSeeds(0)
  , m_doUnsubJet(0)
  , m_T(nullptr)
  , m_event(-1)
  , m_nTruthJet(-1)
  , m_nJet(-1)
  , m_id()
  , m_nComponent()
  , m_eta()
  , m_phi()
  , m_e()
  , m_pt()
  , m_sub_et()
  , m_truthID()
  , m_truthNComponent()
  , m_truthEta()
  , m_truthPhi()
  , m_truthE()
  , m_truthPt()
  , m_eta_rawseed()
  , m_phi_rawseed()
  , m_pt_rawseed()
  , m_e_rawseed()
  , m_rawseed_cut()
  , m_eta_subseed()
  , m_phi_subseed()
  , m_pt_subseed()
  , m_e_subseed()
  , m_subseed_cut()
{
  std::cout << "EMJetVal::EMJetVal(const std::string &name) Calling ctor" << std::endl;
}


//____________________________________________________________________________..
EMJetVal::~EMJetVal()
{
  std::cout << "EMJetVal::~EMJetVal() Calling dtor" << std::endl;
}

//____________________________________________________________________________..
int EMJetVal::Init(PHCompositeNode *topNode)
{
  std::cout << "EMJetVal::Init(PHCompositeNode *topNode) Initializing" << std::endl;
  PHTFileServer::get().open(m_outputFileName, "RECREATE");
  std::cout << "EMJetVal::Init - Output to " << m_outputFileName << std::endl;
  //Analysis hists
  outFile = new TFile("hist_jets.root", "RECREATE");

  // configure Tree
  m_T = new TTree("T", "MyJetAnalysis Tree");
  m_T->Branch("m_event", &m_event, "event/I");
  m_T->Branch("nJet", &m_nJet, "nJet/I");
  m_T->Branch("cent", &m_centrality);
  m_T->Branch("b", &m_impactparam);
  m_T->Branch("id", &m_id);
  m_T->Branch("nComponent", &m_nComponent);

  m_T->Branch("eta", &m_eta);
  m_T->Branch("phi", &m_phi);
  m_T->Branch("e", &m_e);
  m_T->Branch("pt", &m_pt);
  if(m_doUnsubJet)
    {
      m_T->Branch("pt_unsub", &m_unsub_pt);
      m_T->Branch("subtracted_et", &m_sub_et);
    }
  if(m_doTruthJets){
    m_T->Branch("nTruthJet", &m_nTruthJet);
    m_T->Branch("truthID", &m_truthID);
    m_T->Branch("truthNComponent", &m_truthNComponent);
    m_T->Branch("truthEta", &m_truthEta);
    m_T->Branch("truthPhi", &m_truthPhi);
    m_T->Branch("truthE", &m_truthE);
    m_T->Branch("truthPt", &m_truthPt);
  }

  if(m_doSeeds){
    m_T->Branch("rawseedEta", &m_eta_rawseed);
    m_T->Branch("rawseedPhi", &m_phi_rawseed);
    m_T->Branch("rawseedPt", &m_pt_rawseed);
    m_T->Branch("rawseedE", &m_e_rawseed);
    m_T->Branch("rawseedCut", &m_rawseed_cut);
    m_T->Branch("subseedEta", &m_eta_subseed);
    m_T->Branch("subseedPhi", &m_phi_subseed);
    m_T->Branch("subseedPt", &m_pt_subseed);
    m_T->Branch("subseedE", &m_e_subseed);
    m_T->Branch("subseedCut", &m_subseed_cut);
  }

  return Fun4AllReturnCodes::EVENT_OK;
}
//____________________________________________________________________________..

int EMJetVal::retrieveEvent(const fastjet::PseudoJet& jet) {
  // Add the PseudoJet to the event vector
  eventVector.push_back(jet);
    
  return 0; // Return 0 for success
}

//____________________________________________________________________________..
int EMJetVal::InitRun(PHCompositeNode *topNode)
{
  std::cout << "EMJetVal::InitRun(PHCompositeNode *topNode) Initializing for Run XXX" << std::endl;
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int EMJetVal::process_event(PHCompositeNode *topNode)
{
  //std::cout << "EMJetVal::process_event(PHCompositeNode *topNode) Processing Event" << std::endl;
  ++m_event;

  // interface to reco jets
  JetMap* jets = findNode::getClass<JetMap>(topNode, m_recoJetName);
  if (!jets)
    {
      std::cout
	<< "MyJetAnalysis::process_event - Error can not find DST Reco JetMap node "
	<< m_recoJetName << std::endl;
      exit(-1);
    }

  //interface to truth jets
  JetMap* jetsMC = findNode::getClass<JetMap>(topNode, m_truthJetName);
  if (!jetsMC && m_doTruthJets)
    {
      std::cout
	<< "MyJetAnalysis::process_event - Error can not find DST Truth JetMap node "
	<< m_truthJetName << std::endl;
      exit(-1);
    }
  
  // interface to jet seeds
  JetMap* seedjetsraw = findNode::getClass<JetMap>(topNode, "AntiKt_TowerInfo_HIRecoSeedsRaw_r02");
  if (!seedjetsraw && m_doSeeds)
    {
      std::cout
	<< "MyJetAnalysis::process_event - Error can not find DST raw seed jets "
	<< std::endl;
      exit(-1);
    }

  JetMap* seedjetssub = findNode::getClass<JetMap>(topNode, "AntiKt_TowerInfo_HIRecoSeedsSub_r02");
  if (!seedjetssub && m_doSeeds)
    {
      std::cout
	<< "MyJetAnalysis::process_event - Error can not find DST subtracted seed jets "
	<< std::endl;
      exit(-1);
    }

  //centrality
  CentralityInfo* cent_node = findNode::getClass<CentralityInfo>(topNode, "CentralityInfo");
  if (!cent_node)
    {
      std::cout
	<< "MyJetAnalysis::process_event - Error can not find centrality node "
	<< std::endl;
      exit(-1);
    }
  
  //calorimeter towers
  TowerInfoContainer *towersEM3 = findNode::getClass<TowerInfoContainer>(topNode, "TOWERINFO_CALIB_CEMC_RETOWER");
  TowerInfoContainer *towersIH3 = findNode::getClass<TowerInfoContainer>(topNode, "TOWERINFO_CALIB_HCALIN");
  TowerInfoContainer *towersOH3 = findNode::getClass<TowerInfoContainer>(topNode, "TOWERINFO_CALIB_HCALOUT");
  RawTowerGeomContainer *tower_geom = findNode::getClass<RawTowerGeomContainer>(topNode, "TOWERGEOM_HCALIN");
  RawTowerGeomContainer *tower_geomOH = findNode::getClass<RawTowerGeomContainer>(topNode, "TOWERGEOM_HCALOUT");
  if(!towersEM3 || !towersIH3 || !towersOH3){
    std::cout
      <<"MyJetAnalysis::process_event - Error can not find raw tower node "
      << std::endl;
    exit(-1);
  }

  if(!tower_geom || !tower_geomOH){
    std::cout
      <<"MyJetAnalysis::process_event - Error can not find raw tower geometry "
      << std::endl;
    exit(-1);
  }

  //underlying event
  TowerBackground *background = findNode::getClass<TowerBackground>(topNode, "TowerInfoBackground_Sub2");
  if(!background){
    std::cout<<"Can't get background. Exiting"<<std::endl;
    return Fun4AllReturnCodes::EVENT_OK;
  }

  //get the event centrality/impact parameter from HIJING
  m_centrality =  cent_node->get_centile(CentralityInfo::PROP::bimp);
  m_impactparam =  cent_node->get_quantity(CentralityInfo::PROP::bimp);
  
  //get reco jets
  m_nJet = 0;
  float background_v2 = 0;
  float background_Psi2 = 0;
  if(m_doUnsubJet)
    {
      background_v2 = background->get_v2();
      background_Psi2 = background->get_Psi2();
    }
  for (JetMap::Iter iter = jets->begin(); iter != jets->end(); ++iter)
    {

      Jet* jet = iter->second;
      if(jet->get_pt() < 1) continue; // to remove noise jets

      m_id.push_back(jet->get_id());
      m_nComponent.push_back(jet->size_comp());
      m_eta.push_back(jet->get_eta());
      m_phi.push_back(jet->get_phi());
      m_e.push_back(jet->get_e());
      m_pt.push_back(jet->get_pt());
      // Assuming 'jets' is a JetMap::Iter

      //      vector<PseudoJet> pseudoJets;  // Create a vector to store PseudoJet objects

      // Convert 'jets' to a vector of PseudoJet
      //      pseudoJets.push_back(iter->second);
      

      // Now, you can iterate over 'pseudoJets' using a vector iterator
      // for (vector<PseudoJet>::iterator iter = pseudoJets.begin(); iter != pseudoJets.end(); ++iter) {
      //	EMJetVal::retrieveEvent(*iter);
	// }

      // Assuming 'm_eta', 'm_phi', 'm_e', and 'm_pt' are vectors with event data
      for (std::vector<float>::size_type i = 0; i < m_eta.size(); ++i) {
	double eta = m_eta[i];
	double phi = m_phi[i];
	double e = m_e[i];
	double pt = m_pt[i];

	// Create a PseudoJet and add it to the 'tower' vector
	PseudoJet particles(pt * cos(phi), pt * sin(phi), pt * sinh(eta), e);
	//	vector<PseudoJet> particles;

	//	particles.push_back(iter->second);
      }

      //  return 0; // Return 0 for success

      std::vector<PseudoJet> particles;
      
      if(m_doUnsubJet)
	{
	  //  Jet* unsubjet = new Jetv1();
	  float totalPx = 0;
	  float totalPy = 0;
	  float totalPz = 0;
	  float totalE = 0;
	  int nconst = 0;

	  for (Jet::ConstIter comp = jet->begin_comp(); comp != jet->end_comp(); ++comp)
	    {
	      particles.clear();
	      TowerInfo *tower;
	      nconst++;
	      unsigned int channel = (*comp).second;
	      
	      if ((*comp).first == 15 ||  (*comp).first == 30)
		{
		  tower = towersIH3->get_tower_at_channel(channel);
		  if(!tower || !tower_geom){
		    continue;
		  }
		  unsigned int calokey = towersIH3->encode_key(channel);
		  int ieta = towersIH3->getTowerEtaBin(calokey);
		  int iphi = towersIH3->getTowerPhiBin(calokey);
		  const RawTowerDefs::keytype key = RawTowerDefs::encode_towerid(RawTowerDefs::CalorimeterId::HCALIN, ieta, iphi);
		  float UE = background->get_UE(1).at(ieta);
		  float tower_phi = tower_geom->get_tower_geometry(key)->get_phi();
		  float tower_eta = tower_geom->get_tower_geometry(key)->get_eta();

		  UE = UE * (1 + 2 * background_v2 * cos(2 * (tower_phi - background_Psi2)));
		  totalE += tower->get_energy() + UE;
		  double pt = tower->get_energy() / cosh(tower_eta);
		  totalPx += pt * cos(tower_phi);
		  totalPy += pt * sin(tower_phi);
		  totalPz += pt * sinh(tower_eta);
		}
	      else if ((*comp).first == 16 || (*comp).first == 31)
		{
		  tower = towersOH3->get_tower_at_channel(channel);
		  if(!tower || !tower_geomOH)
		    {
		      continue;
		    }
		  
		  unsigned int calokey = towersOH3->encode_key(channel);
		  int ieta = towersOH3->getTowerEtaBin(calokey);
		  int iphi = towersOH3->getTowerPhiBin(calokey);
		  const RawTowerDefs::keytype key = RawTowerDefs::encode_towerid(RawTowerDefs::CalorimeterId::HCALOUT, ieta, iphi);
		  float UE = background->get_UE(2).at(ieta);
		  float tower_phi = tower_geomOH->get_tower_geometry(key)->get_phi();
		  float tower_eta = tower_geomOH->get_tower_geometry(key)->get_eta();

		  UE = UE * (1 + 2 * background_v2 * cos(2 * (tower_phi - background_Psi2)));
		  totalE +=tower->get_energy() + UE;
		  double pt = tower->get_energy() / cosh(tower_eta);
		  totalPx += pt * cos(tower_phi);
		  totalPy += pt * sin(tower_phi);
		  totalPz += pt * sinh(tower_eta);
		}
	      else if ((*comp).first == 14 || (*comp).first == 29)
		{
		  tower = towersEM3->get_tower_at_channel(channel);
		  if(!tower || !tower_geom)
		    {
		      continue;
		    }

		  unsigned int calokey = towersEM3->encode_key(channel);
		  int ieta = towersEM3->getTowerEtaBin(calokey);
		  int iphi = towersEM3->getTowerPhiBin(calokey);
		  const RawTowerDefs::keytype key = RawTowerDefs::encode_towerid(RawTowerDefs::CalorimeterId::HCALIN, ieta, iphi);
		  float UE = background->get_UE(0).at(ieta);
		  float tower_phi = tower_geom->get_tower_geometry(key)->get_phi();
		  float tower_eta = tower_geom->get_tower_geometry(key)->get_eta();


		  UE = UE * (1 + 2 * background_v2 * cos(2 * (tower_phi - background_Psi2)));
		  double ETmp = tower->get_energy() + UE;
		  totalE += ETmp;
		  double pt = tower->get_energy() / cosh(tower_eta);
		  double pxTmp = pt * cos(tower_phi);
		  totalPx += pxTmp;
		  double pyTmp = pt * sin(tower_phi);
		  totalPy += pyTmp;
		  double pzTmp = sinh(tower_eta);
		  totalPz += pzTmp;		      

		  particles.push_back( PseudoJet( pxTmp, pyTmp, pzTmp, ETmp) );
		  //
		  // vector<PseudoJet> tower;

		  // EMJetVal emJetVal; // Create an instance of the EMJetVal class
		  // Retrieve the event data using the retrieveEvent function
		  // int retrieveStatus = emJetVal.retrieveEvent(tower);
		  //  std::cout << "passed here -389 -inserted my kinematics" << std::endl
		  // Detector size, anti-kT radius, and modified mass-drop tagger z.
		  //	  double etaMax = 1.;
		  double radius[5] = {0.05, 0.1, 0.2, 0.4, 0.6}; // jet radius
		  double pseudorapidity = -999; // pseudorapidity
		  double theta_sj = -1; // delta radius (value describes an unachievable value)
		  double z_sj = -1; // delta radius (value describes an unachievable value)
		   
		  //  std::cout << "passed here -397 -my jet defs" << std::endl
		  // Set up FastJet jet finders and modified mass-drop tagger.
		  JetDefinition jetDefAKT_R01( antikt_algorithm, radius[1]);
		  JetDefinition jetDefAKT_R04( antikt_algorithm, radius[3]);
		  JetDefinition jetDefCA(cambridge_algorithm, radius[3]);

		  //  vector<PseudoJet> particles;

		  // Run Fastjet anti-kT algorithm and sort jets in pT order.
		  ClusterSequence clustSeq_R04( particles, jetDefAKT_R04 );
		  std::vector<PseudoJet> sortedJets_R04 = sorted_by_pt( clustSeq_R04.inclusive_jets() );
    
		  //! create a loop to run over the jets -
		  
		  for (std::vector<float>::size_type j = 0; j < sortedJets_R04.size(); ++j) {
		    PseudoJet jet = sortedJets_R04.at(j);
		    if(fabs(jet.eta()) > 0.6)
		      continue;
      
		    ClusterSequence clustSeq_R01_con(jet.constituents() , jetDefAKT_R01 );
		    std:: vector<PseudoJet> sortedJets_R01_con = sorted_by_pt( clustSeq_R01_con.inclusive_jets() );
		    // std::cout << "passed here -476 -pseudojet" << std::endl
		    if (sortedJets_R01_con.size() < 2)
		      continue;
		    PseudoJet sj1 = sortedJets_R01_con.at(0);
		    PseudoJet sj2 = sortedJets_R01_con.at(1);

		    theta_sj = sj1.delta_R(sj2);
		    z_sj = sj2.pt()/(sj2.pt()+sj1.pt());
      
		    // 15 to 20
		    if (jet.pt() > 15 and jet.pt() < 20 ){
		      // cout<<"sorted jets at "<<j<<" the pT = "<<jet.pt()<<endl;
		      _hjetpT_R04->Fill(jet.perp());
		      pseudorapidity = jet.eta();
		      _hjeteta_R04->Fill(pseudorapidity);
		      _hmult_R04->Fill(jet.constituents().size());

		      ClusterSequence clustSeqCA(jet.constituents(), jetDefCA);
		      std:: vector<PseudoJet> cambridgeJets = sorted_by_pt(clustSeqCA.inclusive_jets());
    
		      // SoftDrop parameters
		      double z_cut = 0.20;
		      double beta = 0.0;
		      contrib::SoftDrop sd(beta, z_cut);
		      //! get subjets 15 to 20 pT
		      
		      _h_R04_z_sj_15_20->Fill(z_sj);
		      _h_R04_theta_sj_15_20->Fill(theta_sj);

		      _hmult_R04_pT_15_20GeV->Fill(jet.constituents().size());

		      PseudoJet sd_jet = sd(jet);
		      if (sd_jet == 0)
			continue;
		      // SoftDrop was successful, analyze the resulting sd_jet
		      // cout << endl;
		      // cout << "original    jet: " << cambridgeJet << endl;
		      // cout << "SoftDropped jet: " << sd_jet << endl;
		      cout << "jet pT: " << sd_jet.perp() << endl;
		      cout << "  delta_R between subjets: " << sd_jet.structure_of<contrib::SoftDrop>().delta_R() << endl;
		      cout << "  symmetry measure(z):     " << sd_jet.structure_of<contrib::SoftDrop>().symmetry() << endl;
		      cout << " theta_sj: " << theta_sj << endl;
		      cout << " z_sj: " << z_sj << endl;
		      // cout << "  mass drop(mu):           " << sd_jet.structure_of<contrib::SoftDrop>().mu() << endl;

		      // Fill histograms with delta_R and symmetry measure z
		      double delta_R_subjets = sd_jet.structure_of<contrib::SoftDrop>().delta_R();
		      double z_subjets = sd_jet.structure_of<contrib::SoftDrop>().symmetry();

		      _h_R04_z_g_15_20->Fill(z_subjets);
		      _h_R04_theta_g_15_20->Fill(delta_R_subjets);

		      correlation_theta_15_20->Fill(delta_R_subjets, theta_sj);
		      correlation_z_15_20->Fill(z_subjets, z_sj);
		          
		      // SoftDrop failed, handle the case as needed
		      // e.g., skip this jet or perform alternative analysis
		            
		    }
 
		    // 20 to 25 pT

		    else if (jet.pt() > 20 and jet.pt() < 25 ){
		      // cout<<"sorted jets at "<<j<<" the pT = "<<jet.pt()<<endl;
		      _hjetpT_R04->Fill(jet.perp());
		      pseudorapidity = jet.eta();
		      _hjeteta_R04->Fill(pseudorapidity);
		      _hmult_R04->Fill(jet.constituents().size());
		      
		      ClusterSequence clustSeqCA(jet.constituents(), jetDefCA);
		      std::vector<PseudoJet> cambridgeJets = sorted_by_pt(clustSeqCA.inclusive_jets());

		      // SoftDrop parameters
		      double z_cut = 0.20;
		      double beta = 0.0;
		      contrib::SoftDrop sd(beta, z_cut);
		      //! get subjets 20 to 25 pT
		      _h_R04_z_sj_20_25->Fill(z_sj);
		      _h_R04_theta_sj_20_25->Fill(theta_sj);

		      // Apply SoftDrop to the jet
		      PseudoJet sd_jet = sd(jet);
		      if (sd_jet == 0)
			continue;
		      cout << "jet pT: " << sd_jet.perp() << endl;
		      cout << "  delta_R between subjets: " << sd_jet.structure_of<contrib::SoftDrop>().delta_R() << endl;
		      cout << "  symmetry measure(z):     " << sd_jet.structure_of<contrib::SoftDrop>().symmetry() << endl;
		      cout << " theta_sj: " << theta_sj << endl;
		      cout << " z_sj: " << z_sj << endl;
		      // Fill histograms with delta_R and symmetry measure z
		      double delta_R_subjets = sd_jet.structure_of<contrib::SoftDrop>().delta_R();
		      double z_subjets = sd_jet.structure_of<contrib::SoftDrop>().symmetry();

		      _h_R04_z_g_20_25->Fill(z_subjets);
		      _h_R04_theta_g_20_25->Fill(delta_R_subjets);

		      correlation_theta_20_25->Fill(delta_R_subjets, theta_sj);
		      correlation_z_20_25->Fill(z_subjets, z_sj);
		          
		      // SoftDrop failed, handle the case as needed
		      // e.g., skip this jet or perform alternative analysis
		    } else {
		      _hmult_R04_pT_20_25GeV->Fill(jet.constituents().size());
		    }
        
		    // 25 to 30 pT

		    if (jet.pt() > 25 and jet.pt() < 30 ){
		      // cout<<"sorted jets at "<<j<<" the pT = "<<jet.pt()<<endl;
		      _hjetpT_R04->Fill(jet.perp());
		      pseudorapidity = jet.eta();
		      _hjeteta_R04->Fill(pseudorapidity);
		      _hmult_R04->Fill(jet.constituents().size());
		      
		      ClusterSequence clustSeqCA(jet.constituents(), jetDefCA);
		      std::vector<PseudoJet> cambridgeJets = sorted_by_pt(clustSeqCA.inclusive_jets());


		      // SoftDrop parameters
		      double z_cut = 0.20;
		      double beta = 0.0;
		      contrib::SoftDrop sd(beta, z_cut);
		      //! get subjets
      
		      _h_R04_z_sj_25_30->Fill(z_sj);
		      _h_R04_theta_sj_25_30->Fill(theta_sj);
		      
		      PseudoJet sd_jet = sd(jet);
		      if (sd_jet == 0)
			continue;
		      
		      cout << "jet pT: " << sd_jet.perp() << endl;
		      cout << "  delta_R between subjets: " << sd_jet.structure_of<contrib::SoftDrop>().delta_R() << endl;
		      cout << "  symmetry measure(z):     " << sd_jet.structure_of<contrib::SoftDrop>().symmetry() << endl;
		      cout << " theta_sj: " << theta_sj << endl;
		      cout << " z_sj: " << z_sj << endl;
		      // Fill histograms with delta_R and symmetry measure z
		      double delta_R_subjets = sd_jet.structure_of<contrib::SoftDrop>().delta_R();
		      double z_subjets = sd_jet.structure_of<contrib::SoftDrop>().symmetry();
		        
		      _h_R04_z_g_25_30->Fill(z_subjets);
		      _h_R04_theta_g_25_30->Fill(delta_R_subjets);

		      correlation_theta_25_30->Fill(delta_R_subjets, theta_sj);
		      correlation_z_25_30->Fill(z_subjets, z_sj);
		          
		      // SoftDrop failed, handle the case as needed
		      // e.g., skip this jet or perform alternative analysis
		      
		    } else {
		      _hmult_R04_pT_25_30GeV->Fill(jet.constituents().size());
		    }

		    // 30 to 40 pT

		    if (jet.pt() > 30 and jet.pt() < 40 ){
		      // cout<<"sorted jets at "<<j<<" the pT = "<<jet.pt()<<endl;
		      _hjetpT_R04->Fill(jet.perp());
		      pseudorapidity = jet.eta();
		      _hjeteta_R04->Fill(pseudorapidity);
		      _hmult_R04->Fill(jet.constituents().size());

		      ClusterSequence clustSeqCA(jet.constituents(), jetDefCA);
		      std::vector<PseudoJet> cambridgeJets = sorted_by_pt(clustSeqCA.inclusive_jets());

		      // SoftDrop parameters
		      double z_cut = 0.20;
		      double beta = 0.0;
		      contrib::SoftDrop sd(beta, z_cut);
		      //! get subjets
		      _h_R04_z_sj_30_40->Fill(z_sj);
		      _h_R04_theta_sj_30_40->Fill(theta_sj);
		      
		      // Apply SoftDrop to the jet
		      PseudoJet sd_jet = sd(jet);
		      if (sd_jet == 0)
			continue;
		      cout << "jet pT: " << sd_jet.perp() << endl;
		      cout << "  delta_R between subjets: " << sd_jet.structure_of<contrib::SoftDrop>().delta_R() << endl;
		      cout << "  symmetry measure(z):     " << sd_jet.structure_of<contrib::SoftDrop>().symmetry() << endl;
		      cout << " theta_sj: " << theta_sj << endl;
		      cout << " z_sj: " << z_sj << endl;
		      double delta_R_subjets = sd_jet.structure_of<contrib::SoftDrop>().delta_R();
		      double z_subjets = sd_jet.structure_of<contrib::SoftDrop>().symmetry();

		      _h_R04_z_g_30_40->Fill(z_subjets);
		      _h_R04_theta_g_30_40->Fill(delta_R_subjets);

		      correlation_theta_30_40->Fill(delta_R_subjets, theta_sj);
		      correlation_z_30_40->Fill(z_subjets, z_sj);
		          
		      // SoftDrop failed, handle the case as needed
		      // e.g., skip this jet or perform alternative analysis
		    } else {
		      _hmult_R04_pT_30_40GeV->Fill(jet.constituents().size());
		    }
    
		    //! jet loop
		    // Rjet = 0.4 End

		    //filled nEvent in histogram
		    _hmult_R04->Fill(m_nJet);
    
		    //   std::// cout << "iZ = " << nEvent << std::endl;
    
		  }//! event loop ends for pT

	   
		  // End of event loop. Statistics. Histograms. Done.
		}
	      //return 0;
	    }
	}
    }
  //fill the tree
  m_T->Fill();

  return Fun4AllReturnCodes::EVENT_OK;
}

  /*
  //get unsubtracted jet
  unsubjet->set_px(totalPx);
  unsubjet->set_py(totalPy);
  unsubjet->set_pz(totalPz);
  unsubjet->set_e(totalE);
  m_unsub_pt.push_back(unsubjet->get_pt());
  m_sub_et.push_back(unsubjet->get_et() - jet->get_et());*/

  
  /*  //get truth jets
      if(m_doTruthJets)
      {
      m_nTruthJet = 0;
      for (JetMap::Iter iter = jetsMC->begin(); iter != jetsMC->end(); ++iter)
      {
      Jet* truthjet = iter->second;

      bool eta_cut = (truthjet->get_eta() >= m_etaRange.first) and (truthjet->get_eta() <= m_etaRange.second);
      bool pt_cut = (truthjet->get_pt() >= m_ptRange.first) and (truthjet->get_pt() <= m_ptRange.second);
      if ((not eta_cut) or (not pt_cut)) continue;
      m_truthID.push_back(truthjet->get_id());
      m_truthNComponent.push_back(truthjet->size_comp());
      m_truthEta.push_back(truthjet->get_eta());
      m_truthPhi.push_back(truthjet->get_phi());
      m_truthE.push_back(truthjet->get_e());
      m_truthPt.push_back(truthjet->get_pt());
      m_nTruthJet++;
      }
      }
  */
  //return Fun4AllReturnCodes::EVENT_OK;


  //____________________________________________________________________________..
  int EMJetVal::ResetEvent(PHCompositeNode *topNode)
  {
    //std::cout << "EMJetVal::ResetEvent(PHCompositeNode *topNode) Resetting internal structures, prepare for next event" << std::endl;
    m_id.clear();
    m_nComponent.clear();
    m_eta.clear();
    m_phi.clear();
    m_e.clear();
    m_pt.clear();
    m_unsub_pt.clear();
    m_sub_et.clear();
  

  return Fun4AllReturnCodes::EVENT_OK;

  }
  //____________________________________________________________________________..
  int EMJetVal::EndRun(const int runnumber)
  {
    std::cout << "EMJetVal::EndRun(const int runnumber) Ending Run for Run " << runnumber << std::endl;
    return Fun4AllReturnCodes::EVENT_OK;
  }

  //____________________________________________________________________________..
  int EMJetVal::End(PHCompositeNode *topNode)
  {
    std::cout << "EMJetVal::End - Output to " << m_outputFileName << std::endl;
    PHTFileServer::get().cd(m_outputFileName);

    m_T->Write();
    std::cout << "EMJetVal::End(PHCompositeNode *topNode) This is the End..." << std::endl;
    return Fun4AllReturnCodes::EVENT_OK;
  }

  //____________________________________________________________________________..
  int EMJetVal::Reset(PHCompositeNode *topNode)
  {
    std::cout << "EMJetVal::Reset(PHCompositeNode *topNode) being Reset" << std::endl;
    return Fun4AllReturnCodes::EVENT_OK;
  }

  //____________________________________________________________________________..
  void EMJetVal::Print(const std::string &what) const
  {
    std::cout << "EMJetVal::Print(const std::string &what) const Printing info for " << what << std::endl;
  }
    

