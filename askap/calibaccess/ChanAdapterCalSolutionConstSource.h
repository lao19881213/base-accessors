/// @file
/// @brief An adapter to adjust channel number                           
/// @details This adapter is handy if one needs to add a fixed offset to
/// channel numbers in the requested bandpass solution. It is not clear 
/// whether we want this class to stay long term (it is largely intended
/// for situations where the design was not very good and ideally we need
/// to redesign the code rather than do it quick and dirty way via the adapter).
///
/// @copyright (c) 2011 CSIRO
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
/// @author Max Voronkov <Maxim.Voronkov@csiro.au>

#ifndef ASKAP_ACCESSORS_CHAN_ADAPTER_CAL_SOLUTION_CONST_SOURCE_H
#define ASKAP_ACCESSORS_CHAN_ADAPTER_CAL_SOLUTION_CONST_SOURCE_H

#include <askap/calibaccess/ICalSolutionConstSource.h>

namespace askap {

namespace accessors {

/// @brief An adapter to adjust channel number                           
/// @details This adapter is handy if one needs to add a fixed offset to
/// channel numbers in the requested bandpass solution. It is not clear 
/// whether we want this class to stay long term (it is largely intended
/// for situations where the design was not very good and ideally we need
/// to redesign the code rather than do it quick and dirty way via the adapter).
/// @ingroup calibaccess
struct ChanAdapterCalSolutionConstSource  : public ICalSolutionConstSource {

   /// @brief set up the adapter
   /// @details The constructor sets the shared pointer to the source which
   /// is wrapped around and the channel offset
   /// @param[in] src shared pointer to the original source
   /// @param[in] offset channel offset to add to bandpass value request
   ChanAdapterCalSolutionConstSource(const boost::shared_ptr<ICalSolutionConstSource> &src, const casacore::uInt offset);
  
   /// @brief obtain ID for the most recent solution
   /// @return ID for the most recent solution
   virtual long mostRecentSolution() const;
  
   /// @brief obtain solution ID for a given time
   /// @details This method looks for a solution valid at the given time
   /// and returns its ID. It is equivalent to mostRecentSolution() if
   /// called with a time sufficiently into the future.
   /// @param[in] time time stamp in seconds since MJD of 0.
   /// @return solution ID
   virtual long solutionID(const double time) const;
  
   /// @brief obtain read-only accessor for a given solution ID
   /// @details This method returns a shared pointer to the solution accessor, which
   /// can be used to read the parameters. If a solution with the given ID doesn't 
   /// exist, an exception is thrown. Existing solutions with undefined parameters 
   /// are managed via validity flags of gains, leakages and bandpasses
   /// @param[in] id solution ID to read
   /// @return shared pointer to an accessor object
   virtual boost::shared_ptr<ICalSolutionConstAccessor> roSolution(const long id) const;
  
   /// @brief shared pointer definition
   typedef boost::shared_ptr<ChanAdapterCalSolutionConstSource> ShPtr;
private:
  
   /// @brief original source
   const boost::shared_ptr<ICalSolutionConstSource> itsSource;

   /// @brief channel offset
   const casacore::uInt itsOffset;

};

} // namespace accessors

} // namespace askap

#endif // #ifndef ASKAP_ACCESSORS_CHAN_ADAPTER_CAL_SOLUTION_CONST_SOURCE_H

