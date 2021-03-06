/// @file
/// @brief accessor adapter fitting the best w-plane 
///
/// @details This is an adapter to data accessor which fits a plane
/// into w=w(u,v) and corrects w to represent the distance from
/// this plane rather than the absolute w-term. The planar component
/// can be taken out as a shift in the image space. The adapter provides
/// methods to obtain the magnitude of the shift (i.e. fit coefficients).
/// This class also checkes whether the deviation from the plane is within the
/// tolerance set-up at the construction. A new plane is fitted if necessary. 
/// @note An exception is thrown if the layout is so non-coplanar, that the
/// required tolerance cannot be met.
///
/// @copyright (c) 2007 CSIRO
/// Australia Telescope National Facility (ATNF)
/// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
/// PO Box 76, Epping NSW 1710, Australia
/// atnf-enquiries@csiro.au
///
/// This file is part of the ASKAP software distribution.
///
/// The ASKAP software distribution is free software: you can redistribute it
/// and/or modify it under the terms of the GNU General Public License as
/// published by the Free Software Foundation; either version 2 of the License,
/// or (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the Free Software
/// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
///
/// @author Max Voronkov <maxim.voronkov@csiro.au>
///

// own includes
#include <askap/dataaccess/BestWPlaneDataAccessor.h>
// we use just a static method to track changes of the tangent point
#include <askap/dataaccess/UVWMachineCache.h>
// just for now adding the logger so I can see what is going on
#include <askap/askap/AskapLogging.h>

using namespace askap;
using namespace askap::accessors;

ASKAP_LOGGER(logger,".casaAccessors")

/// @brief constructor
/// @details The only parameter is the w-term tolerance in wavelengths
/// If the deviation from the fitted plane exceeds the tolerance, a new
/// fit will be performed. If it doesn't help, an exception will be thrown.
///
/// @param[in] tolerance w-term tolerance in wavelengths
/// @param[in] checkResidual if true, the magnitude of the residual w-term is checked to be below tolerance 
/// @note An exception could be thrown during the actual processing, not
/// in the constructor call itself.
BestWPlaneDataAccessor::BestWPlaneDataAccessor(const double tolerance, const bool checkResidual) : itsCheckResidual(checkResidual), 
       itsWTolerance(tolerance),
       itsCoeffA(0.), itsCoeffB(0.), itsUVWChangeMonitor(changeMonitor()), itsPredictWPlane(false)
{
   
}

/// @brief copy constructor
/// @details We need it because we have data members of non-trivial types
/// @param[in] other another instance of adapter to copy from
BestWPlaneDataAccessor::BestWPlaneDataAccessor(const BestWPlaneDataAccessor &other) : 
    itsCheckResidual(other.itsCheckResidual), itsWTolerance(other.itsWTolerance), itsCoeffA(other.itsCoeffA),
    itsCoeffB(other.itsCoeffB), itsUVWChangeMonitor(changeMonitor()), itsPlaneChangeMonitor(changeMonitor()),
    itsRotatedUVW(other.itsRotatedUVW.copy()), itsLastTangentPoint(other.itsLastTangentPoint),
    itsPredictWPlane(other.itsPredictWPlane), itsPredictTimeInterval(other.itsPredictTimeInterval) {}

/// @brief assignment operator
/// @details We need it because we have data members of non-trivial types
/// @param[in] other another instance of adapter to copy from
BestWPlaneDataAccessor& BestWPlaneDataAccessor::operator=(const BestWPlaneDataAccessor &other) 
{
  if (&other != this) {
      itsCheckResidual = other.itsCheckResidual;
      itsWTolerance = other.itsWTolerance;
      itsCoeffA = other.itsCoeffA;
      itsCoeffB = other.itsCoeffB;
      itsUVWChangeMonitor.notifyOfChanges();
      itsPlaneChangeMonitor.notifyOfChanges();
      itsRotatedUVW.assign(other.itsRotatedUVW.copy());
      itsLastTangentPoint = other.itsLastTangentPoint;
      itsPredictWPlane = other.itsPredictWPlane;
      itsPredictTimeInterval = other.itsPredictTimeInterval;
  }
  return *this;
}

/// @brief uvw after rotation
/// @details This method subtracts the best plane out of the w coordinates
/// (after uvw-rotation) and return the resulting vectors.
/// @param[in] tangentPoint tangent point to rotate the coordinates to
/// @return uvw after rotation to the new coordinate system for each row
/// @note An exception is thrown if the layout is so non-coplanar that
/// the required tolerance on w-term cannot be met.
const casacore::Vector<casacore::RigidVector<casacore::Double, 3> >&
	         BestWPlaneDataAccessor::rotatedUVW(const casacore::MDirection &tangentPoint) const
{
   // original accessor, this would throw an exception if an accessor is not assigned
   const IConstDataAccessor &acc = getROAccessor();
   
   // change monitor should indicate a change for the first ever call to this method
   // (because an associate method should have been called by now)
   if (itsUVWChangeMonitor == changeMonitor()) {
       // just a sanity check to ensure that assumptions hold
       ASKAPCHECK(UVWMachineCache::compare(tangentPoint,itsLastTangentPoint,1e-6),
           "Current implementation implies that only one tangent point is used per single BestWPlaneDataAccessor adapter. "
           "rotatedUVW got tangent point="<<tangentPoint<<", while the last one was "<<itsLastTangentPoint);
       // no change detected, return the buffer
       return itsRotatedUVW;
   }
   // need to compute uvw's
   itsLastTangentPoint = tangentPoint;
   
   // rotate UVW and get deviations for advanced times
   //
   // the current type is in the apparent frame (APP) and in geocentric.
 
   const casacore::Vector<casacore::RigidVector<casacore::Double, 3> >& originalUVW = acc.rotatedUVW(tangentPoint);
   if (itsRotatedUVW.nelements() != originalUVW.nelements()) {
       itsRotatedUVW.resize(originalUVW.nelements());
   }
   
   // compute tolerance in metres to match units of originalUVW
   const casacore::Vector<double>& freq = acc.frequency();
   ASKAPCHECK(freq.nelements()>=1, "An unexpected accessor with zero spectral channels has been encountered");
   
   // use the largest frequency/smallest wavelength, i.e. worst case scenario
   const double maxFreq = freq.nelements() == 1 ? freq[0] : casacore::max(freq[0],freq[freq.nelements()-1]);
   ASKAPDEBUGASSERT(maxFreq > 0.); 
   const double tolInMetres = itsWTolerance * casacore::C::c / maxFreq;
   
   double maxDeviation = 0.;
   
   if (itsPredictWPlane == false) {
       maxDeviation = updatePlaneIfNecessary(originalUVW, tolInMetres);
   }
   else {
       maxDeviation = updateAdvancedTimePlaneIfNecessary(tolInMetres, tangentPoint);
   }
   if (itsCheckResidual) {
       ASKAPCHECK(maxDeviation < tolInMetres, "The antenna layout is significantly non-coplanar. "
             "The largest w-term deviation after the fit of "<<maxDeviation<<" metres exceedes the w-term tolerance of "<<
              itsWTolerance<<" wavelengths equivalent to "<<tolInMetres<<" metres.");
   }

    
   for (casacore::uInt row=0; row<originalUVW.nelements(); ++row) {
        const casacore::RigidVector<casacore::Double, 3> currentUVW = originalUVW[row];
        itsRotatedUVW[row] = currentUVW;
        // subtract the current plane
        itsRotatedUVW[row](2) -= coeffA()*currentUVW(0) + coeffB()*currentUVW(1);
   }
   

   return itsRotatedUVW;
}	         

/// @brief calculate the largest deviation from the current fitted plane
/// @details This helper method iterates through the given uvw's and returns
/// the largest deviation of the w-term from the current best fit plane.
/// @param[in] uvw a vector with uvw's
/// @return the largest w-term deviation from the current plane (same units as uvw's)
double BestWPlaneDataAccessor::maxWDeviation(const casacore::Vector<casacore::RigidVector<casacore::Double, 3> >& uvw) const
{
   double maxDeviation = 0.;

   // we fit w=Au+Bv, the following lines compute the largest deviation from the current plane.

   for (casacore::uInt row=0; row<uvw.nelements(); ++row) {
        const casacore::RigidVector<casacore::Double, 3> currentUVW = uvw[row];
        const double deviation = fabs(coeffA()*currentUVW(0) + coeffB()*currentUVW(1) - currentUVW(2));
        if (deviation > maxDeviation) {
            maxDeviation = deviation;
        }
   }
   
   return maxDeviation;
}

/// @brief Fit a new plane assuming this is a continuous track and update coefficients if neccessary.
/// @details A best fit plane for the current time can be found with UpdatePlaneIfNecessary ... which minimises the maxW now
/// But this method instead minimises sometime in the future - so that we are currently at tolerance
/// we trend to 0 deviation then drift away to tolerance again. This should reduce the number of W fits and regrids by a factor
/// of two for long tracks
/// @param[in] the tolerance (same units as uvw's)
/// @param[in] the tangent point
/// @return the largest w-term deviation from the fitted plane (same units as uvw's)


double BestWPlaneDataAccessor::updateAdvancedTimePlaneIfNecessary(double tolerance, const casacore::MDirection &tangentPoint) const
{
   
    const bool verbose = false; // verbosity flag for checking
   // we need the accessor becuase I want to spin the uvw's
   const IConstDataAccessor &acc = getROAccessor();
   

   const casacore::Vector<casacore::RigidVector<casacore::Double, 3> >& uvw = acc.rotatedUVW(tangentPoint);
   // these are local coefficients of planes - need these before we are sure which
   // plane we are going to use.
   double tmpCoeffA = 0.0;
   double tmpCoeffB = 0.0;

   double AdvancedDeviation = maxWDeviation(uvw); // using current plane
   
   if (verbose) {
       ASKAPLOG_INFO_STR(logger, "BestWPlaneDataAccessor::On entry current deviation (using the current plane) " << AdvancedDeviation << " tolerance " << tolerance);
       ASKAPLOG_INFO_STR(logger, "BestWPlaneDataAccessor:: w = u * " << coeffA() << " + v * " << coeffB());
   }
    
   if (AdvancedDeviation < tolerance) {
       return AdvancedDeviation;
   }
    
   // we are out of our tolerance range - get a new plane
   // First thing we should do is use the existing update plane to get a plane that minimises W-deviation
   // This fragment breaks the rules of code-reuse - but means I dont have to worry about the changemonitor picking this up
   // we fit w=Au+Bv, the following lines accumulate the necessary sums of the LSF problem
       
   double su2 = 0.; // sum of u-squared
   double sv2 = 0.; // sum of v-squared
   double suv = 0.; // sum of uv-products
   double suw = 0.; // sum of uw-products
   double svw = 0.; // sum of vw-products
       
   for (casacore::uInt row=0; row<uvw.nelements(); ++row) {
       const casacore::RigidVector<casacore::Double, 3> currentUVW = uvw[row];
           
       su2 += casacore::square(currentUVW(0));
       sv2 += casacore::square(currentUVW(1));
       suv += currentUVW(0) * currentUVW(1);
       suw += currentUVW(0) * currentUVW(2);
       svw += currentUVW(1) * currentUVW(2);
   }
       
   // we need a non-zero determinant for a successful fitting
   // some tolerance has to be put on the determinant to avoid unconstrained fits
   // we just accept the current fit results if the new fit is not possible
   double D = su2 * sv2 - casacore::square(suv);
       
   if (fabs(D) < 1e-7) {
       ASKAPLOG_INFO_STR(logger, "BestWPlaneDataAccessor::updateAdvancedTimePlaneIfNecessary::Matrix has almost 0 determinant fit not likely to be valid");
       return AdvancedDeviation;
   }
       
   // make an update to the coefficients
   tmpCoeffA = (sv2 * suw - suv * svw) / D;
   tmpCoeffB = (su2 * svw - suv * suw) / D;
       
       
   // this fragment basically replicates the maxDeviation functionality
     
   AdvancedDeviation = 0.;
       // we fit w=Au+Bv, the following lines compute the largest deviation from the current plane.
       
   for (casacore::uInt row=0; row<uvw.nelements(); ++row) {
       const casacore::RigidVector<casacore::Double, 3> currentUVW = uvw[row];
       const double deviation = fabs(tmpCoeffA*currentUVW(0) + tmpCoeffB*currentUVW(1) - currentUVW(2));
       if (deviation > AdvancedDeviation) {
           AdvancedDeviation = deviation;
       }
   }
   if (AdvancedDeviation > tolerance) {
       ASKAPLOG_INFO_STR(logger, "BestWPlaneDataAccessor::updateAdvancedTimePlaneIfNecessary Current deviation (after next plane fit) " << AdvancedDeviation);
       return AdvancedDeviation; // we cannot get below tolerance at all - let alone in the future - the calling function will pick this up
   }
   if (verbose) {
       ASKAPLOG_INFO_STR(logger, "BestWPlaneDataAccessor::Current deviation (after next plane fit) " << AdvancedDeviation);
   }
    
   casacore::Quantity angle(-1*(360./86400.0),"deg");       // 1 second of rotation

   double TimeShift = itsPredictTimeInterval; // number of seconds
   double TotalShift = 0.0;

   casacore::MDirection newTangentPoint = tangentPoint;

   while (AdvancedDeviation < tolerance) {
       // lets advance the uvw in time until we are out of tolerance again.
       newTangentPoint.shiftLongitude(TimeShift*angle.getValue("rad"),true);
       TotalShift = TotalShift+TimeShift;
    
       const casacore::Vector<casacore::RigidVector<casacore::Double, 3> >& test_uvw = acc.rotatedUVW(newTangentPoint);
           
       AdvancedDeviation = 0.; //reset
           // we fit w=Au+Bv, the following lines compute the largest deviation from the current plane.
           
       for (casacore::uInt row=0; row<uvw.nelements(); ++row) {
           const casacore::RigidVector<casacore::Double, 3> currentUVW = test_uvw[row];
           const double deviation = fabs(tmpCoeffA*currentUVW(0) + tmpCoeffB*currentUVW(1) - currentUVW(2));
           if (deviation > AdvancedDeviation) {
               AdvancedDeviation = deviation;
           }
       }
       if (verbose) {
          ASKAPLOG_INFO_STR(logger, "BestWPlaneDataAccessor::Current deviation (after  " << TotalShift << " seconds) " << AdvancedDeviation);
       }
   }
   // We now have advanced time sufficiently that we will be out of tolerance
   // Lets pull back one time step then evaluate the plane for then.
   double on_exit_deviation = 0.;
   do {
       const casacore::Vector<casacore::RigidVector<casacore::Double, 3> >& on_exit_uvw = acc.rotatedUVW(tangentPoint);
       on_exit_deviation = maxWDeviation(on_exit_uvw);
    
       newTangentPoint.shiftLongitude(-1.0*TimeShift*angle.getValue("rad"),true);
       const casacore::Vector<casacore::RigidVector<casacore::Double, 3> >&advanced_uvw = acc.rotatedUVW(newTangentPoint);
       
     // we fit w=Au+Bv, the following lines accumulate the necessary sums of the LSF problem
       
       su2 = 0.; // sum of u-squared
       sv2 = 0.; // sum of v-squared
       suv = 0.; // sum of uv-products
       suw = 0.; // sum of uw-products
       svw = 0.; // sum of vw-products
       
       for (casacore::uInt row=0; row<uvw.nelements(); ++row) {
           const casacore::RigidVector<casacore::Double, 3> currentUVW = advanced_uvw[row];
           
           su2 += casacore::square(currentUVW(0));
           sv2 += casacore::square(currentUVW(1));
           suv += currentUVW(0) * currentUVW(1);
           suw += currentUVW(0) * currentUVW(2);
           svw += currentUVW(1) * currentUVW(2);
       }
       
       // we need a non-zero determinant for a successful fitting
       // some tolerance has to be put on the determinant to avoid unconstrained fits
       // we just accept the current fit results if the new fit is not possible
       D = su2 * sv2 - casacore::square(suv);
       
       if (fabs(D) < 1e-7) {
           return on_exit_deviation;
       }
       
       // make an update to the coefficients
       itsCoeffA = (sv2 * suw - suv * svw) / D;
       itsCoeffB = (su2 * svw - suv * suw) / D;
       itsPlaneChangeMonitor.notifyOfChanges();
   
   } while (on_exit_deviation > tolerance);
       
   if (verbose) {
           ASKAPLOG_INFO_STR(logger, "BestWPlaneDataAccessor::On exit deviation " << on_exit_deviation);
           ASKAPLOG_INFO_STR(logger, "BestWPlaneDataAccessor:: w = u * " << coeffA() << " + v * " << coeffB());
   }
   
   return maxWDeviation(uvw);


}
    
/// @brief fit a new plane and update coefficients if necessary
/// @details This method iterates over given uvw's, checks whether the
/// largest deviation of the w-term from the current plane is above the 
/// tolerance and updates the fit coefficients if it is. 
/// planeChangeMonitor() can be used to detect the change in the fit plane.
/// 
/// @param[in] uvw a vector with uvw's
/// @param[in] tolerance tolerance in the same units as uvw's
/// @return the largest w-term deviation from the fitted plane (same units as uvw's)
/// @note If a new fit is performed, the devitation is reported with respect to the
/// new fit (it takes place if the deviation from initial plane exceeds the given tolerance).
/// Therefore, if the returned deviation exceeds the tolerance, the layout is significantly
/// non-coplanar, so the required tolerance cannot be achieved.
/// This method has a conceptual constness as it doesn't change the original accessor.
double BestWPlaneDataAccessor::updatePlaneIfNecessary(const casacore::Vector<casacore::RigidVector<casacore::Double, 3> >& uvw,
                 double tolerance) const
{

   const double maxDeviation = maxWDeviation(uvw);
    
   // we need at least two rows for a successful fitting, don't bother doing anything if the
   // number of rows is too small or the deviation is below the tolerance
   if ((uvw.nelements() < 2) || (maxDeviation < tolerance)) {
       return maxDeviation;
   }
   
   // we fit w=Au+Bv, the following lines accumulate the necessary sums of the LSF problem
   
   double su2 = 0.; // sum of u-squared
   double sv2 = 0.; // sum of v-squared
   double suv = 0.; // sum of uv-products
   double suw = 0.; // sum of uw-products
   double svw = 0.; // sum of vw-products
   
   for (casacore::uInt row=0; row<uvw.nelements(); ++row) {
        const casacore::RigidVector<casacore::Double, 3> currentUVW = uvw[row];

        su2 += casacore::square(currentUVW(0));
        sv2 += casacore::square(currentUVW(1));
        suv += currentUVW(0) * currentUVW(1);
        suw += currentUVW(0) * currentUVW(2);
        svw += currentUVW(1) * currentUVW(2);                
   }
   
   // we need a non-zero determinant for a successful fitting
   // some tolerance has to be put on the determinant to avoid unconstrained fits
   // we just accept the current fit results if the new fit is not possible
   const double D = su2 * sv2 - casacore::square(suv);

   if (fabs(D) < 1e-7) {
       return maxDeviation;
   }

   // make an update to the coefficients
   itsCoeffA = (sv2 * suw - suv * svw) / D;
   itsCoeffB = (su2 * svw - suv * suw) / D;
   itsPlaneChangeMonitor.notifyOfChanges();
  
   return maxWDeviation(uvw);
}

