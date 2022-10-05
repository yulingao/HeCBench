#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <chrono>
#include <omp.h>

//define the data set size (cubic volume)
#define DATAXSIZE 100
#define DATAYSIZE 100
#define DATAZSIZE 100

#define SQ(x) ((x)*(x))

typedef double nRarray[DATAYSIZE][DATAXSIZE];

#ifdef VERIFY
#include <string.h>
#include "reference.h"
#endif

#pragma omp declare target
double dFphi(double phi, double u, double lambda)
{
  return (-phi*(1.0-phi*phi)+lambda*u*(1.0-phi*phi)*(1.0-phi*phi));
}


double GradientX(double phi[][DATAYSIZE][DATAXSIZE], 
                 double dx, double dy, double dz, int x, int y, int z)
{
  return (phi[x+1][y][z] - phi[x-1][y][z]) / (2.0*dx);
}


double GradientY(double phi[][DATAYSIZE][DATAXSIZE], 
                 double dx, double dy, double dz, int x, int y, int z)
{
  return (phi[x][y+1][z] - phi[x][y-1][z]) / (2.0*dy);
}


double GradientZ(double phi[][DATAYSIZE][DATAXSIZE], 
                 double dx, double dy, double dz, int x, int y, int z)
{
  return (phi[x][y][z+1] - phi[x][y][z-1]) / (2.0*dz);
}


double Divergence(double phix[][DATAYSIZE][DATAXSIZE], 
                  double phiy[][DATAYSIZE][DATAXSIZE],
                  double phiz[][DATAYSIZE][DATAXSIZE], 
                  double dx, double dy, double dz, int x, int y, int z)
{
  return GradientX(phix,dx,dy,dz,x,y,z) + 
         GradientY(phiy,dx,dy,dz,x,y,z) +
         GradientZ(phiz,dx,dy,dz,x,y,z);
}


double Laplacian(double phi[][DATAYSIZE][DATAXSIZE],
                 double dx, double dy, double dz, int x, int y, int z)
{
  double phixx = (phi[x+1][y][z] + phi[x-1][y][z] - 2.0 * phi[x][y][z]) / SQ(dx);
  double phiyy = (phi[x][y+1][z] + phi[x][y-1][z] - 2.0 * phi[x][y][z]) / SQ(dy);
  double phizz = (phi[x][y][z+1] + phi[x][y][z-1] - 2.0 * phi[x][y][z]) / SQ(dz);
  return phixx + phiyy + phizz;
}


double An(double phix, double phiy, double phiz, double epsilon)
{
  if (phix != 0.0 || phiy != 0.0 || phiz != 0.0){
    return ((1.0 - 3.0 * epsilon) * (1.0 + (((4.0 * epsilon) / (1.0-3.0*epsilon))*
           ((SQ(phix)*SQ(phix)+SQ(phiy)*SQ(phiy)+SQ(phiz)*SQ(phiz)) /
           ((SQ(phix)+SQ(phiy)+SQ(phiz))*(SQ(phix)+SQ(phiy)+SQ(phiz)))))));
  }
  else
  {
    return (1.0-((5.0/3.0)*epsilon));
  }
}


double Wn(double phix, double phiy, double phiz, double epsilon, double W0)
{
  return (W0*An(phix,phiy,phiz,epsilon));
}


double taun(double phix, double phiy, double phiz, double epsilon, double tau0)
{
  return tau0 * SQ(An(phix,phiy,phiz,epsilon));
}


double dFunc(double l, double m, double n)
{
  if (l != 0.0 || m != 0.0 || n != 0.0){
    return (((l*l*l*(SQ(m)+SQ(n)))-(l*(SQ(m)*SQ(m)+SQ(n)*SQ(n)))) /
            ((SQ(l)+SQ(m)+SQ(n))*(SQ(l)+SQ(m)+SQ(n))));
  }
  else
  {
    return 0.0;
  }
}
#pragma omp end declare target

void calculateForce(double phi[][DATAYSIZE][DATAXSIZE], 
                    double Fx[][DATAYSIZE][DATAXSIZE],
                    double Fy[][DATAYSIZE][DATAXSIZE],
                    double Fz[][DATAYSIZE][DATAXSIZE],
                    double dx, double dy, double dz,
                    double epsilon, double W0, double tau0)
{
  #pragma omp target teams distribute parallel for collapse(3) thread_limit(256)
  for (int ix = 0; ix < DATAXSIZE; ix++) {
    for (int iy = 0; iy < DATAYSIZE; iy++) {
      for (int iz = 0; iz < DATAZSIZE; iz++) {

        if ((ix < (DATAXSIZE-1)) && (iy < (DATAYSIZE-1)) && 
            (iz < (DATAZSIZE-1)) && (ix > (0)) && 
            (iy > (0)) && (iz > (0))) {

          double phix = GradientX(phi,dx,dy,dz,ix,iy,iz);
          double phiy = GradientY(phi,dx,dy,dz,ix,iy,iz);
          double phiz = GradientZ(phi,dx,dy,dz,ix,iy,iz);
          double sqGphi = SQ(phix) + SQ(phiy) + SQ(phiz);
          double c = 16.0 * W0 * epsilon;
          double w = Wn(phix,phiy,phiz,epsilon,W0);
          double w2 = SQ(w);
          

          Fx[ix][iy][iz] = w2 * phix + sqGphi * w * c * dFunc(phix,phiy,phiz);
          Fy[ix][iy][iz] = w2 * phiy + sqGphi * w * c * dFunc(phiy,phiz,phix);
          Fz[ix][iy][iz] = w2 * phiz + sqGphi * w * c * dFunc(phiz,phix,phiy);
        }
        else
        {
          Fx[ix][iy][iz] = 0.0;
          Fy[ix][iy][iz] = 0.0;
          Fz[ix][iy][iz] = 0.0;
        }
      }
    }
  }
}

// device function to set the 3D volume
void allenCahn(double phinew[][DATAYSIZE][DATAXSIZE], 
               double phiold[][DATAYSIZE][DATAXSIZE],
               double uold[][DATAYSIZE][DATAXSIZE],
               double Fx[][DATAYSIZE][DATAXSIZE],
               double Fy[][DATAYSIZE][DATAXSIZE],
               double Fz[][DATAYSIZE][DATAXSIZE],
               double epsilon, double W0, double tau0, double lambda,
               double dt, double dx, double dy, double dz)
{
  #pragma omp target teams distribute parallel for collapse(3) thread_limit(256)
  for (int ix = 1; ix < DATAXSIZE-1; ix++) {
    for (int iy = 1; iy < DATAYSIZE-1; iy++) {
      for (int iz = 1; iz < DATAZSIZE-1; iz++) {

        double phix = GradientX(phiold,dx,dy,dz,ix,iy,iz);
        double phiy = GradientY(phiold,dx,dy,dz,ix,iy,iz);
        double phiz = GradientZ(phiold,dx,dy,dz,ix,iy,iz); 

        phinew[ix][iy][iz] = phiold[ix][iy][iz] + 
         (dt / taun(phix,phiy,phiz,epsilon,tau0)) * 
         (Divergence(Fx,Fy,Fz,dx,dy,dz,ix,iy,iz) - 
          dFphi(phiold[ix][iy][iz], uold[ix][iy][iz],lambda));
      }
    }
  }
}

void boundaryConditionsPhi(double phinew[][DATAYSIZE][DATAXSIZE])
{
  #pragma omp target teams distribute parallel for collapse(3) thread_limit(256)
  for (int ix = 0; ix < DATAXSIZE; ix++) {
    for (int iy = 0; iy < DATAYSIZE; iy++) {
      for (int iz = 0; iz < DATAZSIZE; iz++) {

        if (ix == 0){
          phinew[ix][iy][iz] = -1.0;
        }
        else if (ix == DATAXSIZE-1){
          phinew[ix][iy][iz] = -1.0;
        }
        else if (iy == 0){
          phinew[ix][iy][iz] = -1.0;
        }
        else if (iy == DATAYSIZE-1){
          phinew[ix][iy][iz] = -1.0;
        }
        else if (iz == 0){
          phinew[ix][iy][iz] = -1.0;
        }
        else if (iz == DATAZSIZE-1){
          phinew[ix][iy][iz] = -1.0;
        }
      }
    }
  }
}

void thermalEquation(double unew[][DATAYSIZE][DATAXSIZE],
                     double uold[][DATAYSIZE][DATAXSIZE],
                     double phinew[][DATAYSIZE][DATAXSIZE],
                     double phiold[][DATAYSIZE][DATAXSIZE],
                     double D, double dt, double dx, double dy, double dz)
{
  #pragma omp target teams distribute parallel for collapse(3) thread_limit(256)
  for (int ix = 1; ix < DATAXSIZE-1; ix++) {
    for (int iy = 1; iy < DATAYSIZE-1; iy++) {
      for (int iz = 1; iz < DATAZSIZE-1; iz++) {

        unew[ix][iy][iz] = uold[ix][iy][iz] + 
          0.5*(phinew[ix][iy][iz]-
               phiold[ix][iy][iz]) +
          dt * D * Laplacian(uold,dx,dy,dz,ix,iy,iz);
      }
    }
  }
}

void boundaryConditionsU(double unew[][DATAYSIZE][DATAXSIZE], double delta)
{
  #pragma omp target teams distribute parallel for collapse(3) thread_limit(256)
  for (int ix = 0; ix < DATAXSIZE; ix++) {
    for (int iy = 0; iy < DATAYSIZE; iy++) {
      for (int iz = 0; iz < DATAZSIZE; iz++) {

        if (ix == 0){
          unew[ix][iy][iz] =  -delta;
        }
        else if (ix == DATAXSIZE-1){
          unew[ix][iy][iz] =  -delta;
        }
        else if (iy == 0){
          unew[ix][iy][iz] =  -delta;
        }
        else if (iy == DATAYSIZE-1){
          unew[ix][iy][iz] =  -delta;
        }
        else if (iz == 0){
          unew[ix][iy][iz] =  -delta;
        }
        else if (iz == DATAZSIZE-1){
          unew[ix][iy][iz] =  -delta;
        }
      }
    }
  }
}

void swapGrid(double cnew[][DATAYSIZE][DATAXSIZE],
              double cold[][DATAYSIZE][DATAXSIZE])
{
  #pragma omp target teams distribute parallel for collapse(3) thread_limit(256)
  for (int ix = 0; ix < DATAXSIZE; ix++) {
    for (int iy = 0; iy < DATAYSIZE; iy++) {
      for (int iz = 0; iz < DATAZSIZE; iz++) {
        double tmp = cnew[ix][iy][iz];
        cnew[ix][iy][iz] = cold[ix][iy][iz];
        cold[ix][iy][iz] = tmp;
      }
    }
  }
}

void initializationPhi(double phi[][DATAYSIZE][DATAXSIZE], double r0)
{
  #pragma omp parallel for collapse(3)
  for (int ix = 0; ix < DATAXSIZE; ix++) {
    for (int iy = 0; iy < DATAYSIZE; iy++) {
      for (int iz = 0; iz < DATAZSIZE; iz++) {
        double r = std::sqrt(SQ(ix-0.5*DATAXSIZE) + SQ(iy-0.5*DATAYSIZE) + SQ(iz-0.5*DATAZSIZE));
        if (r < r0){
          phi[ix][iy][iz] = 1.0;
        }
        else
        {
          phi[ix][iy][iz] = -1.0;
        }
      }
    }
  }
}

void initializationU(double u[][DATAYSIZE][DATAXSIZE], double r0, double delta)
{
  #pragma omp parallel for collapse(3)
  for (int ix = 0; ix < DATAXSIZE; ix++) {
    for (int iy = 0; iy < DATAYSIZE; iy++) {
      for (int iz = 0; iz < DATAZSIZE; iz++) {
        double r = std::sqrt(SQ(ix-0.5*DATAXSIZE) + SQ(iy-0.5*DATAYSIZE) + SQ(iz-0.5*DATAZSIZE));
        if (r < r0) {
          u[ix][iy][iz] = 0.0;
        }
        else
        {
          u[ix][iy][iz] = -delta * (1.0 - std::exp(-(r-r0)));
        }
      }
    }
  }
}

int main(int argc, char *argv[])
{
  const int num_steps = atoi(argv[1]);  //6000;
  const double dx = 0.4;
  const double dy = 0.4;
  const double dz = 0.4;
  const double dt = 0.01;
  const double delta = 0.8;
  const double r0 = 5.0;
  const double epsilon = 0.07;
  const double W0 = 1.0;
  const double beta0 = 0.0;
  const double D = 2.0;
  const double d0 = 0.5;
  const double a1 = 1.25 / std::sqrt(2.0);
  const double a2 = 0.64;
  const double lambda = (W0*a1)/(d0);
  const double tau0 = ((W0*W0*W0*a1*a2)/(d0*D)) + ((W0*W0*beta0)/(d0));

  // overall data set sizes
  const int nx = DATAXSIZE;
  const int ny = DATAYSIZE;
  const int nz = DATAZSIZE;
  const int vol = nx * ny * nz;

  // storage for result stored on host
  nRarray *phi_host = (nRarray *)malloc(vol*sizeof(double));
  nRarray *u_host = (nRarray *)malloc(vol*sizeof(double));
  initializationPhi(phi_host,r0);
  initializationU(u_host,r0,delta);

#ifdef VERIFY
  nRarray *phi_ref = (nRarray *)malloc(vol*sizeof(double));
  nRarray *u_ref = (nRarray *)malloc(vol*sizeof(double));
  memcpy(phi_ref, phi_host, vol*sizeof(double));
  memcpy(u_ref, u_host, vol*sizeof(double));
  reference(phi_ref, u_ref, vol, num_steps);
#endif 

  auto offload_start = std::chrono::steady_clock::now();

  // storage for result computed on device
  double *d_phiold = (double*)phi_host;
  double *d_uold = (double*)u_host;
  double *d_phinew = (double*) malloc (vol*sizeof(double));
  double *d_unew = (double*) malloc (vol*sizeof(double));
  double *d_Fx = (double*) malloc (vol*sizeof(double));
  double *d_Fy = (double*) malloc (vol*sizeof(double));
  double *d_Fz = (double*) malloc (vol*sizeof(double));

  #pragma omp target data map(tofrom: d_phiold[0:vol], \
                                      d_uold[0:vol]) \
                          map(alloc: d_phinew[0:vol], \
                                     d_unew[0:vol], \
                                     d_Fx[0:vol],\
                                     d_Fy[0:vol],\
                                     d_Fz[0:vol])
  {
    int t = 0;

    auto start = std::chrono::steady_clock::now();
  
    while (t <= num_steps) {
  
      calculateForce((nRarray*)d_phiold, (nRarray*)d_Fx,(nRarray*)d_Fy,(nRarray*)d_Fz,
                     dx,dy,dz,epsilon,W0,tau0);
  
      allenCahn((nRarray*)d_phinew,(nRarray*)d_phiold,(nRarray*)d_uold,
                (nRarray*)d_Fx,(nRarray*)d_Fy,(nRarray*)d_Fz,
                epsilon,W0,tau0,lambda, dt,dx,dy,dz);
  
      boundaryConditionsPhi((nRarray*)d_phinew);
  
      thermalEquation((nRarray*)d_unew,(nRarray*)d_uold,(nRarray*)d_phinew,(nRarray*)d_phiold,
                      D,dt,dx,dy,dz);
  
      boundaryConditionsU((nRarray*)d_unew,delta);
  
      swapGrid((nRarray*)d_phinew, (nRarray*)d_phiold);
  
      swapGrid((nRarray*)d_unew, (nRarray*)d_uold);
  
      t++;
    }
  
    auto end = std::chrono::steady_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    printf("Total kernel execution time: %.3f (ms)\n", time * 1e-6f);
  }

  auto offload_end = std::chrono::steady_clock::now();
  auto offload_time = std::chrono::duration_cast<std::chrono::nanoseconds>(offload_end - offload_start).count();
  printf("Offload time: %.3f (ms)\n", offload_time * 1e-6f);

#ifdef VERIFY
  bool ok = true;
  for (int idx = 0; idx < nx; idx++)
    for (int idy = 0; idy < ny; idy++)
      for (int idz = 0; idz < nz; idz++) {
        if (fabs(phi_ref[idx][idy][idz] - phi_host[idx][idy][idz]) > 1e-3) {
          ok = false; printf("phi: %lf %lf\n", phi_ref[idx][idy][idz], phi_host[idx][idy][idz]);
	}
        if (fabs(u_ref[idx][idy][idz] - u_host[idx][idy][idz]) > 1e-3) {
          ok = false; printf("u: %lf %lf\n", u_ref[idx][idy][idz], u_host[idx][idy][idz]);
        }
      }
  printf("%s\n", ok ? "PASS" : "FAIL");
  free(phi_ref);
  free(u_ref);
#endif

  free(phi_host);
  free(u_host);
  free(d_phinew);
  free(d_unew);
  free(d_Fx);
  free(d_Fy);
  free(d_Fz);
  return 0;
}
