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
/// @author Max Voronkov <maxim.voronkov@csiro.au>

#include <askap/imageaccess/ImageAccessFactory.h>
#include <cppunit/extensions/HelperMacros.h>

#include <casacore/casa/Arrays/Vector.h>
#include <casacore/casa/Arrays/IPosition.h>
#include <casacore/coordinates/Coordinates/LinearCoordinate.h>
#include <casacore/images/Regions/ImageRegion.h>
#include <casacore/images/Regions/RegionHandler.h>


#include <boost/shared_ptr.hpp>

#include <Common/ParameterSet.h>


namespace askap {

namespace accessors {

class CasaImageAccessTest : public CppUnit::TestFixture
{
   CPPUNIT_TEST_SUITE(CasaImageAccessTest);
   CPPUNIT_TEST(testReadWrite);
   CPPUNIT_TEST_SUITE_END();
public:
   void setUp() {
      LOFAR::ParameterSet parset;
      parset.add("imagetype","casa");
      itsImageAccessor = imageAccessFactory(parset);
   }

   void testReadWrite() {
      const std::string name = "tmp.testimage";
      CPPUNIT_ASSERT(itsImageAccessor);
      const casacore::IPosition shape(2,10,5);
      casacore::Array<float> arr(shape);
      arr.set(1.);
      casacore::CoordinateSystem coordsys(makeCoords());

      // create and write a constant into image
      itsImageAccessor->create(name, shape, coordsys);
      itsImageAccessor->write(name,arr);

      // check shape
      CPPUNIT_ASSERT(itsImageAccessor->shape(name) == shape);
      // read the whole array and check
      casacore::Array<float> readBack = itsImageAccessor->read(name);
      CPPUNIT_ASSERT(readBack.shape() == shape);
      for (int x=0; x<shape[0]; ++x) {
           for (int y=0; y<shape[1]; ++y) {
                const casacore::IPosition index(2,x,y);
                CPPUNIT_ASSERT(fabs(readBack(index)-arr(index))<1e-7);
           }
      }
      // write a slice
      casacore::Vector<float> vec(10,2.);
      itsImageAccessor->write(name,vec,casacore::IPosition(2,0,3));
      // read a slice
      vec = itsImageAccessor->read(name,casacore::IPosition(2,0,1),casacore::IPosition(2,9,1));
      CPPUNIT_ASSERT(vec.nelements() == 10);
      for (int x=0; x<10; ++x) {
           CPPUNIT_ASSERT(fabs(vec[x] - arr(casacore::IPosition(2,x,1)))<1e-7);
      }
      vec = itsImageAccessor->read(name,casacore::IPosition(2,0,3),casacore::IPosition(2,9,3));
      CPPUNIT_ASSERT(vec.nelements() == 10);
      for (int x=0; x<10; ++x) {
           CPPUNIT_ASSERT(fabs(vec[x] - arr(casacore::IPosition(2,x,3)))>1e-7);
           CPPUNIT_ASSERT(fabs(vec[x] - 2.)<1e-7);
      }
      // read the whole array and check
      readBack = itsImageAccessor->read(name);
      CPPUNIT_ASSERT(readBack.shape() == shape);
      for (int x=0; x<shape[0]; ++x) {
           for (int y=0; y<shape[1]; ++y) {
                const casacore::IPosition index(2,x,y);
                CPPUNIT_ASSERT(fabs(readBack(index) - (y == 3 ? 2. : 1.))<1e-7);
           }
      }
      CPPUNIT_ASSERT(itsImageAccessor->coordSys(name).nCoordinates() == 1);
      CPPUNIT_ASSERT(itsImageAccessor->coordSys(name).type(0) == casacore::CoordinateSystem::LINEAR);

      // auxilliary methods
      itsImageAccessor->setUnits(name,"Jy/pixel");
      itsImageAccessor->setBeamInfo(name,0.02,0.01,1.0);
      // mask tests

      itsImageAccessor->makeDefaultMask(name);



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
