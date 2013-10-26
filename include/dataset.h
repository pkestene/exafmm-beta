#ifndef dataset_h
#define dataset_h
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include "types.h"

class Dataset {                                                 // Contains all the different datasets
 private:
  long filePosition;                                            // Position of file stream

 private:
  //! Split range and return partial range
  void splitRange(int &begin, int &end, int iSplit, int numSplit) {
    assert(end > begin);                                        // Check that size > 0
    int size = end - begin;                                     // Size of range
    int increment = size / numSplit;                            // Increment of splitting
    int remainder = size % numSplit;                            // Remainder of splitting
    begin += iSplit * increment + std::min(iSplit,remainder);   // Increment the begin counter
    end = begin + increment;                                    // Increment the end counter
    if (remainder > iSplit) end++;                              // Adjust the end counter for remainder
  }

//! Uniform distribution on [-1,1]^3 lattice
  Bodies lattice(int numBodies, int mpirank, int mpisize) {
    int nx = int(std::pow(numBodies*mpisize, 1./3));            // Number of points in x direction
    int ny = nx;                                                // Number of points in y direction
    int nz = nx;                                                // Number of points in z direction
    int begin = 0;                                              // Begin index in z direction
    int end = nz;                                               // End index in z direction
    splitRange(begin, end, mpirank, mpisize);                   // Split range in z direction
    int numLattice = nx * ny * (end - begin);                   // Total number of lattice points
    Bodies bodies(numLattice);                                  // Initialize bodies
    B_iter B = bodies.begin();                                  // Initialize body iterator
    for (int ix=0; ix<nx; ix++) {                               // Loop over x direction
      for (int iy=0; iy<ny; iy++) {                             //  Loop over y direction
        for (int iz=begin; iz<end; iz++, B++) {                 //   Loop over z direction
          B->X[0] = (ix / real_t(nx-1)) * 2 - 1;                //    x coordinate
          B->X[1] = (iy / real_t(ny-1)) * 2 - 1;                //    y coordinate
          B->X[2] = (iz / real_t(nz-1)) * 2 - 1;                //    z coordinate
        }                                                       //   End loop over z direction
      }                                                         //  End loop over y direction
    }                                                           // End loop over x direction
    return bodies;                                              // Return bodies
  }

//! Random distribution in [-1,1]^3 cube
  Bodies cube(int numBodies, int seed, int numSplit) {
    Bodies bodies(numBodies);                                   // Initialize bodies
    for (int i=0; i<numSplit; i++, seed++) {                    // Loop over partitions (if there are any)
      int begin = 0;                                            //  Begin index of bodies
      int end = bodies.size();                                  //  End index of bodies
      splitRange(begin, end, i, numSplit);                      //  Split range of bodies
      srand48(seed);                                            //  Set seed for random number generator
      for (B_iter B=bodies.begin()+begin; B!=bodies.begin()+end; B++) {// Loop over bodies
	for (int d=0; d<3; d++) {                               //   Loop over dimension
	  B->X[d] = drand48() * 2 * M_PI - M_PI;                //    Initialize positions
	}                                                       //   End loop over dimension
      }                                                         //  End loop over bodies
    }                                                           // End loop over partitions
    return bodies;                                              // Return bodies
  }

//! Random distribution on r = 1 sphere
  Bodies sphere(int numBodies, int seed, int numSplit) {
    Bodies bodies(numBodies);                                   // Initialize bodies
    for (int i=0; i<numSplit; i++, seed++) {                    // Loop over partitions (if there are any)
      int begin = 0;                                            //  Begin index of bodies
      int end = bodies.size();                                  //  End index of bodies
      splitRange(begin, end, i, numSplit);                      //  Split range of bodies
      srand48(seed);                                            //  Set seed for random number generator
      for (B_iter B=bodies.begin()+begin; B!=bodies.begin()+end; B++) {// Loop over bodies
	for (int d=0; d<3; d++) {                               //   Loop over dimension
	  B->X[d] = drand48() * 2 - 1;                          //    Initialize positions
	}                                                       //   End loop over dimension
	real_t r = std::sqrt(norm(B->X));                       //   Distance from center
	for (int d=0; d<3; d++) {                               //   Loop over dimension
	  B->X[d] /= r * 1.1;                                   //    Normalize positions
	}                                                       //   End loop over dimension
      }                                                         //  End loop over bodies
    }                                                           // End loop over partitions
    return bodies;                                              // Return bodies
  }

//! Plummer distribution in a r = M_PI/2 sphere
  Bodies plummer(int numBodies, int seed, int numSplit) {
    Bodies bodies(numBodies);                                   // Initialize bodies
    for (int i=0; i<numSplit; i++, seed++) {                    // Loop over partitions (if there are any)
      int begin = 0;                                            //  Begin index of bodies
      int end = bodies.size();                                  //  End index of bodies
      splitRange(begin, end, i, numSplit);                      //  Split range of bodies
      srand48(seed);                                            //  Set seed for random number generator
      for (B_iter B=bodies.begin()+begin; B!=bodies.begin()+end; B++) {// Loop over bodies
	for (int d=0; d<3; d++) B->X[d] = drand48() * 2 - 1;    //   Generate random number between [-1,1]
	real_t R2 = std::pow(drand48()*.999, -2.0/3.0) - 1;     //   Generate distribution radius squared
	real_t rscale = 0.015 * M_PI / std::sqrt(norm(B->X) * R2);// Scaling to fit in 0.015 * M_PI sphere
	for (int d=0; d<3; d++) B->X[d] *= rscale;              //   Rescale particle coordinates
      }                                                         //  End loop over bodies
    }                                                           // End loop over partitions
    return bodies;                                              // Return bodies
  }

 public:
//! Constructor
  Dataset() : filePosition(0) {}

//! Initialize source values
  void initSource(Bodies &bodies, int seed, int numSplit) {
    for (int i=0; i<numSplit; i++, seed++) {                    // Loop over partitions (if there are any)
      int begin = 0;                                            //  Begin index of bodies
      int end = bodies.size();                                  //  End index of bodies
      splitRange(begin, end, i, numSplit);                      //  Split range of bodies
      srand48(seed);                                            //  Set seed for random number generator
#if MASS
      for (B_iter B=bodies.begin()+begin; B!=bodies.begin()+end; B++) {// Loop over bodies
	B->SRC = 1. / bodies.size();                            //   Initialize mass
      }                                                         //  End loop over bodies
#else
      real_t average = 0;                                       //  Initialize average charge
      for (B_iter B=bodies.begin()+begin; B!=bodies.begin()+end; B++) {// Loop over bodies
	B->SRC = drand48() - .5;                                //   Initialize charge
	average += B->SRC;                                      //   Accumulate average
      }                                                         //  End loop over bodies
      average /= (end - begin);                                 //  Normalize average
      for (B_iter B=bodies.begin()+begin; B!=bodies.begin()+end; B++) {// Loop over bodies
	B->SRC -= average;                                      //   Subtract average charge
      }                                                         //  End loop over bodies
#endif
    }                                                           // End loop over partitions
  }

//! Initialize target values
  void initTarget(Bodies &bodies) {
    for (B_iter B=bodies.begin(); B!=bodies.end(); B++) {       // Loop over bodies
      B->TRG = 0;                                               //  Clear target values
      B->IBODY = B-bodies.begin();                              //  Set initial body numbering
    }                                                           // End loop over bodies
  }

//! Initialize dsitribution, source & target value of bodies
  Bodies initBodies(int numBodies, const char * distribution,
		  int mpirank=0, int mpisize=1, int numSplit=1) {
    Bodies bodies;                                              // Initialize bodies
    switch (distribution[0]) {                                  // Switch between data distribution type
    case 'l':                                                   // Case for lattice
      bodies = lattice(numBodies,mpirank,mpisize);              //  Uniform distribution on [-1,1]^3 lattice
      break;                                                    // End case for lattice
    case 'c':                                                   // Case for cube
      bodies = cube(numBodies,mpirank,numSplit);                //  Random distribution in [-1,1]^3 cube
      break;                                                    // End case for cube
    case 's':                                                   // Case for sphere
      bodies = sphere(numBodies,mpirank,numSplit);              //  Random distribution on surface of r = 1 sphere
      break;                                                    // End case for sphere
    case 'p':                                                   // Case plummer
      bodies = plummer(numBodies,mpirank,numSplit);             //  Plummer distribution in a r = M_PI/2 sphere
      break;                                                    // End case for plummer
    default:                                                    // If none of the above
      fprintf(stderr, "Unknown data distribution %s\n", distribution);// Print error message
    }                                                           // End switch between data distribution type
    initSource(bodies,mpirank,numSplit);                        // Initialize source values
    initTarget(bodies);                                         // Initialize target values
    return bodies;                                              // Return bodies
  }

//! Read target values from file
  void readTarget(Bodies &bodies, int mpirank) {
    std::stringstream name;                                     // File name
    name << "direct" << std::setfill('0') << std::setw(4)       // Set format
         << mpirank << ".dat";                                  // Create file name for saving direct calculation values
    std::ifstream file(name.str().c_str(),std::ios::in | std::ios::binary);// Open file
    file.seekg(filePosition);                                   // Set position in file
    for (B_iter B=bodies.begin(); B!=bodies.end(); B++) {       // Loop over bodies
      file >> B->TRG[0];                                        //  Read data for potential
      file >> B->TRG[1];                                        //  Read data for x acceleration
      file >> B->TRG[2];                                        //  Read data for y acceleration
      file >> B->TRG[3];                                        //  Read data for z acceleration
    }                                                           // End loop over bodies
    filePosition = file.tellg();                                // Get position in file
    file.close();                                               // Close file
  }

//! Write target values to file
  void writeTarget(Bodies &bodies, int mpirank) {
    std::stringstream name;                                     // File name
    name << "direct" << std::setfill('0') << std::setw(4)       // Set format
         << mpirank << ".dat";                                  // Create file name for saving direct calculation values
    std::ofstream file(name.str().c_str(),std::ios::out | std::ios::app | std::ios::binary);// Open file
    for (B_iter B=bodies.begin(); B!=bodies.end(); B++) {       // Loop over bodies
      file << B->TRG[0] << std::endl;                           //  Write data for potential
      file << B->TRG[1] << std::endl;                           //  Write data for x acceleration
      file << B->TRG[2] << std::endl;                           //  Write data for y acceleration
      file << B->TRG[3] << std::endl;                           //  Write data for z acceleration
    }                                                           // End loop over bodies
    file.close();                                               // Close file
  }

//! Downsize target bodies by even sampling
  void sampleBodies(Bodies &bodies, int numTargets) {
    if (numTargets < int(bodies.size())) {                      // If target size is smaller than current
      int stride = bodies.size() / numTargets;                  //  Stride of sampling
      for (int i=0; i<numTargets; i++) {                        //  Loop over target samples
        bodies[i] = bodies[i*stride];                           //   Sample targets
      }                                                         //  End loop over target samples
      bodies.resize(numTargets);                                //  Resize bodies to target size
    }                                                           // End if for target size
  }

//! Get bodies with positive charges
  Bodies getPositive(Bodies &bodies) {
    Bodies buffer = bodies;                                     // Copy bodies to buffer
    B_iter B2 = buffer.begin();                                 // Initialize iterator for buffer
    for (B_iter B=bodies.begin(); B!=bodies.end(); B++) {       // Loop over bodies
      if (B->SRC >= 0) {                                        //  If source is positive
        *B2 = *B;                                               //   Copy data to buffer
        B2++;                                                   //   Increment iterator
      }                                                         //  End if for positive source
    }                                                           // End loop over bodies
    buffer.resize(B2-buffer.begin());                           // Resize buffer
    return buffer;                                              // Return buffer
  }


//! Get bodies with negative charges
  Bodies getNegative(Bodies &bodies) {
    Bodies buffer = bodies;                                     // Copy bodies to buffer
    B_iter B2 = buffer.begin();                                 // Initialize iterator for buffer
    for (B_iter B=bodies.begin(); B!=bodies.end(); B++) {       // Loop over bodies
      if (B->SRC < 0) {                                         //  If source is negative
        *B2 = *B;                                               //   Copy data to buffer
        B2++;                                                   //   Increment iterator
      }                                                         //  End if for negative source
    }                                                           // End loop over bodies
    buffer.resize(B2-buffer.begin());                           // Resize buffer
    return buffer;                                              // Return buffer
  }
};
#endif
