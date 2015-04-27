#include "PhysicsTools/SelectorUtils/interface/CutApplicatorWithEventContentBase.h"
#include "DataFormats/EgammaCandidates/interface/Photon.h"
#include "RecoEgamma/EgammaTools/interface/EffectiveAreas.h"


class PhoAnyPFIsoWithEACut : public CutApplicatorWithEventContentBase {
public:
  PhoAnyPFIsoWithEACut(const edm::ParameterSet& c);
  
  result_type operator()(const reco::PhotonPtr&) const override final;

  void setConsumes(edm::ConsumesCollector&) override final;
  void getEventContent(const edm::EventBase&) override final;

  CandidateType candidateType() const override final { 
    return PHOTON; 
  }

private:
  // Cut values
  float _anyPFIsoWithEACutValue_C1_EB;
  float _anyPFIsoWithEACutValue_C2_EB;
  float _anyPFIsoWithEACutValue_C1_EE;
  float _anyPFIsoWithEACutValue_C2_EE;
  // Configuration
  float _barrelCutOff;
  bool  _useRelativeIso;
  // Effective area constants
  EffectiveAreas _effectiveAreas;
  // The isolations computed upstream
  edm::Handle<edm::ValueMap<float> > _anyPFIsoMap;
  // The rho
  edm::Handle< double > _rhoHandle;

  constexpr static char anyPFIsoWithEA_[] = "anyPFIsoWithEA";
  constexpr static char rhoString_     [] = "rho";
};

constexpr char PhoAnyPFIsoWithEACut::anyPFIsoWithEA_[];
constexpr char PhoAnyPFIsoWithEACut::rhoString_[];

DEFINE_EDM_PLUGIN(CutApplicatorFactory,
		  PhoAnyPFIsoWithEACut,
		  "PhoAnyPFIsoWithEACut");

PhoAnyPFIsoWithEACut::PhoAnyPFIsoWithEACut(const edm::ParameterSet& c) :
  CutApplicatorWithEventContentBase(c),
  _anyPFIsoWithEACutValue_C1_EB(c.getParameter<double>("anyPFIsoWithEACutValue_C1_EB")),
  _anyPFIsoWithEACutValue_C2_EB(c.getParameter<double>("anyPFIsoWithEACutValue_C2_EB")),
  _anyPFIsoWithEACutValue_C1_EE(c.getParameter<double>("anyPFIsoWithEACutValue_C1_EE")),
  _anyPFIsoWithEACutValue_C2_EE(c.getParameter<double>("anyPFIsoWithEACutValue_C2_EE")),
  _barrelCutOff(c.getParameter<double>("barrelCutOff")),
  _useRelativeIso(c.getParameter<bool>("useRelativeIso")),
  _effectiveAreas( (c.getParameter<edm::FileInPath>("effAreasConfigFile")).fullPath())
{
  
  edm::InputTag maptag = c.getParameter<edm::InputTag>("anyPFIsoMap");
  contentTags_.emplace(anyPFIsoWithEA_,maptag);

  edm::InputTag rhoTag = c.getParameter<edm::InputTag>("rho");
  contentTags_.emplace(rhoString_,rhoTag);

}

void PhoAnyPFIsoWithEACut::setConsumes(edm::ConsumesCollector& cc) {
  auto anyPFIsoWithEA = 
    cc.consumes<edm::ValueMap<float> >(contentTags_[anyPFIsoWithEA_]);
  contentTokens_.emplace(anyPFIsoWithEA_,anyPFIsoWithEA);

  auto rho = cc.consumes<double>(contentTags_[rhoString_]);
  contentTokens_.emplace(rhoString_, rho);
}

void PhoAnyPFIsoWithEACut::getEventContent(const edm::EventBase& ev) {  
  ev.getByLabel(contentTags_[anyPFIsoWithEA_],_anyPFIsoMap);
  ev.getByLabel(contentTags_[rhoString_],_rhoHandle);
}

CutApplicatorBase::result_type 
PhoAnyPFIsoWithEACut::
operator()(const reco::PhotonPtr& cand) const{  

  // Figure out the cut value
  // The value is generally pt-dependent: C1 + pt * C2
  double absEta = std::abs(cand->superCluster()->position().eta());
  const float anyPFIsoWithEACutValue = 
    ( absEta < _barrelCutOff ? 
      _anyPFIsoWithEACutValue_C1_EB + cand->pt() * _anyPFIsoWithEACutValue_C2_EB
      : 
      _anyPFIsoWithEACutValue_C1_EE + cand->pt() * _anyPFIsoWithEACutValue_C2_EE
      );
  
  // Retrieve the variable value for this particle
  float anyPFIso = (*_anyPFIsoMap)[cand];

  // Apply pile-up correction
  double eA = _effectiveAreas.getEffectiveArea( absEta );
  double rho = *_rhoHandle;
  float anyPFIsoWithEA = std::max(0.0, anyPFIso - rho * eA);

  // Divide by pT if the relative isolation is requested
  if( _useRelativeIso )
    anyPFIsoWithEA /= cand->pt();

  // Apply the cut and return the result
  return anyPFIsoWithEA < anyPFIsoWithEACutValue;
}
