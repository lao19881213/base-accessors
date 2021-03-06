/// @file
/// @brief A method to set up converters and selectors from parset file
/// @details Parameters are currently passed around using parset files.
/// The methods declared in this file set up converters and selectors
/// from the ParameterSet object. This is probably a temporary solution.
/// This code can eventually become a part of some class (e.g. a DataSource
/// which returns selectors and converters with the defaults alread
/// applied according to the parset file).
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

#ifndef ASKAP_ACCESSORS_PARSET_INTERFACE_H
#define ASKAP_ACCESSORS_PARSET_INTERFACE_H

// own includes
#include <askap/dataaccess/IDataConverter.h>
#include <askap/dataaccess/IDataSelector.h>
#include <Common/ParameterSet.h>

// boost includes
#include <boost/shared_ptr.hpp>

namespace askap {

namespace accessors {

/// @brief set selections according to the given parset object
/// @details
/// @param[in] sel a shared pointer to the converter to be updated
/// @param[in] parset a parset object to read the parameters from
/// @ingroup dataaccess_hlp
void operator<<(const boost::shared_ptr<IDataSelector> &sel,
                          const LOFAR::ParameterSet &parset);

} // namespace accessors

} // namespace askap


#endif // #ifndef PARSET_INTERFACE_H
