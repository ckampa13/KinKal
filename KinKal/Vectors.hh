#ifndef KinKal_Vectors_hh
#define KinKal_Vectors_hh
// vetor typedefs used throughout the KinKal package
#include "Math/Cartesian3D.h"
#include "Math/Vector3D.h"
#include "Math/Vector4D.h"
#include "Math/Vector2D.h"
#include <stdexcept>
namespace KinKal {
  typedef ROOT::Math::XYZVector Vec3; // spatial only vector
  typedef ROOT::Math::Cylindrical3D<double> Cyl3; // Cylindrical vector.  Context-dependent z axis definition
  typedef ROOT::Math::Polar3D<double> Pol3; // 3D polar vector
  typedef ROOT::Math::XYZTVector Vec4; // double precision spacetime vector, 4th component = time or energy
  typedef ROOT::Math::PxPyPzMVector Mom4; // 4-momentum with 4th component = mass
  typedef ROOT::Math::Polar2D<double> Pol2; // 2D polar vector.  Context-dependent z axis definition
}
#endif
