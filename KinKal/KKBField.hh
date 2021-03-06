#ifndef KinKal_KKBField_hh
#define KinKal_KKBField_hh
//
// Class to correct for BField inhomogenity; adjust the parameters for the momentum change
// This effect adds no information content or noise (presently), just transports the parameters 
//
#include "KinKal/KKEff.hh"
#include "KinKal/TDir.hh"
#include <iostream>
#include <stdexcept>
#include <array>
#include <ostream>

namespace KinKal {
  template<class KTRAJ> class KKBField : public KKEff<KTRAJ> {
    public:
      typedef KKEff<KTRAJ> KKEFF;
      typedef PKTraj<KTRAJ> PKTRAJ;
      typedef typename KKEFF::PDATA PDATA; // forward the typedef
      typedef typename KKEFF::WDATA WDATA; // forward the typedef
      typedef KKData<PDATA::PDim()> KKDATA;
      typedef typename KTRAJ::DVEC DVEC; // forward the typedef
      virtual double time() const override { return drange_.mid(); } // apply the correction at the middle of the range
      virtual bool isActive() const override { return active_;}
      virtual void update(PKTRAJ const& ref) override;
      virtual void update(PKTRAJ const& ref, MConfig const& mconfig) override;
      virtual void print(std::ostream& ost=std::cout,int detail=0) const override;
      virtual void process(KKDATA& kkdata,TDir tdir) override;
      virtual void append(PKTRAJ& fit) override;
      DVEC const& effect() const { return bfeff_; }
      virtual ~KKBField(){}
      // create from the domain range, the effect, and the
      KKBField(BField const& bfield, PKTRAJ const& pktraj,TRange const& drange) : 
	bfield_(bfield), drange_(drange), active_(false) {} // not active until updated
    private:
      BField const& bfield_; // bfield
      TRange drange_; // extent of this domain
      Vec3 dpfrac_; // fractional change in momentum for BField diff from nominal over this range
      PDATA bfeff_; // effect of the difference beween the actual BField and bnom integrated over this range
      bool active_; // activity state
  };

  template<class KTRAJ> void KKBField<KTRAJ>::process(KKDATA& kkdata,TDir tdir) {
    if(active_){
      // forwards, set the cache AFTER processing this effect
      if(tdir == TDir::forwards) {
	kkdata.append(bfeff_);
      } else {
	// SUBTRACT the effect going backwards: covariance change is sign-independent
	PDATA reverse(bfeff_);
	reverse.parameters() *= -1.0;
      	kkdata.append(reverse);
      }
    }
    KKEffBase::setStatus(tdir,KKEffBase::processed);
  }

  template<class KTRAJ> void KKBField<KTRAJ>::update(PKTRAJ const& ref) {
    auto const& locref = ref.nearestPiece(drange_.mid()); 
    double time = this->time();
    // translate the momentum change to the parameter change.
    // First get the derivatives and perp basis for the BField x-product at this point
    Vec3 t1hat = locref.direction(time,LocalBasis::perpdir);
    Vec3 t2hat = locref.direction(time,LocalBasis::phidir);
    DVEC dpdt1 = locref.momDeriv(time,LocalBasis::perpdir);
    DVEC dpdt2 = locref.momDeriv(time,LocalBasis::phidir);
    // project the momentum change onto these directions to get the parameter change
    // should add noise due to field measurement and gradientXposition uncertainties FIXME!
    bfeff_.parameters() = dpfrac_.Dot(t1hat)*dpdt1 + dpfrac_.Dot(t2hat)*dpdt2;
//    std::cout << "BF parameters " << bfeff_.parameters() << std::endl;
    KKEffBase::updateStatus();
  }

  template<class KTRAJ> void KKBField<KTRAJ>::update(PKTRAJ const& ref, MConfig const& mconfig) {
    if(mconfig.updatebfcorr_){
      active_ = true;
    // integrate the fractional momentum change
      Vec3 dp;
      bfield_.integrate(ref,drange_,dp);
      dpfrac_ = dp/ref.momentumMag(drange_.mid());
//      std::cout << "Updating iteration " << mconfig.miter_ << " dP " << dp << std::endl;
    }
    update(ref);
  }

  template<class KTRAJ> void KKBField<KTRAJ>::append(PKTRAJ& fit) {
    if(active_){
      // adjust to make sure the piece is appendable
      double time = this->time();
      double tlow = std::max(time,double(fit.back().range().low() + 0.01)); // buffer should be a parameter FIXME!
      TRange newrange(tlow,fit.range().high());
// 1st order effect
      KTRAJ newpiece(fit.back());
//      std::cout << "appending dP = " << bfeff_.parameters() << std::endl;
      newpiece.params() += bfeff_.parameters(); // bfield correction is a dead-reckoning correction
      newpiece.setRange(newrange);
      fit.append(newpiece);
    }
  }

  template<class KTRAJ> void KKBField<KTRAJ>::print(std::ostream& ost,int detail) const {
    ost << "KKBField " << static_cast<KKEff<KTRAJ>const&>(*this);
    ost << " dP fraction " << dpfrac_ << " effect " << bfeff_.parameters() << " domain range " << drange_ << std::endl;
  }

  template <class KTRAJ> std::ostream& operator <<(std::ostream& ost, KKBField<KTRAJ> const& kkmat) {
    kkmat.print(ost,0);
    return ost;
  }

}
#endif
