//
// Authors: John G. Shaw
// Revised: Dec. 31 2022
// Version: 1.0.0
//
#include "H5pio.h"

int initParticles(H5pio &pm, int n1d)
{
  // Order counts!
  XcFloat3  *loc= (XcFloat3*)pm.dataPointer[0];
  float *density=    (float*)pm.dataPointer[1];
  float  *energy=    (float*)pm.dataPointer[2];
  float    *mass=    (float*)pm.dataPointer[3];
  int       *pid=      (int*)pm.dataPointer[4];
  float     *sph=    (float*)pm.dataPointer[5];
  XcFloat3  *vel= (XcFloat3*)pm.dataPointer[6];

  // NB. float gamma= 5.0/3.0;
  //
  const float L= 1.0; // diameter
  const float R= L/2;
  const int nbrs= 14;

  const float dx= L/float(n1d-1);
  const float dy= L/float(n1d-1);

  float rho= 1.0;
  float m= 1.0e-4;
  float h= nbrs*dx;
  float u= 0.0;  // NB. initial pressure is zero and U= pressure/((gamma-1)*density)
  
  int i= 0;

  for (float x=-R; x<=R; x+=dx) {
    for (float y=-R; y<=R; y+=dy) {

      float r= sqrt(x*x + y*y);

      if (r <= R) {

        float vx= r>0.0f ? -x/r : 0.0f;
        float vy= r>0.0f ? -y/r : 0.0f;

            loc[i]= XcFloat3(x,y,0.0f);
        density[i]= rho;
         energy[i]= u;
           mass[i]= m;
            pid[i]= i;
            sph[i]= h;
            vel[i]= XcFloat3(vx,vy,0.0f);

      i++;
    } // endif

    } // endfor (y)
  } // endfor (x)

  return i; // n2d
}



int main(int argc, char *argv[])
{
  int jobStatus= 0;

  int n1d= 128;
  XcString saveFile= XcString("./data/disk_2d");

  XcParameters args;
  {
    args.parseCmdLineArguments(argc,argv,
    "  [--n1d= 128]  [--saveFile= ./data/disk_2d]");

    args.get_int("n1d",&n1d, 3);
    args.get_string("save*File",&saveFile);

    args.checkCmdLineArguments();
  }

  const int n2d= n1d*n1d;
  const bool isNodeCentered= true;

  H5pio::initH5Library();

  printf("\n");
  printf("Creating %d particles\n",n2d);

  XcFloat3  *loc= new XcFloat3[n2d];
  float *density= new    float[n2d];
  float  *energy= new    float[n2d];
  float    *mass= new    float[n2d];
    int     *pid= new      int[n2d];
  float     *sph= new    float[n2d];
  XcFloat3  *vel= new XcFloat3[n2d];

  H5pio po;
  po.registerParticles(n2d,H5pio::Gas); // # reserved

  // NB. Order counts for initParticles()
  po.registerGeometry3DField(isNodeCentered,"Coordinates",loc);
  po.registerFloat1DField(isNodeCentered,"Density",density);
  po.registerFloat1DField(isNodeCentered,"InternalEnergy",energy);
  po.registerFloat1DField(isNodeCentered,"Masses",mass);
  po.registerInteger1DField(isNodeCentered,"ParticleIDs",pid);
  po.registerFloat1DField(isNodeCentered,"SmoothingLength",sph);
  po.registerFloat3DField(isNodeCentered,"Velocities",vel);

  int nParticles= initParticles(po,n1d);
  po.registerParticles(nParticles,H5pio::Gas); // actual #

  po.openFiles(saveFile);
  {
    po.saveFrame(0.0f);
    printf("Saved %d disk particles to %s\n",nParticles,saveFile);
  }
  po.closeFiles();

  delete[] loc;
  delete[] density;
  delete[] energy;
  delete[] mass;
  delete[] pid;
  delete[] sph;
  delete[] vel;

  H5pio::closeH5Library();

  return jobStatus;
}

