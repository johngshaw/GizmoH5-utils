//
// Authors: John G. Shaw
// Revised: Dec. 28 2022
// Version: 1.0.0
//
// % h5dump -A ./data/noh_ics.hdf5
//   ... prints info, including #-of-particles ...
// % convertGizmoH5 --p=2097152 --f=noh_ics.hdf5 > ./data/noh_ics.xdmf
//
#include "XCut.h"
#include <string>

using namespace std;

void geometry(string file, string name, int nParticles, int precision=4)
{
  printf("\n");
  printf("        <Geometry GeometryType=\"XYZ\">\n");
  printf("          <DataItem Dimensions=\"%d 3\" NumberType=\"Float\" Precision=\"%d\" Format=\"HDF\" >\n",nParticles,precision);
  printf("            %s:/PartType0/%s\n",file.data(),name.data());
  printf("          </DataItem>\n");
  printf("        </Geometry>\n");
}

void attribute(string file, string name, int nParticles, int precision=4, int dof=1)
{
  printf("\n");
  printf("        <Attribute Name=\"%s\" AttributeType=\"%s\" Center=\"Node\">\n",name.data(),(dof==1)?"Scalar":"Vector");
  printf("          <DataItem Dimensions=\"%d %d\" NumberType=\"Float\" Precision=\"%d\" Format=\"HDF\" >\n",nParticles,dof,precision);
  printf("            %s:/PartType0/%s\n",file.data(),name.data());
  printf("          </DataItem>\n");
  printf("        </Attribute>\n");
}


int main(int argc, char *argv[])
{
  int nParticles= 0;
  bool isDouble= false;
  float time= 0.0;
  XcString xcfile= XcString("snapshot_000.html");
  
  XcParameters args;
  {
    args.parseCmdLineArguments(argc,argv,
    "  --particles=? [--double] [--time=0.0] [--file=snapshot_000.html]\n");

    args.get_int("p*articles",&nParticles, 1);
    args.getCmdLineFlag("d*ouble",&isDouble);
    args.get_float("t*ime",&time, 0.0f);
    args.get_string("f*ile",&xcfile);

    args.checkCmdLineArguments();
    args.checkForMissingParameter("p*articles");
  }
  
  int precision= isDouble?8:4;
  string file(xcfile);
  
  printf("<?xml version=\"1.0\" ?>\n");
  printf("<!DOCTYPE Xdmf SYSTEM \"Xdmf.dtd\" []>\n");
  printf("<Xdmf Version=\"2.0\" >\n");
  printf("  <Domain>\n");
  printf("    <Grid Name=\"Temporal Collection\" GridType=\"Collection\" CollectionType=\"Temporal\">\n");
  printf("\n");
  printf("      <Grid Name=\"GIZMO Particles\" GridType=\"Uniform\">\n");
  printf("        <Time Value=\"%.4e\"/>\n",time);
  printf("\n");
  printf("        <Topology TopologyType=\"Polyvertex\" NumberOfElements=\"%d\" />\n",nParticles);

   geometry(file,"Coordinates",nParticles,precision);
  attribute(file,"Density",nParticles,precision);
  attribute(file,"InternalEnergy",nParticles,precision);
  attribute(file,"Masses",nParticles,precision);
  attribute(file,"ParticleIDs",nParticles,precision);
  attribute(file,"SmoothingLength",nParticles,precision);
  attribute(file,"Velocities",nParticles,precision,3);
  
  printf("\n");
  printf("      </Grid>\n");
  
  printf("\n");
  printf("    </Grid>\n");
  printf("  </Domain>\n");
  printf("</Xdmf>\n");
  
  return 0;
}
