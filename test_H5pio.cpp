//
// Authors: John G. Shaw
// Revised: Dec. 31 2022
// Version: 1.0.0
//
#include "H5pio.h"

void initParticles(H5pio &pm, const float time, const float dt)
{
  const float t= 1.0f + time;
  const unsigned np= pm.getNumberOfParticles(0);

  // do not change the order!
  float *energy=    (float*)pm.dataPointer[0];
  float   *mass=    (float*)pm.dataPointer[1];
  int      *pid=      (int*)pm.dataPointer[2];
  XcFloat3 *vel= (XcFloat3*)pm.dataPointer[3];
  XcFloat3 *loc= (XcFloat3*)pm.dataPointer[4];

  for (int i=0; i<np; i++) {
    const float p= (np<=1) ? 1.0f : 1.0f + float(i)/float(np-1) ;
    const float s= t*p;

    const float u= 1.0f*s;
    const float m= 2.0f*s;
    const XcFloat3 field= XcFloat3(1,2,3)/10.0f;

      energy[i]= u;
        mass[i]= m;
         pid[i]= i;
         loc[i]= field*1*s;
         vel[i]= field*2*s;
  } // endfor (i)
}

bool isClose(XcFloat3 x, XcFloat3 y)
{
  XcFloat3 dxy= x - y;
  return dxy.isSmall(2.0e-4);
}

bool isClose(float x, float y)
{
  return bool(fabsf(x-y) < 2.0e-4);
}

bool isClose(int x, int y)
{
  return bool(abs(x-y) == 0);
}

bool checkParticles(H5pio &po, H5pio &pi)
{
  const unsigned np= pi.getNumberOfParticles(0);

  // check for consistent number of particles
  bool ok= bool(np <= po.getNumberOfParticles(0));

  float    *po_energy= (float*)po.dataPointer[0];
  float    *po_mass=   (float*)po.dataPointer[1];
  int      *po_pid=      (int*)po.dataPointer[2];
  XcFloat3 *po_vel= (XcFloat3*)po.dataPointer[3];
  XcFloat3 *po_loc= (XcFloat3*)po.dataPointer[4];

  float    *pi_energy= (float*)pi.dataPointer[0];
  float    *pi_mass=   (float*)pi.dataPointer[1];
  int      *pi_pid=      (int*)pi.dataPointer[2];
  XcFloat3 *pi_vel= (XcFloat3*)pi.dataPointer[3];
  XcFloat3 *pi_loc= (XcFloat3*)pi.dataPointer[4];

  // check all particle values
  if (ok) {
    for (int i=0; i<np; i++) {
      ok= ok &&
          isClose(po_energy[i],pi_energy[i] ) &&
          isClose(po_mass[i],  pi_mass[i]   ) &&
          isClose(po_pid[i],   pi_pid[i]    ) &&
          isClose(po_vel[i],   pi_vel[i]    ) &&
          isClose(po_loc[i],   pi_loc[i]    );
    } // endfor(i)
  } // endif

  return ok;
}



int main(int argc, char *argv[])
{
  int jobStatus= 0;

  int np= 10;
  int nFrames= 1;
  XcString saveFile= XcString("./data/H5pio");

  XcParameters args;
  {
    args.parseCmdLineArguments(argc,argv,
    "  [--particles= 10] [--frames=1] [--saveFile= ./data/H5pio]");

    args.get_int("p*articles",&np, 1);
    args.get_int("frame*s",&nFrames, 1);
    args.get_string("save*File",&saveFile);

    args.checkCmdLineArguments();
  }

  const unsigned nParticles= np;

  // time runs from 0.0 to 1.0
  const float dt= (nFrames>1) ? 1.0f/float(nFrames-1) : 0.0f ;
  const bool isNodeCentered= true;


  H5pio::initH5Library();

  printf("\n");
  printf("Creating %d particles for output\n",nParticles);

  float *energy= new    float[nParticles];
  float   *mass= new    float[nParticles];
    int    *pid= new      int[nParticles];
  XcFloat3 *loc= new XcFloat3[nParticles];
  XcFloat3 *vel= new XcFloat3[nParticles];

  printf("\n");
  printf("Writing %d particles to: %s\n",nParticles,saveFile);
  printf("{\n");

    H5pio po;

    // do not change the order!
    po.registerParticles(nParticles,H5pio::Gas);
    po.registerFloat1DField(isNodeCentered,"InternalEnergy",energy);
    po.registerFloat1DField(isNodeCentered,"Masses",mass);
    po.registerInteger1DField(isNodeCentered,"ParticleIDs",pid);
    po.registerFloat3DField(isNodeCentered,"Velocities",vel);
    po.registerGeometry3DField(isNodeCentered,"Coordinates",loc);

    po.registerParticles(nParticles/2,H5pio::Buldge);
    po.registerFloat1DField(isNodeCentered,"Masses",mass);
    po.registerFloat3DField(isNodeCentered,"Velocities",vel);
    po.registerGeometry3DField(isNodeCentered,"Coordinates",loc);

    po.openFiles(saveFile);
    {
      float time= 0.0f;
      for (int frame=1; frame<=nFrames; frame++) {
        initParticles(po,time,dt);
        po.saveFrame(time);
        printf("   Saved frame %d at time %.3f\n",frame,time);
        time += dt;
      } // endfor
    }
    po.closeFiles();

  printf("}\n");


  printf("\n");
  printf("Creating %d particles for input\n",nParticles);

  float *energy_in= new    float[nParticles];
  float   *mass_in= new    float[nParticles];
  int      *pid_in= new      int[nParticles];
  XcFloat3 *loc_in= new XcFloat3[nParticles];
  XcFloat3 *vel_in= new XcFloat3[nParticles];

  printf("\n");
  printf("Reading particles from: %s\n",saveFile);
  printf("{\n");

    H5pio pi;

    // do not change the order!
    pi.registerParticles(nParticles,H5pio::Gas);
    pi.registerFloat1DField(isNodeCentered,"InternalEnergy",energy);
    pi.registerFloat1DField(isNodeCentered,"Masses",mass);
    pi.registerInteger1DField(isNodeCentered,"ParticleIDs",pid);
    pi.registerFloat3DField(isNodeCentered,"Velocities",vel);
    pi.registerGeometry3DField(isNodeCentered,"Coordinates",loc);

    pi.registerParticles(nParticles/2,H5pio::Buldge);
    pi.registerFloat1DField(isNodeCentered,"Masses",mass);
    pi.registerFloat3DField(isNodeCentered,"Velocities",vel);
    pi.registerGeometry3DField(isNodeCentered,"Coordinates",loc);

    pi.openFiles(saveFile);
    {
      do {
        pi.loadFrame();
        if (!pi.endOfFile) {
          initParticles(po,pi.frameTime,dt);
          bool status= checkParticles(po,pi);
          printf("  Loaded %d particles at time %.3f: %s\n",
            pi.getNumberOfParticles(0),pi.frameTime,status?"passed":"failed");
          if (!status) status= 1; // failed
        }
      } while (!pi.endOfFile);
    }
    pi.closeFiles();

  printf("}\n");
  
  delete[] energy_in;
  delete[] mass_in;
  delete[] pid_in;
  delete[] loc_in;
  delete[] vel_in;
  

  delete[] energy;
  delete[] mass;
  delete[] pid;
  delete[] loc;
  delete[] vel;

  H5pio::closeH5Library();

  return jobStatus;
}

