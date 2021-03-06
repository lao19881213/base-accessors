/// @file
/// @brief an implementation of IDataAccessor for original visibility
///
/// @details TableDataAccessor is an implementation of the
/// DataAccessor for original visibility working with TableDataIterator.
/// At this moment this class just throws an exception if a write is
/// attempted and mirrors all const functions.
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

/// own includes
#include <askap/dataaccess/TableDataAccessor.h>
#include <askap/dataaccess/DataAccessError.h>

using namespace askap;
using namespace askap::accessors;

/// construct an object linked with the given read-write iterator
/// @param iter a reference to the associated read-write iterator
TableDataAccessor::TableDataAccessor(const TableDataIterator &iter) :
                 MetaDataAccessor(iter.getAccessor()), 
                 itsVisNeedsFlush(false), itsFlagNeedsFlush(false),
                 itsIterator(iter) {}

/// Read-only visibilities (a cube is nRow x nChannel x nPol; 
/// each element is a complex visibility)
///
/// @return a reference to nRow x nChannel x nPol cube, containing
/// all visibility data
///
const casacore::Cube<casacore::Complex>& TableDataAccessor::visibility() const
{
  return getROAccessor().visibility();  
}

/// Read-write access to visibilities (a cube is nRow x nChannel x nPol;
/// each element is a complex visibility)
///
/// @return a reference to nRow x nChannel x nPol cube, containing
/// all visibility data
///
casacore::Cube<casacore::Complex>& TableDataAccessor::rwVisibility()
{    
  if (!itsIterator.mainTableWritable()) {
      throw DataAccessLogicError("rwVisibility() is used for original visibilities, "
           "but the table is not writable");
  }
  
  itsVisNeedsFlush = true;
  
  // the solution with const_cast is not very elegant, however it seems to
  // be the only alternative to creating a copy of the buffer or making the whole
  // const interface untidy by putting a non-const method there. 
  // It is safe to use const_cast here because we know that the actual buffer
  // is declared mutable in CachedAccessorField.
  return const_cast<casacore::Cube<casacore::Complex>&>(getROAccessor().visibility());
}

/// Cube of flags corresponding to the output of visibility()
/// @return a reference to nRow x nChannel x nPol cube with the flag
///         information. If True, the corresponding element is flagged.
const casacore::Cube<casacore::Bool>& TableDataAccessor::flag() const
{
  return getROAccessor().flag();  
}

/// Non-const access to the cube of flags.
/// @return a reference to nRow x nChannel x nPol cube with the flag
///         information. If True, the corresponding element is flagged.
casacore::Cube<casacore::Bool>& TableDataAccessor::rwFlag()
{
   if (!itsIterator.mainTableWritable()) {
       throw DataAccessLogicError("rwFlag() is used for original visibilities, "
           "but the table is not writable");
   }
   // also need a check that FLAG_ROW is not present

   itsFlagNeedsFlush = true;
  
   // the solution with const_cast is not very elegant, however it seems to
   // be the only alternative to creating a copy of the buffer or making the whole
   // const interface untidy by putting a non-const method there. 
   // It is safe to use const_cast here because we know that the actual buffer
   // is declared mutable in CachedAccessorField.
   return const_cast<casacore::Cube<casacore::Bool>&>(getROAccessor().flag());
}


/// this method flush back the data to disk if there are any changes
void TableDataAccessor::sync() const
{
  if (itsVisNeedsFlush) {
      itsVisNeedsFlush = false;
      itsIterator.writeOriginalVis();
  }

  if (itsFlagNeedsFlush) {
      itsFlagNeedsFlush = false;
      itsIterator.writeOriginalFlag();
  }
}
