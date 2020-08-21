/*
       * Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
       *
       * Permission is hereby granted, free of charge, to any person obtaining a
       * copy of this software and associated documentation files (the
       * "Software"), to deal in the Software without restriction, including
       * without limitation the rights to use, copy, modify, merge, publish,
       * distribute, sublicense, and/or sell copies of the Software, and to
       * permit persons to whom the Software is furnished to do so, subject to
       * the following conditions:
       *
       * The above copyright notice and this permission notice shall be included
       * in all copies or substantial portions of the Software.
       *
       * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
       * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
       * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
       * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
       * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
       * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
       * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
       */

#include <CL/sycl.hpp>
#include <dpct/dpct.hpp>
#include <chrono>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <string>

#ifdef CURAND
#include <mkl_rng_sycl.hpp>
#endif

#define TCRIT 2.26918531421f
#define THREADS  128

    // Initialize lattice spins
    void init_spins(signed char *lattice, const float *__restrict__ randvals,
                    const long long nx, const long long ny,
                    sycl::nd_item<3> item_ct1) {
  const long long tid =
      static_cast<long long>(item_ct1.get_local_range().get(2)) *
          item_ct1.get_group(2) +
      item_ct1.get_local_id(2);
  if (tid >= nx * ny) return;

  float randval = randvals[tid];
  signed char val = (randval < 0.5f) ? -1 : 1;
  lattice[tid] = val;
}

template<bool is_black>
void update_lattice(signed char* lattice,
                               const signed char* __restrict__ op_lattice,
                               const float* __restrict__ randvals,
                               const float inv_temp,
                               const long long nx,
                               const long long ny,
                               sycl::nd_item<3> item_ct1) {
  const long long tid =
      static_cast<long long>(item_ct1.get_local_range().get(2)) *
          item_ct1.get_group(2) +
      item_ct1.get_local_id(2);
  const int i = tid / ny;
  const int j = tid % ny;

  if (i >= nx || j >= ny) return;

  // Set stencil indices with periodicity
  int ipp = (i + 1 < nx) ? i + 1 : 0;
  int inn = (i - 1 >= 0) ? i - 1: nx - 1;
  int jpp = (j + 1 < ny) ? j + 1 : 0;
  int jnn = (j - 1 >= 0) ? j - 1: ny - 1;

  // Select off-column index based on color and row index parity
  int joff;
  if (is_black) {
    joff = (i % 2) ? jpp : jnn;
  } else {
    joff = (i % 2) ? jnn : jpp;
  }

  // Compute sum of nearest neighbor spins
  signed char nn_sum = op_lattice[inn * ny + j] + op_lattice[i * ny + j] + op_lattice[ipp * ny + j] + op_lattice[i * ny + joff];

  // Determine whether to flip spin
  signed char lij = lattice[i * ny + j];
  float acceptance_ratio = sycl::exp(-2.0f * inv_temp * nn_sum * lij);
  if (randvals[i*ny + j] < acceptance_ratio) {
    lattice[i * ny + j] = -lij;
  }
}

void update(signed char *lattice_b, signed char *lattice_w, float *randvals,
#ifdef CURAND
  mkl::rng::philox4x32x10 &rng,
#endif
            float inv_temp, long long nx, long long ny) {
  dpct::device_ext &dev_ct1 = dpct::get_current_device();
  sycl::queue &q_ct1 = dev_ct1.default_queue();

  // Setup CUDA launch configuration
  int blocks = (nx * ny/2 + THREADS - 1) / THREADS;

  // Update black
#ifdef CURAND
  /*
  DPCT1034:0: Migrated API does not return error code. 0 is returned in the
  lambda. You may need to rewrite this code.
  */
    mkl::rng::uniform<float> distr_ct1;
    mkl::rng::generate(distr_ct1, rng, nx * ny / 2, randvals);
#endif
  q_ct1.submit([&](sycl::handler &cgh) {
    cgh.parallel_for(sycl::nd_range<3>(sycl::range<3>(1, 1, blocks) *
                                           sycl::range<3>(1, 1, THREADS),
                                       sycl::range<3>(1, 1, THREADS)),
                     [=](sycl::nd_item<3> item_ct1) {
                       update_lattice<true>(lattice_b, lattice_w, randvals,
                                            inv_temp, nx, ny / 2, item_ct1);
                     });
  });

  // Update white
#ifdef CURAND
  /*
  DPCT1034:1: Migrated API does not return error code. 0 is returned in the
  lambda. You may need to rewrite this code.
  */
    mkl::rng::uniform<float> distr_ct2;
    mkl::rng::generate(distr_ct2, rng, nx * ny / 2, randvals);
#endif
  q_ct1.submit([&](sycl::handler &cgh) {
    cgh.parallel_for(sycl::nd_range<3>(sycl::range<3>(1, 1, blocks) *
                                           sycl::range<3>(1, 1, THREADS),
                                       sycl::range<3>(1, 1, THREADS)),
                     [=](sycl::nd_item<3> item_ct1) {
                       update_lattice<false>(lattice_w, lattice_b, randvals,
                                             inv_temp, nx, ny / 2, item_ct1);
                     });
  });
}

static void usage(const char *pname) {

  const char *bname = rindex(pname, '/');
  if (!bname) {bname = pname;}
  else        {bname++;}

  fprintf(stdout,
          "Usage: %s [options]\n"
          "options:\n"
          "\t-x|--lattice-n <LATTICE_N>\n"
          "\t\tnumber of lattice rows\n"
          "\n"
          "\t-y|--lattice_m <LATTICE_M>\n"
          "\t\tnumber of lattice columns\n"
          "\n"
          "\t-w|--nwarmup <NWARMUP>\n"
          "\t\tnumber of warmup iterations\n"
          "\n"
          "\t-n|--niters <NITERS>\n"
          "\t\tnumber of trial iterations\n"
          "\n"
          "\t-a|--alpha <ALPHA>\n"
          "\t\tcoefficient of critical temperature\n"
          "\n"
          "\t-s|--seed <SEED>\n"
          "\t\tseed for random number generation\n\n",
          bname);
  exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  dpct::device_ext &dev_ct1 = dpct::get_current_device();
  sycl::queue &q_ct1 = dev_ct1.default_queue();

  // Defaults
  long long nx = 5120;
  long long ny = 5120;
  float alpha = 0.1f;
  int nwarmup = 100;
  int niters = 1000;
  bool write = false;
  unsigned long long seed = 1234ULL;

  while (1) {
    static struct option long_options[] = {
        {     "lattice-n", required_argument, 0, 'x'},
        {     "lattice-m", required_argument, 0, 'y'},
        {         "alpha", required_argument, 0, 'y'},
        {          "seed", required_argument, 0, 's'},
        {       "nwarmup", required_argument, 0, 'w'},
        {        "niters", required_argument, 0, 'n'},
        { "write-lattice",       no_argument, 0, 'o'},
        {          "help",       no_argument, 0, 'h'},
        {               0,                 0, 0,   0}
    };

    int option_index = 0;
    int ch = getopt_long(argc, argv, "x:y:a:s:w:n:h", long_options, &option_index);
    if (ch == -1) break;

    switch(ch) {
      case 0:
        break;
      case 'x':
        nx = atoll(optarg); break;
      case 'y':
        ny = atoll(optarg); break;
      case 'a':
        alpha = atof(optarg); break;
      case 's':
        seed = atoll(optarg); break;
      case 'w':
        nwarmup = atoi(optarg); break;
      case 'n':
        niters = atoi(optarg); break;
      case 'h':
        usage(argv[0]); break;
      case '?':
        exit(EXIT_FAILURE);
      default:
        fprintf(stderr, "unknown option: %c\n", ch);
        exit(EXIT_FAILURE);
    }
  }

  // Check arguments
  if (nx % 2 != 0 || ny % 2 != 0) {
    fprintf(stderr, "ERROR: Lattice dimensions must be even values.\n");
    exit(EXIT_FAILURE);
  }

  float inv_temp = 1.0f / (alpha*TCRIT);


#ifdef CURAND
  // Setup cuRAND generator
  mkl::rng::philox4x32x10 rng(q_ct1, seed);
#else
  // for verification across difference platforms, generate random values once
  srand(seed);
  float* randvals_host = (float*) malloc(nx * ny/2 * sizeof(float));
  for (int i = 0; i < nx * ny/2; i++)
    randvals_host[i] = (float)rand() / (float)RAND_MAX;
#endif

  float *randvals;
  /*
  DPCT1003:4: Migrated API does not return error code. (*, 0) is inserted. You
  may need to rewrite this code.
  */
  randvals = (float *)sycl::malloc_device(
                  nx * ny / 2 * sizeof(*randvals), q_ct1);

  // Setup black and white lattice arrays on device
  signed char *lattice_b, *lattice_w;
  /*
  DPCT1003:6: Migrated API does not return error code. (*, 0) is inserted. You
  may need to rewrite this code.
  */
  lattice_b = (signed char *)sycl::malloc_device(
                  nx * ny / 2 * sizeof(*lattice_b), q_ct1);
  /*
  DPCT1003:7: Migrated API does not return error code. (*, 0) is inserted. You
  may need to rewrite this code.
  */
  lattice_w = (signed char *)sycl::malloc_device(
                  nx * ny / 2 * sizeof(*lattice_w), q_ct1);

  int blocks = (nx * ny/2 + THREADS - 1) / THREADS;

#ifndef CURAND
  q_ct1.memcpy(randvals, randvals_host, nx * ny / 2 * sizeof(float)).wait();
#endif

#ifdef CURAND
  /*
  DPCT1034:8: Migrated API does not return error code. 0 is returned in the
  lambda. You may need to rewrite this code.
  */
  mkl::rng::uniform<float> distr_ct3;
  mkl::rng::generate(distr_ct3, rng, nx * ny / 2, randvals);
#endif
  q_ct1.submit([&](sycl::handler &cgh) {
    cgh.parallel_for(sycl::nd_range<3>(sycl::range<3>(1, 1, blocks) *
                                           sycl::range<3>(1, 1, THREADS),
                                       sycl::range<3>(1, 1, THREADS)),
                     [=](sycl::nd_item<3> item_ct1) {
                       init_spins(lattice_b, randvals, nx, ny / 2, item_ct1);
                     });
  });

#ifdef CURAND
  /*
  DPCT1034:9: Migrated API does not return error code. 0 is returned in the
  lambda. You may need to rewrite this code.
  */
  mkl::rng::uniform<float> distr_ct4;
  mkl::rng::generate(distr_ct4, rng, nx * ny / 2, randvals);
#endif
  q_ct1.submit([&](sycl::handler &cgh) {
    cgh.parallel_for(sycl::nd_range<3>(sycl::range<3>(1, 1, blocks) *
                                           sycl::range<3>(1, 1, THREADS),
                                       sycl::range<3>(1, 1, THREADS)),
                     [=](sycl::nd_item<3> item_ct1) {
                       init_spins(lattice_w, randvals, nx, ny / 2, item_ct1);
                     });
  });

  // Warmup iterations
  printf("Starting warmup...\n");
  for (int i = 0; i < nwarmup; i++) {
    update(lattice_b, lattice_w, randvals, 
#ifdef CURAND
        rng, 
#endif
        inv_temp, nx, ny);
  }

  dev_ct1.queues_wait_and_throw();

  printf("Starting trial iterations...\n");
  auto t0 = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < niters; i++) {
    update(lattice_b, lattice_w, randvals, 
#ifdef CURAND
        rng, 
#endif
        inv_temp, nx, ny);
    if (i % 1000 == 0) printf("Completed %d/%d iterations...\n", i+1, niters);
  }

  dev_ct1.queues_wait_and_throw();
  auto t1 = std::chrono::high_resolution_clock::now();

  double duration = (double) std::chrono::duration_cast<std::chrono::microseconds>(t1-t0).count();
  printf("REPORT:\n");
  printf("\tnGPUs: %d\n", 1);
  printf("\ttemperature: %f * %f\n", alpha, TCRIT);
  printf("\tseed: %llu\n", seed);
  printf("\twarmup iterations: %d\n", nwarmup);
  printf("\ttrial iterations: %d\n", niters);
  printf("\tlattice dimensions: %lld x %lld\n", nx, ny);
  printf("\telapsed time: %f sec\n", duration * 1e-6);
  printf("\tupdates per ns: %f\n", (double) (nx * ny) * niters / duration * 1e-3);

  signed char* lattice_b_h = (signed char*) malloc(nx * ny/2 * sizeof(*lattice_b_h));
  signed char* lattice_w_h = (signed char*) malloc(nx * ny/2 * sizeof(*lattice_w_h));
  q_ct1.memcpy(lattice_b_h, lattice_b, nx * ny / 2 * sizeof(*lattice_b)).wait();
  q_ct1.memcpy(lattice_w_h, lattice_w, nx * ny / 2 * sizeof(*lattice_w)).wait();
  double naivesum = 0.0;
  for (int i = 0; i < nx*ny/2; i++) {
    naivesum += lattice_b_h[i];
    naivesum += lattice_w_h[i];
  }
  printf("checksum = %lf\n", naivesum);
#ifndef CURAND
  free(randvals_host);
#endif
  free(lattice_b_h);
  free(lattice_w_h);

  sycl::free(lattice_b, q_ct1);
  sycl::free(lattice_w, q_ct1);
  sycl::free(randvals, q_ct1);

  return 0;
}
