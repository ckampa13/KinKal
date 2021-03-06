#ifndef KinKal_WireHit_hh
#define KinKal_WireHit_hh
//
//  class representing a drift wire measurement.  Implemented using TPOCA between the particle traj and the wire
//  Used as part of the kinematic Kalman fit
//
#include "KinKal/THit.hh"
#include "KinKal/D2T.hh"
#include "KinKal/TLine.hh"
#include "KinKal/TPoca.hh"
#include "KinKal/LRAmbig.hh"
#include "KinKal/BField.hh"
#include <stdexcept>
namespace KinKal {
// struct for updating wire hits; this is just parameters, but could be methods as well
  struct WireHitUpdater {
    double mindoca_; // minimum DOCA value to set an ambiguity
    double maxdoca_; // maximum DOCA to still use a hit
    WireHitUpdater(double mindoca,double maxdoca) : mindoca_(mindoca), maxdoca_(maxdoca) {}
  };

  template <class KTRAJ> class WireHit : public THit<KTRAJ> {
    public:
      typedef THit<KTRAJ> THIT;
      typedef PKTraj<KTRAJ> PKTRAJ;
      typedef TPoca<PKTRAJ,TLine> TPOCA;
      typedef Residual<KTRAJ::NParams()> RESIDUAL;
      typedef DXing<KTRAJ> DXING;
      typedef std::shared_ptr<DXING> DXINGPTR;
      typedef typename KTRAJ::DVEC DVEC;
      // THit interface overrrides
      virtual void resid(PKTRAJ const& pktraj, RESIDUAL& resid) const override;
      void resid(TPOCA const& tpoca, RESIDUAL& resid) const; // actual implementation of resid uses TPOCA
      virtual void update(PKTRAJ const& pktraj, MConfig const& config, RESIDUAL& resid) override;
      virtual unsigned nDOF() const override { return 1; }
      double cellSize() const { return csize_; } // approximate transverse cell size, used to set null variance
// construct from a D2T relationship; BField is needed to compute ExB effects
      TLine const& wire() const { return wire_; }
      // set the null variance given the min DOCA used to assign LR ambiguity.  This assumes a flat DOCA distribution
      void setNullVar(double mindoca) { nullvar_ = mindoca*mindoca/3.0; }
      void setAmbig(LRAmbig newambig) { ambig_ = newambig; }
      WireHit(DXINGPTR const& dxing, BField const& bfield, TLine const& wire, D2T const& d2t, double csize,LRAmbig ambig=LRAmbig::null) : 
	THIT(dxing,true), wire_(wire), d2t_(d2t), csize_(csize), ambig_(ambig), bfield_(bfield) { setNullVar(csize_); }
      virtual ~WireHit(){}
      LRAmbig ambig() const { return ambig_; }
      D2T const& d2T() const { return d2t_; }
    private:
      TLine wire_; // local linear approximation to the wire of this hit.  The range describes the active wire length
      D2T const& d2t_; // distance to time relationship for drift in this cell
      double csize_; // transverse cell size in mm
      double nullvar_; // variance of the error in space for null ambiguity
      LRAmbig ambig_; // current ambiguity assignment: can change during a fit
      BField const& bfield_;
  };

  template <class KTRAJ> void WireHit<KTRAJ>::resid(PKTRAJ const& pktraj, RESIDUAL& residual) const {
    // compute TPOCA.  wire hit measurement time is too crude to provide a good hint
    TPOCA tpoca(pktraj,wire_);
    resid(tpoca,residual);
  }

  template <class KTRAJ> void WireHit<KTRAJ>::update(PKTRAJ const& pktraj, MConfig const& mconfig, RESIDUAL& residual ) {
    // find TPOCA
    TPOCA tpoca(pktraj,wire());
    // find the wire hit updater in the update params.  If there are more than 1 throw 
    const WireHitUpdater* whupdater(0);
    for(auto const& uparams : mconfig.hitupdaters_){
      auto const* whu = std::any_cast<WireHitUpdater>(&uparams);
      if(whu != 0){
	if(whupdater !=0) throw std::invalid_argument("Multiple WireHitUpdaters found");
	whupdater = whu;
      }
    }
    if(whupdater != 0){
        // use DOCA to set the ambiguity
      if(fabs(tpoca.doca()) > whupdater->mindoca_){
	LRAmbig newambig = tpoca.doca() > 0.0 ? LRAmbig::right : LRAmbig::left;
	setAmbig(newambig);
      } else {
	setAmbig(LRAmbig::null);
	setNullVar(std::min(cellSize(),whupdater->mindoca_));
      }
      // decide if the hit is consistent with this track, and if not disable/enable it.
      // for now, just look at DOCA, but could use tension too TODO!
      THIT::setActivity(fabs(tpoca.doca()) < whupdater->maxdoca_);
    } // allow no updater: hits may be frozen this meta-iteration
    // compute the residual
    resid(tpoca,residual);
  }

  template <class KTRAJ> void WireHit<KTRAJ>::resid(TPOCA const& tpoca, RESIDUAL& resid) const {
    if(tpoca.usable()){
      // translate TPOCA to residual
      if(ambig_ != LRAmbig::null){ 
	auto iambig = static_cast<std::underlying_type<LRAmbig>::type>(ambig_);
	// convert DOCA to wire-local polar coordinates.  This defines azimuth WRT the B field for ExB effects
	double rho = tpoca.doca()*iambig; // this is allowed to go negative
	Vec3 bvec = bfield_.fieldVect(tpoca.particlePoca().Vect());
	auto pdir = bvec.Cross(wire_.dir()).Unit(); // direction perp to wire and BField
	Vec3 dvec = tpoca.delta().Vect();
	double phi = asin(double(dvec.Unit().Dot(pdir)));
	Pol2 drift(rho, phi);
	double tdrift, tdvar, vdrift;
	d2T().distanceToTime(drift, tdrift, tdvar, vdrift);
	// residual is in time, so unit dependendence on time, distance dependence is the local drift velocity
	DVEC dRdP = tpoca.dDdP()*iambig/vdrift - tpoca.dTdP(); 
	resid = RESIDUAL(RESIDUAL::dtime,tpoca,tpoca.deltaT()-tdrift,tdvar,dRdP);
      } else {
	// interpret DOCA against the wire directly as the residual.  There is no direct time dependence in this case
	// residual is in space, so unit dependendence on distance, none on time
	resid = RESIDUAL(RESIDUAL::distance,tpoca,-tpoca.doca(),nullvar_,tpoca.dDdP());
      }
    } else
      throw std::runtime_error("POCA failure");
  }

}
#endif
