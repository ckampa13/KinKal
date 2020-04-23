#ifndef KinKal_BField_hh
#define KinKal_BField_hh
// class defining a BField Map interface for use in KinKal.
#include "KinKal/Vectors.hh"
#include <vector>
#include "Math/SMatrix.h"

namespace KinKal {
  class BField {
    public:
      typedef ROOT::Math::SMatrix<float,3> Grad; // field gradient: ie dBi/d(x,y,z)
      virtual void fieldVect(Vec3& field, Vec3 const& position=Vec3()) const = 0; 
      virtual void fieldGrad(Grad& grad,Vec3 const& position=Vec3()) const = 0;
      virtual ~BField(){}
      // add interface for path integration FIXME!
  };

  // trivial instance of the above, used for testing
  class UniformBField : public BField {
    public:
      virtual void fieldVect(Vec3& fvec, Vec3 const& position=Vec3()) const override { fvec = fvec_; }
      virtual void fieldGrad(Grad& fgrad, Vec3 const& position=Vec3()) const override { fgrad = Grad(); }
      UniformBField(Vec3 const& bnom) : fvec_(bnom) {}
      UniformBField(double BZ) : UniformBField(Vec3(0.0,0.0,BZ)) {}
      virtual ~UniformBField(){}
    private:
      Vec3 fvec_; // constant field
  };

// use superposition to create a composite field
  class CompositeBField : public BField {
    public:
      virtual void fieldVect(Vec3& fvec, Vec3 const& position=Vec3()) const override {
	fvec = Vec3();
	for(auto const field : fields_ ){
	  Vec3 temp;
	  field->fieldVect(temp,position);
	  fvec += temp;
	}
      }
      virtual void fieldGrad(Grad& fgrad, Vec3 const& position=Vec3()) const override {
	fgrad = Grad();
	for(auto const field : fields_ ){
	  Grad temp;
	  field->fieldGrad(temp,position);
	  fgrad += temp;
	}

      }
      CompositeBField () {}
      void addField(BField const& field) { fields_.push_back(&field); }
    private:
      std::vector<const BField*> fields_; // fields
  };

  // simple Z gradient field, used to test Field corrections
  class GradBField : public BField {
    public:
      GradBField(double b0, double b1, double zg0, double zg1) :
	f0_(0.0,0.0,b0), f1_(0.0,0.0,b1), z0_(zg0), z1_(zg1), grad_((b1 - b0)/(zg1-zg0)) {}
      virtual void fieldVect(Vec3& fvec, Vec3 const& position=Vec3()) const override {
	if(position.z() < z0_)
	  fvec = f0_;
	else if(position.z() > z1_)
	  fvec = f1_; 
	else {
	  double bgrad = grad_*(position.z()-z0_);
	  // work in cylindrical coordinates
	  double bz = f0_.z()+bgrad;
	  double bx = -0.5*grad_*position.x();
	  double by = -0.5*grad_*position.y();
	  fvec = Vec3(bx,by,bz);
	}
      }
      virtual void fieldGrad(Grad& fgrad, Vec3 const& position=Vec3()) const override {
	if(position.z() > z0_ && position.z() < z1_ ){
	  std::vector<float> gvals;
	  gvals.push_back(-0.5*grad_);// dBx/dx 
	  gvals.push_back(0.0);// dBy/dx 
	  gvals.push_back(0.0);// dBz/dx
	  gvals.push_back(0.0);// dBx/dy 
	  gvals.push_back(-0.5*grad_);// dBy/dy
	  gvals.push_back(0.0);// dBz/dy
	  gvals.push_back(0.0);// dBx/dz 
	  gvals.push_back(0.0);// dBy/dz
	  gvals.push_back(grad_);// dBz/dz
	  fgrad = Grad(gvals.data(),9);
	} else
	  fgrad = Grad();
      }
      virtual ~GradBField(){}
    private:
      Vec3 f0_, f1_;
      double z0_, z1_;
      double grad_; // gradient in tesla/mm, computed from the fvec values
  };
}
#endif
