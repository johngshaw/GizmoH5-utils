//
// Authors: John G. Shaw
// Revised: Dec. 31 2022
// Version: 1.0.1
//
// Copyright (C) 2022 University of Rochester. All rights reserved.
//
#include "H5pio.h"
#include <libgen.h>

H5pio::H5pio(void)
{
  frameTime= 0.0f;
  endOfFile= false;

  fileIsOpen= false;
  file_id= 0;

  xdmfFileIsOpen= false;
  xdmfFrameID= 0;
  xdmfFile= nullptr;

  writeXdmfTerminator= true;

  resetFields();
}

H5pio::~H5pio(void)
{
  closeFiles();
}


void H5pio::resetFields(void)
{  
  for (int i=0; i<N_TYPES; i++) nParticles[i]= 0;
  theParticleType= 0;

  vector<int>().swap(dataParticleType);
  vector<bool>().swap(dataIsNodeCentered);
  vector<bool>().swap(dataIsBoolean1D);
  vector<bool>().swap(dataIsInteger1D);
  vector<bool>().swap(dataIsFloat1D);
  vector<bool>().swap(dataIsFloat3D);
  vector<bool>().swap(dataIsGeometry3D);
  vector<string>().swap(dataName);
  vector<void*>().swap(dataPointer);
}


void H5pio::registerParticles(const int np, const int type)
{
  XcHandleError(np<=0,XCUDA_ERROR,"H5pio::registerParticles","nParticles <= 0");
  XcHandleError(type<0||type>5, XCUDA_ERROR,"H5pio::registerParticles","invalid particle type");

  nParticles[type]= np;
  theParticleType= type;
}


void H5pio::registerBoolean1DField(const bool isNodeCentered, string name, bool *ptr)
{
  if (ptr != nullptr) {
    dataParticleType.push_back(theParticleType);
    dataIsNodeCentered.push_back(isNodeCentered);
    dataIsBoolean1D.push_back(true);
    dataIsInteger1D.push_back(false);
    dataIsFloat1D.push_back(false);
    dataIsFloat3D.push_back(false);
    dataIsGeometry3D.push_back(false);
    dataName.push_back(name);
    dataPointer.push_back(ptr);
  } // endif
}


void H5pio::registerInteger1DField(const bool isNodeCentered, string name, int *ptr)
{
  if (ptr != nullptr) {
    dataParticleType.push_back(theParticleType);
    dataIsNodeCentered.push_back(isNodeCentered);
    dataIsBoolean1D.push_back(false);
    dataIsInteger1D.push_back(true);
    dataIsFloat1D.push_back(false);
    dataIsFloat3D.push_back(false);
    dataIsGeometry3D.push_back(false);
    dataName.push_back(name);
    dataPointer.push_back(ptr);
  } // endif
}


void H5pio::registerFloat1DField(const bool isNodeCentered, string name, float *ptr)
{
  if (ptr != nullptr) {
    dataParticleType.push_back(theParticleType);
    dataIsNodeCentered.push_back(isNodeCentered);
    dataIsBoolean1D.push_back(false);
    dataIsInteger1D.push_back(false);
    dataIsFloat1D.push_back(true);
    dataIsFloat3D.push_back(false);
    dataIsGeometry3D.push_back(false);
    dataName.push_back(name);
    dataPointer.push_back(ptr);
  } // endif
}


void H5pio::registerFloat3DField(const bool isNodeCentered, string name, XcFloat3 *ptr)
{
  if (ptr != nullptr) {
    dataParticleType.push_back(theParticleType);
    dataIsNodeCentered.push_back(isNodeCentered);
    dataIsBoolean1D.push_back(false);
    dataIsInteger1D.push_back(false);
    dataIsFloat1D.push_back(false);
    dataIsFloat3D.push_back(true);
    dataIsGeometry3D.push_back(false);
    dataName.push_back(name);
    dataPointer.push_back(ptr);
  } // endif
}


void H5pio::registerGeometry3DField(const bool isNodeCentered, string name, XcFloat3 *ptr)
{
  if (ptr != nullptr) {
    dataParticleType.push_back(theParticleType);
    dataIsNodeCentered.push_back(isNodeCentered);
    dataIsBoolean1D.push_back(false);
    dataIsInteger1D.push_back(false);
    dataIsFloat1D.push_back(false);
    dataIsFloat3D.push_back(false);
    dataIsGeometry3D.push_back(true);
    dataName.push_back(name);
    dataPointer.push_back(ptr);
  } // endif
}


int H5pio::getNumberOfParticles(const int type)
{
  return nParticles[type];
}


// ***** consolidated file I/O *****
//
void H5pio::openFiles(XcCString fileName_in)
{
  multiTemporalFrameID= 0;
  XCuda::stringCopy(theBaseName,fileName_in,XCUDA_PATH_LENGTH);
  stripSuffix(theBaseName);
  stripID(theBaseName);
}


void H5pio::closeFiles(void)
{
  closeH5File();
  closeXdmfFile();
}


void H5pio::saveFrame(const float time)
{
  multiTemporalFrameID++;

  char fileName[XCUDA_PATH_LENGTH];
  sprintf(fileName,"%s_%04d.hdf5",theBaseName,multiTemporalFrameID);

  pushXdmfState();
  {
    openH5File(fileName,true);
    openXdmfFile();
    saveH5Frame(time);
    saveXdmfFrame(time);
    closeH5File();
    closeXdmfFile();
  }
  popXdmfState();

  xdmfFrameID= 0;
  saveXdmfFrame(time);
}

void H5pio::loadFrame(void)
{
  multiTemporalFrameID++;

  char fileName[XCUDA_PATH_LENGTH];
  sprintf(fileName,"%s_%04d.hdf5",theBaseName,multiTemporalFrameID);
  FILE *fp= fopen(fileName,"r");
  endOfFile= bool(fp == nullptr);

  if (fp) {
    fclose(fp);

    // loadH5Frame() will set these.
    // closeH5File() will reset them!
    //
    float theFrameTime;

    openH5File(fileName,false);
    {
      loadH5Frame();
      theFrameTime= frameTime;
    }
    closeH5File();

    frameTime= theFrameTime;
    endOfFile= false;
  } // endif
}


// ***** utilities for HDF5 I/O *****
//
void H5pio::openH5File(XcCString fileName, const bool createFile)
{
  frameTime= 0.0f;
  endOfFile= false;

  XCuda::stringCopy(hdf5Name,fileName,XCUDA_PATH_LENGTH);
  addSuffix(hdf5Name,".hdf5");

  if (createFile) {
    file_id= H5Fcreate(hdf5Name,H5F_ACC_TRUNC,H5P_DEFAULT,H5P_DEFAULT);
  } else {
    file_id= H5Fopen(hdf5Name,H5F_ACC_RDWR,H5P_DEFAULT); // 02/21/2020 H5F_ACC_RDONLY
  }

  XcHandleError(bool(file_id<0),XCUDA_ERROR,"H5pio::openH5File",
    "Unable to create or open an HDF5 file (check name and/or path)");

  fileIsOpen= true;
}

void H5pio::closeH5File(void)
{
  if (!fileIsOpen) return;

  H5Fclose(file_id);
  fileIsOpen= false;

  frameTime= 0.0f;
  endOfFile= true;
}

void H5pio::saveH5Frame(const float time)
{
  if (!fileIsOpen || endOfFile) return;

  frameTime= time;

  hid_t group_id= H5Gcreate(file_id,"Header",H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);
  {
    int flag_DoublePrecision= 0; // single for now
    float massTable[N_TYPES]; for (int i=0; i<N_TYPES; i++) massTable[i]= 0.0f; // in datasets
    int numFilesPerSnapshot= 1;
    int numPart_Total_HighWord[N_TYPES]; for (int i=0; i<N_TYPES; i++) numPart_Total_HighWord[i]= 0; // ?

    writeAttribute(group_id,H5T_NATIVE_INT,   "Flag_DoublePrecision", &flag_DoublePrecision);
    writeAttribute(group_id,H5T_NATIVE_FLOAT, "MassTable", &massTable, N_TYPES);
    writeAttribute(group_id,H5T_NATIVE_INT,   "NumFilesPerSnapshot", &numFilesPerSnapshot);
    writeAttribute(group_id,H5T_NATIVE_INT,   "NumPart_ThisFile", nParticles, N_TYPES);
    writeAttribute(group_id,H5T_NATIVE_INT,   "NumPart_Total", &nParticles, N_TYPES);
    writeAttribute(group_id,H5T_NATIVE_INT,   "NumPart_Total_HighWord", &numPart_Total_HighWord, N_TYPES);
    writeAttribute(group_id,H5T_NATIVE_FLOAT, "Time", &frameTime);
  }

  for (int type=0; type<N_TYPES; type++) {
    const int np= nParticles[type];
    if (np > 0) {

      char partType[16];
      sprintf(partType,"PartType%d",type);

      hid_t group_id= H5Gcreate(file_id,partType,H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);
      {
        for (int gid=0; gid<dataName.size(); gid++) {
          if (dataParticleType[gid] == type) {

            bool isNodeCentered= dataIsNodeCentered[gid]; // N/A
            bool isBoolean1D= dataIsBoolean1D[gid];
            bool isInteger1D= dataIsInteger1D[gid];
            bool isFloat1D= dataIsFloat1D[gid];
            bool isFloat3D= dataIsFloat3D[gid];
            bool isGeometry3D= dataIsGeometry3D[gid];
            char *name= (char*)dataName[gid].c_str();
            void *ptr= dataPointer[gid];

            if (isBoolean1D) {
              writeDataset(group_id,H5T_NATIVE_HBOOL,np,1,name,ptr);
            } else if (isInteger1D) {
              writeDataset(group_id,H5T_NATIVE_INT,np,1,name,ptr);
            } else if (isFloat1D) {
              writeDataset(group_id,H5T_NATIVE_FLOAT,np,1,name,ptr);
            } else if (isFloat3D) {
              writeDataset(group_id,H5T_NATIVE_FLOAT,np,3,name,ptr);
            } else if (isGeometry3D) {
              writeDataset(group_id,H5T_NATIVE_FLOAT,np,3,name,ptr);
            } // endif

          } // endif
        } // endfor(gid)
      }
      H5Gclose(group_id);

    } // endif
  } // endfor(type)
}


void H5pio::loadH5Frame(void)
{
  if (!fileIsOpen) return;

  hid_t group_id= H5Gopen(file_id,"Header",H5P_DEFAULT);
  {
    int np[N_TYPES];
    frameTime= 0.0f;
    
    readAttribute(group_id,H5T_NATIVE_INT,"NumPart_ThisFile",np);
    readAttribute(group_id,H5T_NATIVE_FLOAT,"Time",&frameTime);

    for (int i=0; i<N_TYPES; i++) {
      XcHandleError(bool(np[i] != nParticles[i]),XCUDA_ERROR,"H5pio::loadH5Frame",
        "Inconsistent number of particles; bad checkpoint file?");
    } // endfor
  }
  H5Gclose(group_id);

  for (int type=0; type<N_TYPES; type++) {
    const int np= nParticles[type];
    if (np > 0) {

      char partType[16];
      sprintf(partType,"PartType%d",type);

      hid_t group_id= H5Gopen(file_id,partType,H5P_DEFAULT);
      {

        for (int gid=0; gid<dataName.size(); gid++) {
          if (dataParticleType[gid] == type) {

            bool isNodeCentered= dataIsNodeCentered[gid]; // N/A
            bool isBoolean1D= dataIsBoolean1D[gid];
            bool isInteger1D= dataIsInteger1D[gid];
            bool isFloat1D= dataIsFloat1D[gid];
            bool isFloat3D= dataIsFloat3D[gid];
            bool isGeometry3D= dataIsGeometry3D[gid];
            char *name= (char*)dataName[gid].c_str();
            void *ptr= dataPointer[gid];

            if (isBoolean1D) {
              readDataset(group_id,H5T_NATIVE_HBOOL,name,ptr);
            } else if (isInteger1D) {
              readDataset(group_id,H5T_NATIVE_INT,name,ptr);
            } else if (isFloat1D) {
              readDataset(group_id,H5T_NATIVE_FLOAT,name,ptr);
            } else if (isFloat3D) {
              readDataset(group_id,H5T_NATIVE_FLOAT,name,ptr);
            } else if (isGeometry3D) {
              readDataset(group_id,H5T_NATIVE_FLOAT,name,ptr);
            }

          } // endif
        } // endfor(gid)
      }
      H5Gclose(group_id);


    } // endif
  } // endfor(type)
}

void H5pio::writeDataset(hid_t group_id, hid_t type, int nItems, int dof, XcCString name, void* data)
{
  if (data == nullptr) return;

  hsize_t dims[2]= {hsize_t(nItems),hsize_t(dof)};
  hid_t dataspace_id= H5Screate_simple(2,dims,nullptr);
  {
    hsize_t cdims[2]= {hsize_t(nItems),hsize_t(dof)};
    hid_t plist_id= H5Pcreate(H5P_DATASET_CREATE);
    H5Pset_chunk(plist_id,2,cdims);
    H5Pset_deflate(plist_id,6);
    {
      hid_t dataset_id= H5Dcreate(group_id,name,type,dataspace_id,
                                  H5P_DEFAULT,plist_id,H5P_DEFAULT);
      {
        H5Dwrite(dataset_id,type,H5S_ALL,H5S_ALL,H5P_DEFAULT,data);
      }
      H5Dclose(dataset_id);
    }
    H5Pclose(plist_id);
  }
  H5Sclose(dataspace_id);
}


void H5pio::readDataset(hid_t group_id, hid_t type, XcCString name, void* data)
{
  if (data == nullptr) return;

  hid_t dataset_id= H5Dopen(group_id,name,H5P_DEFAULT);
  {
    H5Dread(dataset_id,type,H5S_ALL,H5S_ALL,H5P_DEFAULT,data);
  }
  H5Dclose(dataset_id);
}

void H5pio::writeAttribute(hid_t group_id, hid_t type, XcCString name, void* data, int nDims)
{
  if (data == nullptr) return;
  bool exists= H5Aexists(group_id,name);

  hsize_t dims= nDims;
  hid_t dataspace_id= H5Screate_simple(1,&dims,nullptr);
  {
    hid_t attribute_id;
    if (exists) {
      attribute_id= H5Aopen(group_id,name,H5P_DEFAULT);
    } else {
      attribute_id= H5Acreate(group_id,name,type,dataspace_id,H5P_DEFAULT,H5P_DEFAULT);
    }
    H5Awrite(attribute_id,type,data);
    H5Aclose(attribute_id);
  }
  H5Sclose(dataspace_id);
}

void H5pio::readAttribute(hid_t group_id, hid_t type, XcCString name, void* data)
{
  if (data == nullptr) return;
  bool exists= H5Aexists(group_id,name);

  if (exists) {
    hid_t attribute_id= H5Aopen(group_id,name,H5P_DEFAULT);
    H5Aread(attribute_id,type,data);
    H5Aclose(attribute_id);
  } // endif
}


// ***** utilities for XDMF I/O *****
//
void H5pio::openXdmfFile(XcCString fileName)
{
  if (XCuda::stringLength(fileName) > 0) {
    XCuda::stringCopy(xdmfFileName,fileName,XCUDA_PATH_LENGTH);
  } else {
    XCuda::stringCopy(xdmfFileName,hdf5Name,XCUDA_PATH_LENGTH);
  }
  addSuffix(xdmfFileName,".xdmf");

  xdmfFile= fopen(xdmfFileName,"w");

  xdmfFileIsOpen= bool(xdmfFile!=nullptr);
  xdmfFrameID= 0;

  XcHandleError(!xdmfFileIsOpen,XCUDA_ERROR,"H5pio::openXdmfFile",
   "Unable to create or open an XDFM file (check name and/or path)");

  fprintf(xdmfFile,"<?xml version=\"1.0\" ?>\n");
  fprintf(xdmfFile,"<!DOCTYPE Xdmf SYSTEM \"Xdmf.dtd\" []>\n");
  fprintf(xdmfFile,"\n");
  fprintf(xdmfFile,"<Xdmf Version=\"2.0\" >\n");
  fprintf(xdmfFile,"  <Domain>\n");
  fprintf(xdmfFile,"    <Grid Name=\"Temporal Collection\" GridType=\"Collection\" CollectionType=\"Temporal\" >\n");
  writeXdmfTerminator= true;
}

void H5pio::closeXdmfFile(void)
{
  if (!xdmfFileIsOpen) return;

  if (writeXdmfTerminator) {
    fprintf(xdmfFile,"    </Grid>\n");
    fprintf(xdmfFile,"  </Domain>\n");
    fprintf(xdmfFile,"</Xdmf>\n");
  } // endif

  fclose(xdmfFile);
  xdmfFile= nullptr;

  xdmfFileIsOpen= false;
  xdmfFrameID= 0;
}


void H5pio::skipXdmfFrames(const int nFrames)
{
  char *line= NULL;
  size_t len= 0;

  // file header (6 lines)
  //
  for (int h=0; h<6; h++) {
    getline(&line,&len,xdmfFile);
  } // endfor(h)

  // skip frames
  for (int f=0; f<nFrames; f++) {

    // skip grid/topology header
    for (int gt=0; gt<5; gt++) {
      getline(&line,&len,xdmfFile);
    } // endfor(gt)

    // skip group
    for (int g=0; g<dataName.size(); g++) {

      // skip attribute
      for (int a=0; a<6; a++) {
        getline(&line,&len,xdmfFile);
      } // endfor(a)

    } // endfor(g)

    // skip grid/toppology trailer
    for (int gt=0; gt<3; gt++) {
      getline(&line,&len,xdmfFile);
    } // endfor(gt)

  } // endfor(f)

  free(line);
  //
} // end skipXdmfFrames()


void H5pio::saveXdmfFrame(const float time)
{
  if (!xdmfFileIsOpen) return;

  xdmfFrameID++;

  fprintf(xdmfFile,"\n");
  fprintf(xdmfFile,"      <Grid Name=\"GIZMO Particles\" GridType=\"Uniform\">\n");
  fprintf(xdmfFile,"        <Time Value=\"%.4e\"/>\n",time);
  fprintf(xdmfFile,"\n");

  const int nGroups= dataName.size();

  if (nGroups > 0) {
    for (int pg=0; pg<nGroups; pg++) {

      const int type= dataParticleType[pg]; // [0,5]
      const int np= nParticles[type];

      fprintf(xdmfFile,"        <Topology TopologyType=\"Polyvertex\" NumberOfElements=\"%d\" />\n",np);

      char partType[16];
      sprintf(partType,"PartType%d",type);

      bool isNodeCentered= dataIsNodeCentered[pg];
      bool isBoolean1D= dataIsBoolean1D[pg];
      bool isInteger1D= dataIsInteger1D[pg];
      bool isFloat1D= dataIsFloat1D[pg];
      bool isFloat3D= dataIsFloat3D[pg];
      bool isGeometry3D= dataIsGeometry3D[pg];
      char *name= (char*)dataName[pg].c_str();
      void *ptr= dataPointer[pg];

      if (isBoolean1D) {
        writeXdmfAttributeBoolean1D(np,partType,name,isNodeCentered);
      } else if (isInteger1D) {
        writeXdmfAttributeInteger1D(np,partType,name,isNodeCentered);
      } else if (isFloat1D) {
        writeXdmfAttributeFloat1D(np,partType,name,isNodeCentered);
      } else if (isFloat3D) {
        writeXdmfAttributeFloat3D(np,partType,name,isNodeCentered);
      } else if (isGeometry3D) {
        writeXdmfGeometry3D(np,partType,name);
      } // endif

    } // endfor(pg)
  } // endif(nGroups)

  fprintf(xdmfFile,"\n");
  fprintf(xdmfFile,"      </Grid>\n");
  fprintf(xdmfFile,"\n");
}

void H5pio::writeXdmfAttributeBoolean1D(int np, XcCString partType, XcCString name, const bool isNodeCentered)
{
  if (!xdmfFileIsOpen) return;

  const char *mode= isNodeCentered ? "Node" : "Cell";

  fprintf(xdmfFile,"\n");
  fprintf(xdmfFile,"        <Attribute Name=\"%s\" AttributeType=\"Scalar\" Center=\"%s\">\n",name,mode);
  fprintf(xdmfFile,"          <DataItem Dimensions=\"%d\" NumberType=\"Char\" Precision=\"1\" Format=\"HDF\" >\n",np);
  fprintf(xdmfFile,"            %s:/%s/%s\n",basename(hdf5Name),partType,name);
  fprintf(xdmfFile,"          </DataItem>\n");
  fprintf(xdmfFile,"        </Attribute>\n");
}


void H5pio::writeXdmfAttributeInteger1D(int np, XcCString partType, XcCString name, const bool isNodeCentered)
{
  if (!xdmfFileIsOpen) return;

  const char *mode= isNodeCentered ? "Node" : "Cell";

  fprintf(xdmfFile,"\n");
  fprintf(xdmfFile,"        <Attribute Name=\"%s\" AttributeType=\"Scalar\" Center=\"%s\">\n",name,mode);
  fprintf(xdmfFile,"          <DataItem Dimensions=\"%d\" NumberType=\"Integer\" Precision=\"4\" Format=\"HDF\" >\n",np);
  fprintf(xdmfFile,"            %s:/%s/%s\n",basename(hdf5Name),partType,name);
  fprintf(xdmfFile,"          </DataItem>\n");
  fprintf(xdmfFile,"        </Attribute>\n");
}


void H5pio::writeXdmfAttributeFloat1D(int np, XcCString partType, XcCString name, const bool isNodeCentered)
{
  if (!xdmfFileIsOpen) return;

  const char *mode= isNodeCentered ? "Node" : "Cell";

  fprintf(xdmfFile,"\n");
  fprintf(xdmfFile,"        <Attribute Name=\"%s\" AttributeType=\"Scalar\" Center=\"%s\">\n",name,mode);
  fprintf(xdmfFile,"          <DataItem Dimensions=\"%d\" NumberType=\"Float\" Precision=\"4\" Format=\"HDF\" >\n",np);
  fprintf(xdmfFile,"            %s:/%s/%s\n",basename(hdf5Name),partType,name);
  fprintf(xdmfFile,"          </DataItem>\n");
  fprintf(xdmfFile,"        </Attribute>\n");
}


void H5pio::writeXdmfAttributeFloat3D(int np, XcCString partType, XcCString name, const bool isNodeCentered)
{
  if (!xdmfFileIsOpen) return;

  const char *mode= isNodeCentered ? "Node" : "Cell";

  fprintf(xdmfFile,"\n");
  fprintf(xdmfFile,"        <Attribute Name=\"%s\" AttributeType=\"Vector\" Center=\"%s\">\n",name,mode);
  fprintf(xdmfFile,"          <DataItem Dimensions=\"%d 3\" NumberType=\"Float\" Precision=\"4\" Format=\"HDF\" >\n",np);
  fprintf(xdmfFile,"            %s:/%s/%s\n",basename(hdf5Name),partType,name);
  fprintf(xdmfFile,"          </DataItem>\n");
  fprintf(xdmfFile,"        </Attribute>\n");
}


void H5pio::writeXdmfGeometry3D(int np, XcCString partType, XcCString name)
{
  if (!xdmfFileIsOpen) return;

  fprintf(xdmfFile,"\n");
  fprintf(xdmfFile,"        <Geometry GeometryType=\"XYZ\">\n");
  fprintf(xdmfFile,"          <DataItem Dimensions=\"%d 3\" NumberType=\"Float\" Precision=\"4\" Format=\"HDF\" >\n",np);
  fprintf(xdmfFile,"            %s:/%s/%s\n",basename(hdf5Name),partType,name);
  fprintf(xdmfFile,"          </DataItem>\n");
  fprintf(xdmfFile,"        </Geometry>\n");
}


// ***** utilities for file I/O *****
//
bool H5pio::isDot(const char c)
{ return bool(c=='.'); }

bool H5pio::isDelimiter(const char c)
{ return bool(c=='/' || c==':'); }

bool H5pio::isDigit(const char c)
{ return bool('0'<=c && c<='9'); }


void H5pio::stripSuffix(XcString fileName)
{
  int idx= XCuda::stringLength(fileName) - 1;

  XcHandleError(idx<0,XCUDA_ERROR,"nullptr fileName","H5pio::stripSuffix");
  XcHandleError(isDot(fileName[idx]) || isDelimiter(fileName[idx]),
    XCUDA_ERROR,"Invalid fileName","H5pio::stripSuffix");

  while ( (idx>0) && !(isDot(fileName[idx]) || isDelimiter(fileName[idx])) ) idx--;
  if (isDot(fileName[idx])) fileName[idx]= '\0';
  //
} // end stripSuffix()

void H5pio::stripID(XcString fileName)
{
  int idx= XCuda::stringLength(fileName) - 1;

  XcHandleError(idx<0,XCUDA_ERROR,"nullptr fileName","H5pio::stripID");
  XcHandleError(isDot(fileName[idx]) || isDelimiter(fileName[idx]),
    XCUDA_ERROR,"Invalid fileName","H5pio::stripID");

  while ( (idx>0) && (fileName[idx]=='_' || isDigit(fileName[idx])) ) idx--;
  fileName[idx+1]= '\0';
  //
} // end stripID()


void H5pio::addSuffix(XcString fileName, XcCString suffix)
{
  stripSuffix(fileName);
  XCuda::stringCat(fileName,suffix,XCUDA_PATH_LENGTH);
  //
} // end addSuffix()
