/***************************************************************************
 *   Copyright (C) 2011 by Jan Wagner                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
//===========================================================================
// SVN properties (DO NOT CHANGE)
//
// $Id$
// $HeadURL$
// $LastChangedRevision$
// $Author$
// $LastChangedDate$
//
//============================================================================

#ifndef _COVARIANCE_H
#define _COVARIANCE_H

#include "ArrayElements.h"

#include <armadillo>
#include <iostream>

namespace bf {

/** 
 * Storage for time-integrated covariance data in the
 * form of a 3D data cube (Nantennas x Nantennas x Nchannels).
 *
 * In case of Fourier domain data, the stored data would be
 * called spectral density matrices instead of covariances.
 */
class Covariance {

   friend std::ostream &operator<<(std::ostream&, Covariance const&);
   //friend int CovarianceModifier::templateSubtraction(Covariance&, arma::Col<int> const&, const int);
   friend class CovarianceModifier;

   private:

      Covariance();
      Covariance(const Covariance&);
      Covariance& operator= (const Covariance&);

   public:
      /**
       * C'stor, allocate space for covariances and set their starting timestamp
       * as well as integration time.
       * @param[in]   Nant       Number of elements or antennas.
       * @param[in]   Nchannels  Number of frequency channels.
       * @param[in]   Msmp       Number of sample vectors (x(t)'*x(t) matrices) that were averaged
       * @param[in]   timestamp  Starting time of the data cube.
       * @param[in]   Tint       Integration time used for the data cube.
       */
      Covariance(int Nant, int Nchannels, int Msmp, double timestamp, double Tint) : _N_ant(Nant), _N_chan(Nchannels), _M_smp(Msmp) { 
         _Rxx.zeros(Nant, Nant, Nchannels);
         _freqs.zeros(Nant);
         _timestamp = timestamp;
         _Tint = Tint;
      }

      /**
       * D'stor
       */
      ~Covariance() { }

   public:

      /**
       * Const accessor to data cube.
       * @return  Const reference to covariance data cube.
       */
      const arma::Cube<arma::cx_double>& get() const { return _Rxx; }

      /**
       * Writeable reference to data cube.
       * @return Reference to covariance data cube.
       */
      arma::Cube<arma::cx_double>& getWriteable() { return _Rxx; }
 
   public:

      /**
       * Load data cube contents from a memory location and
       * reorganize the memory layout if necessary.
       * @param[in]  raw_data  Pointer to data to load
       * @param[in]  format    Data format (0..N, to be defined)
       */
      void load(double* raw_data, const int format);

      /**
       * Load data cube contents from a file and
       * reorganize the memory layout if necessary.
       * @param[in]  fn        Input file name and path
       * @param[in]  format    Data format (0..N, to be defined)
       */
      void load(const char* fn, const int format);

      /**
       * Store data cube contents into a file.
       * @param[in]  fn        Output file name and path
       * @param[in]  format    Data format (0..N, to be defined)
       */
      void store(const char* fn, const int format) const;

   public:
    
      /**
       * Add an artificial signal to the covariance matrix
       * @param[in]  ch     Channel number
       * @param[in]  lambda Wavelength in meters
       * @param[in]  ae     ArrayElement object with element positions
       * @param[in]  phi    Azimuth angle of signal
       * @param[in]  theta  Tilt angle of plane wave normal from zenith
       * @param[in]  p      Signal power
       * @param[in]  Pna    Internal noise power (added to autocorrelations)
       * @param[in]  Pnc    Correlated noise power (added to cross and auto)
       */
      void addSignal(int ch, double lambda, ArrayElements const& ae, double phi, double theta, double P, double Pna, double Pnc);

      /**
       * Add an artificial signal to the covariance matrix, separating the
       * array into a set of normal elements and a set of RFI-only reference
       * antennas.
       * @param[in]  ch     Channel number
       * @param[in]  lambda Wavelength in meters
       * @param[in]  ae     ArrayElement object with element positions
       * @param[in]  phi    Azimuth angle of signal
       * @param[in]  theta  Tilt angle of plane wave normal from zenith
       * @param[in]  p      Signal power
       * @param[in]  Pna    Internal noise power (added to autocorrelations)
       * @param[in]  Pnc    Correlated noise power (added to cross and auto)
       * @param[in]  Gref   Reference antenna gain over array element gain
       * @param[in]  Iref   Vector with reference antenna indices between 0:(Nant-1)
       */
      void addSignal(int ch, double lambda, ArrayElements const& ae, double phi, double theta, double P, double Pna, double Pnc, 
                     double Gref, arma::Col<int> const& Iref);

   public:

      /**
       * Operator += for summing the data from another covariance
       * object into the data cube contained in this object.
       */
      Covariance& operator+= (const Covariance &rhs) {
         _Rxx += rhs._Rxx;
         _Tint += rhs._Tint;
         return *this;
      }

   private:
      int _N_ant;
      int _N_chan;
      int _M_smp;

   public:
      const int N_ant(void)  const { return _N_ant; }
      const int N_chan(void) const { return _N_chan; }
      const int M_smp(void)  const { return _M_smp; }

   private:
      arma::Cube<arma::cx_double> _Rxx;
      arma::Col<double> _freqs;
      double _timestamp;
      double _Tint;
};

extern std::ostream &operator<<(std::ostream&, Covariance const&);

} // namespace bf

#endif // _COVARIANCE_H
