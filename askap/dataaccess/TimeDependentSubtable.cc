/// @file
/// @brief A base class for handler of a time-dependent subtable
/// @details All classes representing time-dependent subtables are expected
/// to be derived from this one. It implements the method to 
/// convert a fully specified epoch into casacore::Double intrinsically used by
/// the subtable. The actual subtable handler can use this for either 
/// an intelligent selection or efficient caching. The main idea behind this
/// class is to provide data necessary for a table
/// selection on the TIME column (which is a measure column). The class
/// reads units and the reference frame and sets up the converter.
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
#include <askap/dataaccess/TimeDependentSubtable.h>
#include <askap/dataaccess/EpochConverter.h>

#include <askap/askap/AskapError.h>
#include <askap/dataaccess/DataAccessError.h>

// casa includes
#include <casacore/casa/Arrays/Array.h>
#include <casacore/casa/BasicSL/String.h>
#include <casacore/tables/Tables/TableRecord.h>

// uncomment to use the logger, if it is really used somewhere.
//#include <askap_accessors.h>
//#include <askap/askap/AskapLogging.h>
//ASKAP_LOGGER(logger, "");


using namespace askap;
using namespace askap::accessors;

/// @brief obtain time epoch in the subtable's native format
/// @details Convert a given epoch to the table's native frame/units
/// @param[in] time an epoch specified as a measure
/// @return an epoch in table's native frame/units
casacore::Double TimeDependentSubtable::tableTime(const casacore::MEpoch &time) const
{
  if (!itsConverter) {
      // first use, we need to read frame/unit information and set up the 
      // converter
      initConverter();
   }
  ASKAPDEBUGASSERT(itsConverter);
  return (*itsConverter)(time);
}

/// @brief obtain a full epoch object for a given time (reverse conversion)
/// @details Some subtables can have more than one time-related columns, i.e.
/// TIME and INTERVAL. This method allows to form a full MEpoch measure from
/// the time represented as double in the native table's reference frame/unit.
/// It allows to extract frame/unit information and compare them with that of
/// the other columns. 
casacore::MEpoch TimeDependentSubtable::tableTime(casacore::Double time) const
{
  if (!itsConverter) {
      // first use, we need to read frame/unit information and set up the 
      // converter
      initConverter();
   }
  ASKAPDEBUGASSERT(itsConverter);
  return itsConverter->toMeasure(time);  
}

/// @brief initialize itsConverter
void TimeDependentSubtable::initConverter() const
{
  const casacore::Array<casacore::String> &tabUnits=table().tableDesc().
      columnDesc("TIME").keywordSet().asArrayString("QuantumUnits");
  if (tabUnits.nelements()!=1 || tabUnits.ndim()!=1) {
      ASKAPTHROW(DataAccessError, "Unable to interpret the QuantumUnits "
        "keyword for the TIME column of a time-dependent subtable (type="<<
         table().tableInfo().type()<<"). It should be a 1D Array of exactly "
        "one String element and the table has "<<tabUnits.nelements()<<
        " elements and "<<tabUnits.ndim()<<" dimensions");
  }
  const casacore::Unit timeUnits=casacore::Unit(tabUnits(casacore::IPosition(1,0)));
  
  const casacore::RecordInterface &timeMeasInfo=table().tableDesc().
        columnDesc("TIME").keywordSet().asRecord("MEASINFO");
  ASKAPASSERT(timeMeasInfo.asString("type")=="epoch");
            
  itsConverter.reset(new EpochConverter(casacore::MEpoch(casacore::MVEpoch(),
                     frameType(timeMeasInfo.asString("Ref"))),timeUnits));
}  


/// @brief translate a name of the epoch reference frame to the type enum
/// @details Table store the reference frame as a string and one needs a
/// way to convert it to a enum used in the constructor of the epoch
/// object to be able to construct it. This method provides a required
/// translation.
/// @param[in] name a string name of the reference frame 
casacore::MEpoch::Types TimeDependentSubtable::frameType(const casacore::String &name)
{
  if (name == "UTC") {
      return casacore::MEpoch::UTC;
  } else if (name == "TAI" || name == "IAT") {
      return casacore::MEpoch::TAI;
  } else if (name == "UT" || name == "UT1") {
      return casacore::MEpoch::UT1;
  } else if (name == "UT2") {
      return casacore::MEpoch::UT2;   
  } else if (name == "TDT" || name == "TT" || name == "ET") {
      return casacore::MEpoch::TDT;   
  } else if (name == "GMST" || name == "GMST1") {
      return casacore::MEpoch::GMST;
  } else if (name == "TCB") {
      return casacore::MEpoch::TCB;   
  } else if (name == "TDB") {
      return casacore::MEpoch::TDB;   
  } else if (name == "TCG") {
      return casacore::MEpoch::TCG;   
  } else if (name == "LAST") {
      return casacore::MEpoch::LAST;   
  } else if (name == "LMST") {
      return casacore::MEpoch::LMST;   
  } else if (name == "GAST") {
      return casacore::MEpoch::GAST;   
  } 
  ASKAPTHROW(DataAccessError, "The frame "<<name<<
              " is not supported at the moment");
  return casacore::MEpoch::UTC; // to keep the compiler happy
}
