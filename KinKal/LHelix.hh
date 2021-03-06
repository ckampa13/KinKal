#ifndef KinKal_LHelix_hh
#define KinKal_LHelix_hh
//
// class desribing the looping helix basis for the kinematic Kalman fit
// It provides geometric, kinematic, and algebraic representation of
// a particle executing a multi-loop helix in a constant magnetic field.
// Original Author David Brown (LBNL) 1/2020
//

#include "KinKal/Vectors.hh"
#include "KinKal/TRange.hh"
#include "KinKal/PData.hh"
#include "KinKal/LocalBasis.hh"
#include "KinKal/BField.hh"
#include "CLHEP/Units/PhysicalConstants.h"
#include "Math/Rotation3D.h"
#include <vector>
#include <string>
#include <ostream>

namespace KinKal {

  class LHelix {
    public:
      // This class must provide the following to be used to instantiate the 
      // classes implementing the Kalman fit
      // define the indices and names of the parameters
      enum ParamIndex {rad_=0,lam_=1,cx_=2,cy_=3,phi0_=4,t0_=5,npars_=6};
      constexpr static ParamIndex t0Index() { return t0_; }
      constexpr static size_t NParams() { return npars_; }
      typedef PData<npars_> PDATA; // Data payload for this class
      typedef typename PDATA::DVEC DVEC; // derivative of parameters type
      static std::vector<std::string> const& paramNames(); 
      static std::vector<std::string> const& paramUnits(); 
      static std::vector<std::string> const& paramTitles();
      static std::string const& paramName(ParamIndex index);
      static std::string const& paramUnit(ParamIndex index);
      static std::string const& paramTitle(ParamIndex index);
      static std::string const& trajName();

      // interface needed for KKTrk instantiation
      // construct from momentum, position, and particle properties.
      // This also requires the nominal BField, which can be a vector (3d) or a scalar (B along z)
      LHelix(Vec4 const& pos, Mom4 const& mom, int charge, Vec3 const& bnom, TRange const& range=TRange());
      LHelix(Vec4 const& pos, Mom4 const& mom, int charge, double bnom, TRange const& range=TRange());
      // copy and override the parameters  
      LHelix(PDATA const& pdata, LHelix const& other);
      Vec4 pos4(double time) const;
      void position(Vec4& pos) const; // time of pos is input 
      Vec3 position(double time) const;
      Vec3 velocity(double time) const;
      double speed(double time) const  {  return CLHEP::c_light*beta(); }
      void rangeInTolerance(TRange& range, BField const& bfield, double tol) const;
      void print(std::ostream& ost, int detail) const;
      TRange const& range() const { return trange_; }
      TRange& range() { return trange_; }
      void setRange(TRange const& trange) { trange_ = trange; }
      bool inRange(double time) const { return trange_.inRange(time); }
      Mom4 momentum(double time) const;
      double momentumMag(double time) const  { return  fabs(mass_*betaGamma()); }
      double momentumVar(double time) const;
      double energy(double time) const  { return  fabs(mass_*ebar()/mbar_); }
      DVEC momDeriv(double time, LocalBasis::LocDir mdir) const;
      Vec3 direction(double time, LocalBasis::LocDir mdir= LocalBasis::momdir) const;
      double mass() const { return mass_;} // mass 
      int charge() const { return charge_;} // charge in proton charge units
      double paramVal(size_t index) const { return pars_.parameters()[index]; }
      PDATA const& params() const { return pars_; }
      PDATA& params() { return pars_; }

      // named parameter accessors
      double rad() const { return paramVal(rad_); }
      double lam() const { return paramVal(lam_); }
      double cx() const { return paramVal(cx_); }
      double cy() const { return paramVal(cy_); }
      double phi0() const { return paramVal(phi0_); }
      double t0() const { return paramVal(t0_); }
      
      // simple functions; these can be cached if they cause performance problems
      double sign() const { return copysign(1.0,mbar_); } // combined bending sign including Bz and charge
      double pbar2() const { return  rad()*rad() + lam()*lam(); } 
      double pbar() const { return  sqrt(pbar2()); } // momentum in mm
      double ebar2() const { return  pbar2() + mbar_*mbar_; }
      double ebar() const { return  sqrt(ebar2()); } // energy in mm
      double mbar() const { return mbar_; } // mass in mm; includes charge information!
      double Q() const { return mass_/mbar_; } // reduced charge
      double omega() const { return CLHEP::c_light*sign()/ ebar(); } // rotational velocity, sign set by magnetic force
      double beta() const { return pbar()/ebar(); } // relativistic beta
      double gamma() const { return fabs(ebar()/mbar_); } // relativistic gamma
      double betaGamma() const { return fabs(pbar()/mbar_); } // relativistic betagamma
      double dphi(double t) const { return omega()*(t - t0()); }
      double phi(double t) const { return dphi(t) + phi0(); }
      double ztime(double zpos) const { return t0() + zpos/(omega()*lam()); }
      double zphi(double zpos) const { return zpos/lam() + phi0(); }
      Vec3 const& bnom(double time=0.0) const { return bnom_; }
      double bnomR() const { return bnom_.R(); }
      // flip the helix in time and charge; it remains unchanged geometrically
      void invertCT() {
	mbar_ *= -1.0;
	charge_ *= -1;
	pars_.parameters()[t0_] *= -1.0;
//	pars_.parameters()[lam_] *= -1.0;
//	pars_.parameters()[rad_] *= -1.0;
      }
      //
    private :
      TRange trange_;
      PDATA pars_; // parameters
      double mass_;  // in units of MeV/c^2
      int charge_; // charge in units of proton charge
      double mbar_;  // reduced mass in units of mm, computed from the mass and nominal field
      Vec3 bnom_; // nominal BField
      ROOT::Math::Rotation3D l2g_, g2l_; // rotations between local and global coordinates 
      static std::vector<std::string> paramTitles_;
      static std::vector<std::string> paramNames_;
      static std::vector<std::string> paramUnits_;
      static std::string trajName_;
      // non-const accessors
      double& param(size_t index) { return pars_.parameters()[index]; }
 };
  std::ostream& operator <<(std::ostream& ost, LHelix const& lhel);
}
#endif
