/// @file
///
/// Unit test for the CASA image access code
///
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
/// @author Steve Ord <stephen.ord@csiro.au>

#include <askap/imageaccess/ImageAccessFactory.h>
#include <cppunit/extensions/HelperMacros.h>

#include <casacore/casa/Arrays/Vector.h>
#include <casacore/casa/Arrays/IPosition.h>
#include <casacore/casa/Arrays/Matrix.h>
#include <casacore/casa/Arrays/ArrayIO.h>

#include <casacore/coordinates/Coordinates/LinearCoordinate.h>
#include <casacore/coordinates/Coordinates/DirectionCoordinate.h>
#include <casacore/coordinates/Coordinates/SpectralCoordinate.h>
#include <casacore/coordinates/Coordinates/Projection.h>

#include <casacore/coordinates/Coordinates/CoordinateSystem.h>



#include <boost/shared_ptr.hpp>

#include <Common/ParameterSet.h>

#include "askap_accessors.h"
#include <askap/askap/AskapLogging.h>



namespace askap {

namespace accessors {

class FitsImageAccessTest : public CppUnit::TestFixture
{
   CPPUNIT_TEST_SUITE(FitsImageAccessTest);
   CPPUNIT_TEST(testReadWrite);
   CPPUNIT_TEST(testCreate);
   CPPUNIT_TEST_SUITE_END();
public:
    void setUp() {
        LOFAR::ParameterSet parset;
        parset.add("imagetype","fits");
        itsImageAccessor = imageAccessFactory(parset);
    }

    void testCreate() {
        // Create FITS image
        const std::string name = "tmpemptyfitsimage";

        CPPUNIT_ASSERT(itsImageAccessor);
        size_t ra=100, dec=100, spec=5;
        const casacore::IPosition shape(3,ra,dec,spec);
        casacore::Array<float> arr(shape);
        arr.set(1.);
        // Build a coordinate system for the image
        casacore::Matrix<double> xform(2,2);                                    // 1
        xform = 0.0; xform.diagonal() = 1.0;                          // 2
        casacore::DirectionCoordinate radec(casacore::MDirection::J2000,                  // 3
            casacore::Projection(casacore::Projection::SIN),        // 4
            135*casacore::C::pi/180.0, 60*casacore::C::pi/180.0,    // 5
            -1*casacore::C::pi/180.0, 1*casacore::C::pi/180,        // 6
            xform,                              // 7
            ra/2., dec/2.);                       // 8


        casacore::Vector<casacore::String> units(2); units = "deg";                        //  9
        radec.setWorldAxisUnits(units);

        // Build a coordinate system for the spectral axis
        // SpectralCoordinate
        casacore::SpectralCoordinate spectral(casacore::MFrequency::TOPO,               // 27
    				1400 * 1.0E+6,                  // 28
    				20 * 1.0E+3,                    // 29
    				0,                              // 30
    				1420.40575 * 1.0E+6);           // 31
        units.resize(1);
        units = "MHz";
        spectral.setWorldAxisUnits(units);

        casacore::CoordinateSystem coordsys;
        coordsys.addCoordinate(radec);
        coordsys.addCoordinate(spectral);


        itsImageAccessor->create(name, shape, coordsys);
    }
 
    void testReadWrite() {
        // Create FITS image
        const std::string name = "tmpfitsimage";

        CPPUNIT_ASSERT(itsImageAccessor);
        size_t ra=100, dec=100, spec=5;
        const casacore::IPosition shape(3,ra,dec,spec);
        casacore::Array<float> arr(shape);
        arr.set(1.);
        // Build a coordinate system for the image
        casacore::Matrix<double> xform(2,2);                                    // 1
        xform = 0.0; xform.diagonal() = 1.0;                          // 2
        casacore::DirectionCoordinate radec(casacore::MDirection::J2000,                  // 3
            casacore::Projection(casacore::Projection::SIN),        // 4
            135*casacore::C::pi/180.0, 60*casacore::C::pi/180.0,    // 5
            -1*casacore::C::pi/180.0, 1*casacore::C::pi/180,        // 6
            xform,                              // 7
            ra/2., dec/2.);                       // 8


        casacore::Vector<casacore::String> units(2); units = "deg";                        //  9
        radec.setWorldAxisUnits(units);

        // Build a coordinate system for the spectral axis
        // SpectralCoordinate
        casacore::SpectralCoordinate spectral(casacore::MFrequency::TOPO,               // 27
    				1400 * 1.0E+6,                  // 28
    				20 * 1.0E+3,                    // 29
    				0,                              // 30
    				1420.40575 * 1.0E+6);           // 31
        units.resize(1);
        units = "MHz";
        spectral.setWorldAxisUnits(units);

        casacore::CoordinateSystem coordsys;
        coordsys.addCoordinate(radec);
        coordsys.addCoordinate(spectral);


        itsImageAccessor->create(name, shape, coordsys);

        itsImageAccessor->write(name,arr);

        // // check shape
        CPPUNIT_ASSERT(itsImageAccessor->shape(name) == shape);
        // // // read the whole array and check
        casacore::Array<float> readBack = itsImageAccessor->read(name);
        CPPUNIT_ASSERT(readBack.shape() == shape);
        for (int x=0; x<shape[0]; ++x) {

            for (int y=0; y<shape[1]; ++y) {
                for (int z = 0; z < shape[2]; ++z) {
                    //std::cout << x << ":" << y << ":" << z << std::endl;
                    const casacore::IPosition index(3,x,y,z);
                    CPPUNIT_ASSERT(fabs(readBack(index)-arr(index))<1e-7);
                }
            }
        }
        // write a slice
        const casacore::IPosition chanShape(2,ra,dec);
        casacore::Array<float> chanArr(chanShape);
        chanArr.set(2.0);

        itsImageAccessor->write(name,chanArr,casacore::IPosition(3,0,0,2));
        // // read a slice
        // vec = itsImageAccessor->read(name,casacore::IPosition(2,0,1),casacore::IPosition(2,9,1));
        // CPPUNIT_ASSERT(vec.nelements() == 10);
        // for (int x=0; x<10; ++x) {
        //    CPPUNIT_ASSERT(fabs(vec[x] - arr(casacore::IPosition(2,x,1)))<1e-7);
        // }
        // vec = itsImageAccessor->read(name,casacore::IPosition(2,0,3),casacore::IPosition(2,9,3));
        // CPPUNIT_ASSERT(vec.nelements() == 10);
        // for (int x=0; x<10; ++x) {
        //    CPPUNIT_ASSERT(fabs(vec[x] - arr(casacore::IPosition(2,x,3)))>1e-7);
        //    CPPUNIT_ASSERT(fabs(vec[x] - 2.)<1e-7);
        // }
      // read the whole array and check
    //     casacore::Array<float> readBack = itsImageAccessor->read(name);
    //     CPPUNIT_ASSERT(readBack.shape() == shape);
    //     for (int x=0; x<shape[0]; ++x) {
    //         for (int y=0; y<shape[1]; ++y) {
    //             const casacore::IPosition index(2,x,y);
    //             CPPUNIT_ASSERT(fabs(readBack(index) - (y == 3 ? 2. : 1.))<1e-7);
    //        }
    //   }
    //   CPPUNIT_ASSERT(itsImageAccessor->coordSys(name).nCoordinates() == 1);
    //   CPPUNIT_ASSERT(itsImageAccessor->coordSys(name).type(0) == casacore::CoordinateSystem::LINEAR);
      //
    //   // auxilliary methods
        itsImageAccessor->setUnits(name,"Jy/pixel");
        itsImageAccessor->setBeamInfo(name,0.02,0.01,1.0);

        casacore::Vector<casacore::Quantum<double> > beamInfo = itsImageAccessor->beamInfo(name);

   }

protected:

   casacore::CoordinateSystem makeCoords() {
      casacore::Vector<casacore::String> names(2);
      names[0]="x"; names[1]="y";
      casacore::Vector<double> increment(2 ,1.);

      casacore::Matrix<double> xform(2,2,0.);
      xform.diagonal() = 1.;
      casacore::LinearCoordinate linear(names, casacore::Vector<casacore::String>(2,"pixel"),
             casacore::Vector<double>(2,0.),increment, xform, casacore::Vector<double>(2,0.));

      casacore::CoordinateSystem coords;
      coords.addCoordinate(linear);
      return coords;
   }

private:
   /// @brief method to access image
   boost::shared_ptr<IImageAccess> itsImageAccessor;
};

} // namespace accessors

} // namespace askap
