//
// Authors: John G. Shaw
// Revised: Dec. 31 2022
// Version: 1.0.1
//
// Copyright (C) 2022 University of Rochester. All rights reserved.
//
#ifndef GIZMO_HEADER_H5pio
#define GIZMO_HEADER_H5pio

#include "SysIncludes.h"

#include <hdf5.h>
#include <string>
#include <vector>

/*!
\verbatim
 *********************************************************************
 *
 * Simple PIC particle I/O
 *
 * resetFields()
 *   Zeros the list of registered particle-data arrays.
 *
 * registerParticles()
 *   The H5pio class points to particle arrays. The size of the
 *   arrays, is given by nParticles[6] as there are six types
 *   of particles allowed by GADGET/GIZMO.
 *
 * register{type}{dim}Field()
 *   Provides a name and pointer to an array of particle data for
 *   the current particle type specified by the last call to
 *   registerParticles().
 *
 *   The data can be node or cell centered, and be either a scalar
 *   of booleans, integers, or floats; or a triplet of floating-
 *   point numbers. The geometry field is a special case of a
 *   triple of floats, which specify the location of the particles.
 *   The difference is that the locations are treated as a type
 *   of "mesh" object in HDF5.
 *
 * openFiles()
 *   Opens all files needed to save a temporal sequence. This
 *   includes hdf5 and xdmf files. This function will replace
 *   the specified file's suffix by "hdmf" and "xdmf". If a
 *   suffix was not specified, as in "./data/myFiles", the
 *   suffixes will be added to the base name.
 *
 * closeFiles()
 *   Closes the files created by openFiles().
 *
 * saveFrame()
 *   A frame header is written based on the current status of the
 *   particle manager.
 *
 * loadFrame()
 *   The header is read for each frame, and the status flags are
 *   updated to reflect the frame's state.
 *
 *********************************************************************
\endverbatim
 */
class H5pio {
public:
  H5pio(void);
 ~H5pio(void);

  static void  initH5Library(void) { H5open();  }
  static void closeH5Library(void) { H5close(); }

  static const bool CENTER_BY_NODE= true;
  static const bool CENTER_BY_CELL= false;

  // from Gadget-2
  //
  static const int N_TYPES= 6;
  enum PARTICLE_TYPES {Gas, Halo, Disk, Buldge, Stars, Bndry};

  void resetFields(void);
  void registerParticles(const int nParticles, const int type); // type is in [0,5]

  void registerBoolean1DField (const bool isNodeCentered, string name, bool *ptr=nullptr);
  void registerInteger1DField (const bool isNodeCentered, string name, int *ptr=nullptr);
  void registerFloat1DField   (const bool isNodeCentered, string name, float *ptr=nullptr);
  void registerFloat3DField   (const bool isNodeCentered, string name, XcFloat3 *ptr=nullptr);
  void registerGeometry3DField(const bool isNodeCentered, string name, XcFloat3 *ptr=nullptr);

  int getNumberOfParticles(const int type);

  // *** consolidated file I/O ***************************************
  //
  // Combines HDF5/XDF5 files, with temporal support.
  //
  void  openFiles(XcCString fileName);
  void closeFiles(void);

  void saveFrame(const float time);
  void loadFrame(void);

  // *** HDF5 file I/O ***********************************************
  //
  void  openH5File(XcCString fileName, const bool createFile);
  void closeH5File(void);
  void saveH5Frame(const float time);
  void loadH5Frame(void);

  // *** XDMF file I/O ***********************************************
  //
  void  openXdmfFile(XcCString fileName="");
  void closeXdmfFile(void);
  void saveXdmfFrame(const float time);
  void skipXdmfFrames(const int nFrames);

// *** data **********************************************************
//
public: // particle data
  int nParticles[N_TYPES];
  int theParticleType;

  vector<int>    dataParticleType;
  vector<bool>   dataIsNodeCentered;
  vector<bool>   dataIsBoolean1D;
  vector<bool>   dataIsInteger1D;
  vector<bool>   dataIsFloat1D;
  vector<bool>   dataIsFloat3D;
  vector<bool>   dataIsGeometry3D;
  vector<string> dataName;
  vector<void*>  dataPointer;

  float frameTime;
  bool endOfFile;

private: // frame data
    int multiTemporalFrameID;
   char theBaseName[XCUDA_PATH_LENGTH];

private: // utilities
  bool isDot(const char c);
  bool isDelimiter(const char c);
  bool isDigit(const char c);
  void stripSuffix(XcString fileName);
  void stripID(XcString fileName);
  void addSuffix(XcString fileName, XcCString suffix);

private: // HDF5 support
   char hdf5Name[XCUDA_PATH_LENGTH];
   bool fileIsOpen;
  hid_t file_id;

  void writeDataset(hid_t group_id, hid_t type, int nItems, int dof, XcCString name, void* data);
  void  readDataset(hid_t group_id, hid_t type, XcCString name, void* data);

  void writeAttribute(hid_t group_id, hid_t type, XcCString name, void* data, int nDims=1);
  void  readAttribute(hid_t group_id, hid_t type, XcCString name, void* data);

private: // XDMF support
  char  xdmfFileName[XCUDA_PATH_LENGTH];
  bool  xdmfFileIsOpen;
   int  xdmfFrameID;
  FILE *xdmfFile;

  bool  writeXdmfTerminator;

  void writeXdmfAttributeBoolean1D(int np, XcCString partType, XcCString name, const bool isNodeCentered);
  void writeXdmfAttributeInteger1D(int np, XcCString partType, XcCString name, const bool isNodeCentered);
  void writeXdmfAttributeFloat1D(int np, XcCString partType, XcCString name, const bool isNodeCentered);
  void writeXdmfAttributeFloat3D(int np, XcCString partType, XcCString name, const bool isNodeCentered);
  void writeXdmfGeometry3D(int np, XcCString partType, XcCString name);

private: // support for switching between XDMF files
  struct {FILE *fp; bool isOpen; int frameID;} saveXdmfState;

  inline void pushXdmfState(void) {
    saveXdmfState= {xdmfFile,xdmfFileIsOpen,xdmfFrameID};
  }

  inline void popXdmfState(void) {
    xdmfFile= saveXdmfState.fp;
    xdmfFileIsOpen= saveXdmfState.isOpen;
    xdmfFrameID= saveXdmfState.frameID;
  }
};

// GIZMO_HEADER_H5pio
#endif
