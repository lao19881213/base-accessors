/// @file
///
/// Unit test for CalParamNameHelper class (naming convension for
/// the calibration parameters)
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

#include <cppunit/extensions/HelperMacros.h>

// own includes
#include <askap/calibaccess/JonesIndex.h>
#include <askap/calibaccess/CalParamNameHelper.h>

// std includes
#include <string>
#include <utility>

namespace askap {

namespace accessors {

class CalParamNameHelperTest : public CppUnit::TestFixture 
{
   CPPUNIT_TEST_SUITE(CalParamNameHelperTest);
   CPPUNIT_TEST(testToString);
   CPPUNIT_TEST(testFromString);
   CPPUNIT_TEST_EXCEPTION(testFromStringException1, AskapError);
   CPPUNIT_TEST_EXCEPTION(testFromStringException2, AskapError);
   CPPUNIT_TEST_EXCEPTION(testFromStringException3, AskapError);
   CPPUNIT_TEST_EXCEPTION(testFromStringException4, AskapError);
   CPPUNIT_TEST(testChannelPacking);
   CPPUNIT_TEST_SUITE_END();
public:
   void testToString() {
      CPPUNIT_ASSERT_EQUAL(std::string("gain.g11.21.5"),CalParamNameHelper::paramName(JonesIndex(21u,5u),casacore::Stokes::XX));
      CPPUNIT_ASSERT_EQUAL(std::string("gain.g22.11.11"),CalParamNameHelper::paramName(JonesIndex(11u,11u),casacore::Stokes::YY));
      CPPUNIT_ASSERT_EQUAL(std::string("leakage.d12.10.1"),CalParamNameHelper::paramName(JonesIndex(10u,1u),casacore::Stokes::XY));
      CPPUNIT_ASSERT_EQUAL(std::string("leakage.d21.15.10"),CalParamNameHelper::paramName(JonesIndex(15u,10u),casacore::Stokes::YX));
      // bandpass parameters      
      CPPUNIT_ASSERT_EQUAL(std::string("bp.gain.g11.21.5"),CalParamNameHelper::paramName(JonesIndex(21u,5u),casacore::Stokes::XX,true));
      CPPUNIT_ASSERT_EQUAL(std::string("bp.gain.g22.11.11"),CalParamNameHelper::paramName(JonesIndex(11u,11u),casacore::Stokes::YY,true));
      CPPUNIT_ASSERT_EQUAL(std::string("bp.leakage.d12.10.1"),CalParamNameHelper::paramName(JonesIndex(10u,1u),casacore::Stokes::XY,true));
      CPPUNIT_ASSERT_EQUAL(std::string("bp.leakage.d21.15.10"),CalParamNameHelper::paramName(JonesIndex(15u,10u),casacore::Stokes::YX,true));
      CPPUNIT_ASSERT_EQUAL(std::string("bp."), CalParamNameHelper::bpPrefix());
   }
   
   void doFromStringChecks(const casacore::uInt ant, const casacore::uInt beam, const casacore::Stokes::StokesTypes pol) {
      const JonesIndex index(ant,beam);
      const std::string name = CalParamNameHelper::paramName(index,pol);
      CPPUNIT_ASSERT(!CalParamNameHelper::bpParam(name));
      const std::pair<JonesIndex, casacore::Stokes::StokesTypes> res = CalParamNameHelper::parseParam(name);
      CPPUNIT_ASSERT((res.first.antenna() >=0) && (res.first.antenna()<256)); 
      CPPUNIT_ASSERT((res.first.beam() >=0) && (res.first.beam()<256)); 
      CPPUNIT_ASSERT_EQUAL(ant, casacore::uInt(res.first.antenna()));
      CPPUNIT_ASSERT_EQUAL(beam, casacore::uInt(res.first.beam()));
      CPPUNIT_ASSERT(index == res.first);
      CPPUNIT_ASSERT(pol == res.second);

      // bandpass parameter
      const std::string bpName = CalParamNameHelper::paramName(index,pol,true);
      CPPUNIT_ASSERT(CalParamNameHelper::bpParam(bpName));
      const std::pair<JonesIndex, casacore::Stokes::StokesTypes> bpRes = CalParamNameHelper::parseParam(bpName);
      CPPUNIT_ASSERT((bpRes.first.antenna() >=0) && (bpRes.first.antenna()<256)); 
      CPPUNIT_ASSERT((bpRes.first.beam() >=0) && (bpRes.first.beam()<256)); 
      CPPUNIT_ASSERT_EQUAL(ant, casacore::uInt(bpRes.first.antenna()));
      CPPUNIT_ASSERT_EQUAL(beam, casacore::uInt(bpRes.first.beam()));
      CPPUNIT_ASSERT(index == bpRes.first);
      CPPUNIT_ASSERT(pol == bpRes.second);                                                 
   }
   
   void testFromString() {
      for (casacore::uInt ant=0; ant<36; ++ant) {
           for (casacore::uInt beam=0; beam<30; ++beam) {
                doFromStringChecks(ant,beam,casacore::Stokes::XX);
                doFromStringChecks(ant,beam,casacore::Stokes::XY);
                doFromStringChecks(ant,beam,casacore::Stokes::YX);
                doFromStringChecks(ant,beam,casacore::Stokes::YY);                
           }
      }
   }
   
   void testFromStringException1() {
        CalParamNameHelper::parseParam("something.g11.3.4");
   }

   void testFromStringException2() {
        CalParamNameHelper::parseParam("leakage.junk.3.4");
   }

   void testFromStringException3() {
        CalParamNameHelper::parseParam("leakage.d21.3");
   }

   void testFromStringException4() {
        CalParamNameHelper::parseParam("gain.g11.3.xx");
   }
   
   void testChannelPacking() {
        const std::string base("bp.gain.g11.3.4");
        CPPUNIT_ASSERT_EQUAL(base + ".15", CalParamNameHelper::addChannelInfo(base,15));
        CPPUNIT_ASSERT_EQUAL(base, CalParamNameHelper::extractChannelInfo(base+".15").second);
        CPPUNIT_ASSERT_EQUAL(casacore::uInt(15), CalParamNameHelper::extractChannelInfo(base+".15").first);
   }
   
}; // class CalParamNameHelperTest

} // namespace accessors

} // namespace askap

