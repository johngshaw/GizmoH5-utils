//
// Authors: John G. Shaw
// Revised: Nov. 25 2021
// Version: 3.4.8
//
// Copyright (C) 2021 University of Rochester. All rights reserved.
//
#ifndef TFLINK_HEADER_SysIncludes
#define TFLINK_HEADER_SysIncludes

// Use C form of standard libraries
//
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// For the "new (std::nothrow)" operator
//
#include <new>

// For system queries
//
#include <unistd.h>



// For OpenMP calls
//
#ifdef HAS_OMP
  #include <omp.h>
#endif

// The order of these headers matter.
// Do not change unless you know what you are doing!
//
#ifdef HAS_XCUDA
  #include "XCuda.h"
  using namespace XCuda;
#endif

// XCut is always needed, even if Cuda is not present.
//
#ifdef HAS_XCUT
  #include "XCut.h"
  using namespace XCuda;
#endif

// OPENCL_VERSION should be defined somewhere else!
//
// This is for debugging. Cuda includes an old version
// of OpenCL which is not sufficient for full TFLink
// operation. This allows a minimal test of compiling
// and linking the "Kernel_opencl.cpp" functions.
//
#ifndef OPENCL_VERSION
  #ifdef HAS_CUDA
    #define OPENCL_VERSION "1.2.0"
  #endif
#endif



// Define this macro to disable MPI calls (temporary!)
//
#define DISABLE_MPI

// Removes warning from fftw3-mpi (is this safe?)
//
#ifdef HAS_MPI

  #define MPICH_IGNORE_CXX_SEEK
  #include "mpi.h"

  // Allows use of MPI_COMPLEX types with newer versions of MPI
  //
  #if (!defined(MPI_VERSION) || MPI_VERSION < 2 || (MPI_VERSION == 2 && MPI_SUBVERSION < 2))
    // avoids all sorts of nasty warnings
    #undef MPI_C_COMPLEX
    #undef MPI_C_FLOAT_COMPLEX
    #undef MPI_C_DOUBLE_COMPLEX

    #define MPI_C_COMPLEX MPI_COMPLEX
    #define MPI_C_FLOAT_COMPLEX MPI_COMPLEX
    #define MPI_C_DOUBLE_COMPLEX MPI_DOUBLE_COMPLEX
  #endif

#endif

const int MPI_ROOT_NODE= 0;
const int MAX_IO_LINE_LENGTH= 50;



// Utility functions for common MPI I/O
//
static XcString printEnabled(bool flag)
{
  return XcString(flag ? "enabled" : "disabled");
  //
} // end printEnabled()

static XcString printBoolean(bool flag)
{
  return XcString(flag ? "true" : "false");
  //
} // end printBoolean()


// end TFLINK_HEADER_SysIncludes
#endif
