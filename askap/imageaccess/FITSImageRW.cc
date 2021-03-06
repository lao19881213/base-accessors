/// @file FITSImageRW.cc
/// @brief Read/Write FITS image class
/// @details This class implements the write methods that are absent
/// from the casacore FITSImage.
///
///
/// @copyright (c) 2016 CSIRO
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
/// @author Stephen Ord <stephen.ord@csiro.au
///
#include <askap_accessors.h>
#include <askap/askap/AskapLogging.h>

#include <casacore/images/Images/FITSImage.h>
#include <casacore/casa/BasicSL/String.h>
#include <casacore/casa/Utilities/DataType.h>
#include <casacore/fits/FITS/fitsio.h>
#include <casacore/fits/FITS/FITSDateUtil.h>
#include <casacore/fits/FITS/FITSHistoryUtil.h>
#include <casacore/fits/FITS/FITSReader.h>

#include <casacore/casa/Quanta/MVTime.h>
#include <askap/imageaccess/FITSImageRW.h>

#include <fitsio.h>
#include <iostream>
#include <fstream>

ASKAP_LOGGER(FITSlogger, ".FITSImageRW");

void printerror(int status)
{
    /*****************************************************/
    /* Print out cfitsio error messages and exit program */
    /*****************************************************/

    char status_str[FLEN_STATUS];
    fits_get_errstatus(status, status_str);

    if (status) {
        ASKAPLOG_ERROR_STR(FITSlogger, "FitsIO error: " << status_str); /* print error report */

        exit(status);      /* terminate the program, returning error status */
    }
    return;
}

using namespace askap;
using namespace askap::accessors;

FITSImageRW::FITSImageRW(const std::string &name)
{
    std::string fullname = name + ".fits";
    this->name = std::string(name.c_str());
}
FITSImageRW::FITSImageRW()
{

}
bool FITSImageRW::create(const std::string &name, const casacore::IPosition &shape, \
                         const casacore::CoordinateSystem &csys, \
                         uint memoryInMB, bool preferVelocity, \
                         bool opticalVelocity, int BITPIX, float minPix, float maxPix, \
                         bool degenerateLast, bool verbose, bool stokesLast, \
                         bool preferWavelength, bool airWavelength, bool primHead, \
                         bool allowAppend, bool history)
{

    std::string fullname = name + ".fits";

    this->name = std::string(fullname.c_str());
    this->shape = shape;
    this->csys = csys;
    this->memoryInMB = memoryInMB;
    this->preferVelocity = preferVelocity;
    this->opticalVelocity = opticalVelocity;
    this->BITPIX = BITPIX;
    this->minPix = minPix;
    this->maxPix = maxPix;
    this->degenerateLast = degenerateLast;
    this->verbose = verbose ;
    this->stokesLast = stokesLast;
    this->preferWavelength = preferWavelength;
    this->airWavelength = airWavelength;
    this->primHead = primHead;
    this->allowAppend = allowAppend;
    this->history = history;

    ASKAPLOG_INFO_STR(FITSlogger, "Creating R/W FITSImage " << this->name);

    unlink(this->name.c_str());
    std::ofstream outfile(this->name.c_str());
    ASKAPCHECK(outfile.is_open(), "Cannot open FITS file for output");
    ASKAPLOG_INFO_STR(FITSlogger, "Created Empty R/W FITSImage " << this->name);
    ASKAPLOG_INFO_STR(FITSlogger, "Generating FITS header");


    casacore::String error;
    const casacore::uInt ndim = shape.nelements();
    // //
    // // Find scale factors
    // //
    casacore::Record header;
    casacore::Double b_scale, b_zero;
    ASKAPLOG_INFO_STR(FITSlogger, "Created blank FITS header");
    if (BITPIX == -32) {

        b_scale = 1.0;
        b_zero = 0.0;
        header.define("bitpix", BITPIX);
        header.setComment("bitpix", "Floating point (32 bit)");

    }

    else {
        error =
            "BITPIX must be -32 (floating point)";
        return false;
    }
    ASKAPLOG_INFO_STR(FITSlogger, "Added BITPIX");
    //
    // At this point, for 32 floating point, we must apply the given
    // mask.  For 16bit, we may know that there are in fact no blanks
    // in the image, so we can dispense with looking at the mask again.


    //
    casacore::Vector<casacore::Int> naxis(ndim);
    casacore::uInt i;
    for (i = 0; i < ndim; i++) {
        naxis(i) = shape(i);
    }
    header.define("naxis", naxis);

    ASKAPLOG_INFO_STR(FITSlogger, "Added NAXES");
    if (allowAppend)
        header.define("extend", casacore::True);
    if (!primHead) {
        header.define("PCOUNT", 0);
        header.define("GCOUNT", 1);
    }
    ASKAPLOG_INFO_STR(FITSlogger, "Extendable");

    header.define("bscale", b_scale);
    header.setComment("bscale", "PHYSICAL = PIXEL*BSCALE + BZERO");
    header.define("bzero", b_zero);
    ASKAPLOG_INFO_STR(FITSlogger, "BSCALE");

    header.define("COMMENT1", ""); // inserts spaces
    // I should FITS-ize the units

    header.define("BUNIT", "Jy");
    header.setComment("BUNIT", "Brightness (pixel) unit");
    //
    ASKAPLOG_INFO_STR(FITSlogger, "BUINT");
    casacore::IPosition shapeCopy = shape;
    casacore::CoordinateSystem cSys = csys;

    casacore::Record saveHeader(header);
    ASKAPLOG_INFO_STR(FITSlogger, "Saved header");
    casacore::Bool ok = cSys.toFITSHeader(header, shapeCopy, casacore::True, 'c', casacore::True, // use WCS
                                      preferVelocity, opticalVelocity,
                                      preferWavelength, airWavelength);
    if (!ok) {
        ASKAPLOG_WARN_STR(FITSlogger, "Could not make a standard FITS header. Setting" \
                          <<  " a simple linear coordinate system.") ;

        casacore::uInt n = cSys.nWorldAxes();
        casacore::Matrix<casacore::Double> pc(n, n); pc = 0.0; pc.diagonal() = 1.0;
        casacore::LinearCoordinate linear(cSys.worldAxisNames(),
                                      cSys.worldAxisUnits(),
                                      cSys.referenceValue(),
                                      cSys.increment(),
                                      cSys.linearTransform(),
                                      cSys.referencePixel());
        casacore::CoordinateSystem linCS;
        linCS.addCoordinate(linear);

        // Recover old header before it got mangled by toFITSHeader

        header = saveHeader;
        casacore::IPosition shapeCopy = shape;
        casacore::Bool ok = linCS.toFITSHeader(header, shapeCopy, casacore::True, 'c', casacore::False); // don't use WCS
        if (!ok) {
            ASKAPLOG_WARN_STR(FITSlogger, "Fallback linear coordinate system fails also.");
            return false;
        }
    }
    ASKAPLOG_INFO_STR(FITSlogger, "Added coordinate system");
    // When this if test is True, it means some pixel axes had been removed from
    // the coordinate system and degenerate axes were added.

    if (naxis.nelements() != shapeCopy.nelements()) {
        naxis.resize(shapeCopy.nelements());
        for (casacore::uInt j = 0; j < shapeCopy.nelements(); j++) {
            naxis(j) = shapeCopy(j);
        }
        header.define("NAXIS", naxis);
    }

    //
    // DATE
    //

    casacore::String date, timesys;
    casacore::Time nowtime;
    casacore::MVTime now(nowtime);
    casacore::FITSDateUtil::toFITS(date, timesys, now);
    header.define("date", date);
    header.setComment("date", "Date FITS file was written");
    if (!header.isDefined("timesys") && !header.isDefined("TIMESYS")) {
        header.define("timesys", timesys);
        header.setComment("timesys", "Time system for HDU");
    }

    ASKAPLOG_INFO_STR(FITSlogger, "Added date");
    // //
    // // ORIGIN
    // //

    header.define("ORIGIN", "ASKAPsoft");

    theKeywordList = casacore::FITSKeywordUtil::makeKeywordList(primHead, casacore::True);

    //kw.mk(FITS::EXTEND, True, "Tables may follow");
    // add the general keywords for WCS and so on
    ok = casacore::FITSKeywordUtil::addKeywords(theKeywordList, header);
    if (! ok) {
        error = "Error creating initial FITS header";
        return false;
    }


    //
    // END
    //

    theKeywordList.end();
    ASKAPLOG_INFO_STR(FITSlogger, "All keywords created ... adding to file");
    // now get them into a file ...

    theKeywordList.first();
    theKeywordList.next(); // skipping an extra SIMPLE... hack
    casacore::FitsKeyCardTranslator m_kc;
    const size_t cards_size = 2880 * 4;
    char cards[cards_size];
    memset(cards, 0, sizeof(cards));
    while (1) {
        if (m_kc.build(cards, theKeywordList)) {

            outfile << cards;
            memset(cards, 0, sizeof(cards));
        } else {
            if (cards[0] != 0) {
                outfile << cards;
            }
            break;
        }

    }
    // outfile << cards;
    ASKAPLOG_INFO_STR(FITSlogger, "All keywords added to file");
    try {
      outfile.close();
      ASKAPLOG_INFO_STR(FITSlogger, "Outfile closed");
    }
    catch (...) {
      ASKAPLOG_WARN_STR(FITSlogger, "Failed to properly close outfile");
      return false;
    }



    return true;

}
void FITSImageRW::print_hdr()
{
    fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */

    int status, nkeys, keypos, hdutype, ii, jj;
    char card[FLEN_CARD];   /* standard string lengths defined in fitsioc.h */

    status = 0;

    if (fits_open_file(&fptr, this->name.c_str(), READONLY, &status))
        printerror(status);

    /* attempt to move to next HDU, until we get an EOF error */
    for (ii = 1; !(fits_movabs_hdu(fptr, ii, &hdutype, &status)); ii++) {
        /* get no. of keywords */
        if (fits_get_hdrpos(fptr, &nkeys, &keypos, &status))
            printerror(status);

        printf("Header listing for HDU #%d:\n", ii);
        for (jj = 1; jj <= nkeys; jj++)  {
            if (fits_read_record(fptr, jj, card, &status))
                printerror(status);

            printf("%s\n", card); /* print the keyword card */
        }
        printf("END\n\n");  /* terminate listing with END */
    }

    if (status == END_OF_FILE)   /* status values are defined in fitsioc.h */
        status = 0;              /* got the expected EOF error; reset = 0  */
    else
        printerror(status);       /* got an unexpected error                */

    if (fits_close_file(fptr, &status))
        printerror(status);

    return;

}
bool FITSImageRW::write(const casacore::Array<float> &arr)
{
    ASKAPLOG_INFO_STR(FITSlogger, "Writing array to FITS image");
    fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */


    int status;


    status = 0;

    if (fits_open_file(&fptr, this->name.c_str(), READWRITE, &status))
        printerror(status);

    long fpixel = 1;                               /* first pixel to write      */
    size_t nelements = arr.nelements();          /* number of pixels to write */
    bool deleteIt;
    const float *data = arr.getStorage(deleteIt);
    void *dataptr = (void *) data;

    /* write the array of unsigned integers to the FITS file */
    if (fits_write_img(fptr, TFLOAT, fpixel, nelements, dataptr, &status))
        printerror(status);

    if (fits_close_file(fptr, &status))
        printerror(status);

    return true;
}


bool FITSImageRW::write(const casacore::Array<float> &arr, const casacore::IPosition &where)
{
    ASKAPLOG_INFO_STR(FITSlogger, "Writing array to FITS image at (Cindex)" << where);
    fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */

    int status, hdutype;


    status = 0;

    if (fits_open_file(&fptr, this->name.c_str(), READWRITE, &status))
        printerror(status);

    if (fits_movabs_hdu(fptr, 1, &hdutype, &status))
        printerror(status);

    // get the dimensionality & size of the fits file.
    int naxes;
    if (fits_get_img_dim(fptr, &naxes, &status)) {
        printerror(status);
    }
    long *axes = new long[naxes];
    if (fits_get_img_size(fptr, naxes, axes, &status)) {
        printerror(status);
    }

    ASKAPCHECK(where.nelements() == naxes,
               "Mismatch in dimensions - FITS file has " << naxes
               << " axes, while requested location has " << where.nelements());

    long fpixel[4], lpixel[4];
    int array_dim = arr.shape().nelements();
    int location_dim = where.nelements();
    ASKAPLOG_INFO_STR(FITSlogger, "There are " << array_dim << " dimensions in the slice");
    ASKAPLOG_INFO_STR(FITSlogger," There are " << location_dim << " dimensions in the place");

    fpixel[0] = where[0] + 1;
    lpixel[0] = where[0] + arr.shape()[0];
    ASKAPLOG_INFO_STR(FITSlogger, "fpixel[0] = " << fpixel[0] << ", lpixel[0] = " << lpixel[0]);
    fpixel[1] = where[1] + 1;
    lpixel[1] = where[1] + arr.shape()[1];
    ASKAPLOG_INFO_STR(FITSlogger, "fpixel[1] = " << fpixel[1] << ", lpixel[1] = " << lpixel[1]);

    if (array_dim == 2 && location_dim == 3) {
        ASKAPLOG_INFO_STR(FITSlogger,"Writing a single slice into an array");
        fpixel[2] = where[2] + 1;
        lpixel[2] = where[2] + 1;
//        lpixel[2] = where[2] + arr.shape()[2];
        ASKAPLOG_INFO_STR(FITSlogger, "fpixel[2] = " << fpixel[2] << ", lpixel[2] = " << lpixel[2]);
    }
    else if (array_dim == 3 && location_dim == 3) {
        ASKAPLOG_INFO_STR(FITSlogger,"Writing more than 1 slice into the array");
        fpixel[2] = where[2] + 1;
        lpixel[2] = where[2] + arr.shape()[2];
        ASKAPLOG_INFO_STR(FITSlogger, "fpixel[2] = " << fpixel[2] << ", lpixel[2] = " << lpixel[2]);
    }
    else if (array_dim == 3 && location_dim == 4) {
        fpixel[2] = where[2] + 1;
        lpixel[2] = where[2] + arr.shape()[2];
        fpixel[3] = where[3] + 1;
        // lpixel[3] = where[3] + arr.shape()[3];
        lpixel[3] = where[3] + 1;
        ASKAPLOG_INFO_STR(FITSlogger, "fpixel[2] = " << fpixel[2] << ", lpixel[2] = " << lpixel[2]);
        ASKAPLOG_INFO_STR(FITSlogger, "fpixel[3] = " << fpixel[3] << ", lpixel[3] = " << lpixel[3]);
    }
    else if (array_dim == 4 && location_dim == 4) {

        fpixel[2] = where[2] + 1;
        lpixel[2] = where[2] + arr.shape()[2];
        fpixel[3] = where[3] + 1;
        lpixel[3] = where[3] + arr.shape()[3];

        ASKAPLOG_INFO_STR(FITSlogger, "fpixel[2] = " << fpixel[2] << ", lpixel[2] = " << lpixel[2]);
        ASKAPLOG_INFO_STR(FITSlogger, "fpixel[3] = " << fpixel[3] << ", lpixel[3] = " << lpixel[3]);
    }


    int64_t nelements = arr.nelements();          /* number of pixels to write */

    ASKAPLOG_INFO_STR(FITSlogger, "We are writing " << nelements << " elements");
    bool deleteIt = false;
    const float *data = arr.getStorage(deleteIt);
    float *dataptr = (float *) data;

    // status = 0;

    // if ( fits_write_pix(fptr, TFLOAT,fpixel, nelements, dataptr, &status) )
    //     printerror( status );

    status = 0;
    long group = 0;

    if (fits_write_subset_flt(fptr, group, naxes, axes, fpixel, lpixel, dataptr, &status))
        printerror(status);

    ASKAPLOG_INFO_STR(FITSlogger, "Written " << nelements << " elements");
    status = 0;

    if (fits_close_file(fptr, &status))
        printerror(status);

    delete [] axes;

    return true;

}
void FITSImageRW::setUnits(const std::string &units)
{
    ASKAPLOG_INFO_STR(FITSlogger, "Updating brightness units");
    fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
    int status = 0;

    if (fits_open_file(&fptr, this->name.c_str(), READWRITE, &status))
        printerror(status);

    if (fits_update_key(fptr, TSTRING, "BUNIT", (void *)(units.c_str()),
                        "Brightness (pixel) unit", &status))
        printerror(status);

    if (fits_close_file(fptr, &status))
        printerror(status);

}

void FITSImageRW::setHeader(const std::string &keyword, const std::string &value, const std::string &desc)
{
    ASKAPLOG_INFO_STR(FITSlogger, "Setting header value for " << keyword);
    fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
    int status = 0;
    if (fits_open_file(&fptr, this->name.c_str(), READWRITE, &status))
        printerror(status);


    if (fits_update_key(fptr, TSTRING, keyword.c_str(), (char *)value.c_str(),
                        desc.c_str(), &status))
        printerror(status);

    if (fits_close_file(fptr, &status))
        printerror(status);


}

void FITSImageRW::setRestoringBeam(double maj, double min, double pa)
{
    ASKAPLOG_INFO_STR(FITSlogger, "Setting Beam info");
    ASKAPLOG_INFO_STR(FITSlogger, "Updating brightness units");
    fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
    int status = 0;
    double radtodeg = 360. / (2 * M_PI);
    if (fits_open_file(&fptr, this->name.c_str(), READWRITE, &status))
        printerror(status);

    double value = radtodeg * maj;
    if (fits_update_key(fptr, TDOUBLE, "BMAJ", &value,
                        "Restoring beam major axis", &status))
        printerror(status);
    value = radtodeg * min;
    if (fits_update_key(fptr, TDOUBLE, "BMIN", &value,
                        "Restoring beam minor axis", &status))
        printerror(status);
    value = radtodeg * pa;
    if (fits_update_key(fptr, TDOUBLE, "BPA", &value,
                        "Restoring beam position angle", &status))
        printerror(status);
    if (fits_update_key(fptr, TSTRING, "BTYPE", (void *) "Intensity",
                        " ", &status))
        printerror(status);

    if (fits_close_file(fptr, &status))
        printerror(status);

}

void FITSImageRW::addHistory(const std::string &history)
{

    ASKAPLOG_INFO_STR(FITSlogger,"Adding HISTORY string: " << history);
    fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
    int status = 0;
    if ( fits_open_file(&fptr, this->name.c_str(), READWRITE, &status) )
        printerror( status );

    if ( fits_write_history(fptr, history.c_str(), &status) )
        printerror( status );

    if ( fits_close_file(fptr, &status) )
        printerror( status );

}



FITSImageRW::~FITSImageRW()
{
}
