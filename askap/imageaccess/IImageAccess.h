/// @file IImageAccess.h
/// @brief Basic interface to access an image
/// @details This interface class is somewhat analogous to casacore::ImageInterface. But it has
/// only methods we need for accessors and allow more functionality to access a part of the image.
/// In the future we can benefit from using this minimalistic interface because it should be
/// relatively easy to do parallel operations on the same image or even distributed storage.
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
///

#ifndef ASKAP_ACCESSORS_I_IMAGE_ACCESS_H
#define ASKAP_ACCESSORS_I_IMAGE_ACCESS_H

#include <string>

#include <casacore/casa/Arrays/Array.h>
#include <casacore/coordinates/Coordinates/CoordinateSystem.h>
#include <casacore/casa/Quanta/Quantum.h>


namespace askap {
namespace accessors {

/// @brief Basic interface to access an image
/// @details This interface class is somewhat analogous to casacore::ImageInterface. But it has
/// only methods we need for accessors and allow more functionality to access a part of the image.
/// In the future we can benefit from using this minimalistic interface because it should be
/// relatively easy to do parallel operations on the same image or even distributed storage.
/// @ingroup imageaccess
struct IImageAccess {
    /// @brief void virtual desctructor, to keep the compiler happy
    virtual ~IImageAccess();

    //////////////////
    // Reading methods
    //////////////////

    /// @brief obtain the shape
    /// @param[in] name image name
    /// @return full shape of the given image
    virtual casacore::IPosition shape(const std::string &name) const = 0;

    /// @brief read full image
    /// @param[in] name image name
    /// @return array with pixels
    virtual casacore::Array<float> read(const std::string &name) const = 0;

    /// @brief read part of the image
    /// @param[in] name image name
    /// @param[in] blc bottom left corner of the selection
    /// @param[in] trc top right corner of the selection
    /// @return array with pixels for the selection only
    virtual casacore::Array<float> read(const std::string &name, const casacore::IPosition &blc,
                                    const casacore::IPosition &trc) const = 0;

    /// @brief obtain coordinate system info
    /// @param[in] name image name
    /// @return coordinate system object
    virtual casacore::CoordinateSystem coordSys(const std::string &name) const = 0;

    /// @brief obtain coordinate system info for part of an image
    /// @param[in] name image name
    /// @return coordinate system object
    virtual casacore::CoordinateSystem coordSysSlice(const std::string &name, const casacore::IPosition &blc,
            const casacore::IPosition &trc) const = 0 ;
    /// @brief obtain beam info
    /// @param[in] name image name
    /// @return beam info vector
    virtual casacore::Vector<casacore::Quantum<double> > beamInfo(const std::string &name) const = 0;

    /// @brief obtain pixel units
    /// @param[in] name image name
    /// @return units string
    virtual std::string getUnits(const std::string &name) const = 0;

    /// @brief Get a particular keyword from the image metadata (A.K.A header)
    /// @details This reads a given keyword to the image metadata.
    /// @param[in] name Image name
    /// @param[in] keyword The name of the metadata keyword
    virtual std::string getMetadataKeyword(const std::string &name, const std::string &keyword) const = 0;

    //////////////////
    // Writing methods
    //////////////////

    /// @brief create a new image
    /// @details A call to this method should preceed any write calls. The actual
    /// image may be created only upon the first write call. Details depend on the
    /// implementation.
    /// @param[in] name image name
    /// @param[in] shape full shape of the image
    /// @param[in] csys coordinate system of the full image
    virtual void create(const std::string &name, const casacore::IPosition &shape,
                        const casacore::CoordinateSystem &csys) = 0;

    /// @brief write full image
    /// @param[in] name image name
    /// @param[in] arr array with pixels
    virtual void write(const std::string &name, const casacore::Array<float> &arr) = 0;

    /// @brief write a slice of an image
    /// @param[in] name image name
    /// @param[in] arr array with pixels
    /// @param[in] where bottom left corner where to put the slice to (trc is deduced from the array shape)
    virtual void write(const std::string &name, const casacore::Array<float> &arr,
                       const casacore::IPosition &where) = 0;

    /// @brief write a slice of an image pixel mask
    /// @param[in] name image name
    /// @param[in] arr array with pixels
    /// @param[in] where bottom left corner where to put the slice to (trc is deduced from the array shape)
    virtual void writeMask(const std::string &name, const casacore::Array<bool> &mask,
                           const casacore::IPosition &where) = 0;

    /// @brief write a slice of an image mask
    /// @param[in] name image name
    /// @param[in] arr array with pixels

    virtual void writeMask(const std::string &name, const casacore::Array<bool> &mask) = 0;
    /// @brief set brightness units of the image
    /// @details
    /// @param[in] name image name
    /// @param[in] units string describing brightness units of the image (e.g. "Jy/beam")
    virtual void setUnits(const std::string &name, const std::string &units) = 0;

    /// @brief set restoring beam info
    /// @details For the restored image we want to carry size and orientation of the restoring beam
    /// with the image. This method allows to assign this info.
    /// @param[in] name image name
    /// @param[in] maj major axis in radians
    /// @param[in] min minor axis in radians
    /// @param[in] pa position angle in radians
    virtual void setBeamInfo(const std::string &name, double maj, double min, double pa) = 0;

    /// @brief apply mask to image
    /// @details Deteails depend upon the implemenation - CASA images will have the pixel mask assigned
    /// but FITS images will have it applied to the pixels ... which is an irreversible process
    /// @param[in] name image name

    virtual void makeDefaultMask(const std::string &name) = 0;


    /// @brief Set a particular keyword for the metadata (A.K.A header)
    /// @details This adds a given keyword to the image metadata.
    /// @param[in] name Image name
    /// @param[in] keyword The name of the metadata keyword
    /// @param[in] value The value for the keyword, in string format
    /// @param[in] desc A description of the keyword
    virtual void setMetadataKeyword(const std::string &name, const std::string &keyword,
                                    const std::string value, const std::string &desc = "") = 0;


    /// @brief Add a HISTORY message to the image metadata
    /// @details Adds a string detailing the history of the image
    /// @param[in] name Image name
    /// @param[in] history History comment to add
    virtual void addHistory(const std::string &name, const std::string &history) = 0;


};

} // namespace accessors
} // namespace askap

#endif
