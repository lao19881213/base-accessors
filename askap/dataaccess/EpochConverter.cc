/// @file EpochConverter.cc
/// @brief A class for epoch conversion
/// @details This is an implementation
/// of the low-level interface, which is used within the implementation of
/// the data accessor. The end user interacts with the IDataConverter
/// class. 
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

/// CASA includes
#include <casacore/measures/Measures/MCEpoch.h>
#include <casacore/casa/Quanta/MVTime.h>
#include <casacore/measures/Measures/MeasConvert.h>

/// own includes
#include <askap/dataaccess/EpochConverter.h>


using namespace askap;
using namespace askap::accessors;
using namespace casa;

/// create a converter to the target frame/unit
/// @param targetOrigin a measure describing target reference frame
///        and origin (e.g. w.r.t. midnight 30/05/2007 UTC)
///        Class defaults to MJD 0 UTC
/// @param targetUnit desired units in the output. Class defaults
///        to seconds
EpochConverter::EpochConverter(const casacore::MEpoch &targetOrigin,
                       const casacore::Unit &targetUnit) :
        itsTargetOrigin(targetOrigin.getValue()),
	itsTargetRef(targetOrigin.getRef()),
	itsTargetUnit(targetUnit) {}

/// convert specified MEpoch to the target units/frame
/// @param in an epoch to convert. 
casacore::Double EpochConverter::operator()(const casacore::MEpoch &in) const
{
  /// this class is supposed to be used in the most general case, hence
  /// we do all conversions. Specializations can be written for the cases
  /// when either frame or unit conversions are not required
  MVEpoch converted=MEpoch::Convert(in.getRef(),
                        itsTargetRef)(in).getValue();
  // relative to the origin
  converted-=itsTargetOrigin;
  return converted.getTime(itsTargetUnit).getValue();
}

/// set a frame (for epochs it is just a position), where the
/// conversion is performed
void EpochConverter::setMeasFrame(const casacore::MeasFrame &frame)
{
  itsTargetRef.set(frame);
}

/// Reverse conversion (casacore::Double to full measure)
/// @param in an epoch given as Double in the target units/frame
/// @return the same epoch as a fully qualified measure
casacore::MEpoch EpochConverter::toMeasure(casacore::Double in) const
{
  casacore::MVEpoch res(casacore::Quantity(in,itsTargetUnit));
  res+=itsTargetOrigin;
  return casacore::MEpoch(res,itsTargetRef);
}

/// Reverse conversion (casacore::MVEpoch to full measure)
/// @param in an epoch given as MVEpoch in the target frame
/// @return the same epoch as a fully qualified measure
casacore::MEpoch EpochConverter::toMeasure(const casacore::MVEpoch &in)
                                       const throw()
{
  return casacore::MEpoch(in+itsTargetOrigin,itsTargetRef);
}
