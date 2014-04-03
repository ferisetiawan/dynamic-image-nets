//
//  normalize.cpp
//  matconv
//
//  Created by Andrea Vedaldi on 28/03/2014.
//  Copyright (c) 2014 Andrea Vedaldi. All rights reserved.
//

#include "normalize.hpp"
#include "caffe-scraps.hpp"

/* ---------------------------------------------------------------- */
/*                                                  normalize (CPU) */
/* ---------------------------------------------------------------- */

#define xat(t) x[(t) * offset]
#define yat(t) y[(t) * offset]
#define zat(t) z[(t) * offset]

template<typename T>
void normalize_cpu(T* normalized,
                   T const* data,
                   size_t width,
                   size_t height,
                   size_t depth,
                   size_t normDepth,
                   T kappa, T alpha, T beta)
{
  int m = ((signed)normDepth-1)/2 ;
  int offset = width*height ;
  for (int h = 0 ; h < height ; ++h) {
    for (int w = 0 ; w < width ; ++w) {
      int t ;
      T const* x = data + w + h * width ;
      T* y = normalized + w + h * width ;
      T acc = 0 ;
      for (t = -m ; t < (signed)depth + m ; ++t) {
        T ap = 0 ;
        T am = 0 ;
        if (t-m-1 >= 0) { am = xat(t-m-1) ; }
        if (t+m < depth) { ap = xat(t+m) ; }
        acc += ap*ap - am*am ;
        if (0 <= t && t < depth) {
          yat(t) = xat(t) * pow(kappa + alpha * acc, -beta) ;
        }
      }
    }
  }
}

template
void normalize_cpu<float>(float* normalized,
                          float const* data,
                          size_t width,
                          size_t height,
                          size_t depth,
                          size_t normDetph,
                          float kappa, float alpha, float beta) ;

template
void normalize_cpu<double>(double* normalized,
                           double const* data,
                           size_t width,
                           size_t height,
                           size_t depth,
                           size_t normDetph,
                           double kappa, double alpha, double beta) ;


/* ---------------------------------------------------------------- */
/*                                                  normalize (CPU) */
/* ---------------------------------------------------------------- */

template<typename T>
void normalizeBackward_cpu(T* normalized,
                           T const* data,
                           T const* dzdy,
                           size_t width,
                           size_t height,
                           size_t depth,
                           size_t normDepth,
                           T kappa, T alpha, T beta)
{
  int m = ((signed)normDepth-1)/2 ;
  int offset = width*height ;
  T ab2 = 2*alpha*beta ;
  for (int h = 0 ; h < height ; ++h) {
    for (int w = 0 ; w < width ; ++w) {
      int t, q ;
      T const* x = data + w + h * width ;
      T* y = normalized + w + h * width ;
      T const* z = dzdy + w + h * width ;
      T acc = 0 ;
      for (t = 0 ; t < (signed)depth ; ++t) {
        yat(t) = 0 ;
      }
      for (t = -m ; t < (signed)depth + m ; ++t) {
        int q1 = t-m ;
        int q2 = t+m ;
        T ap = 0 ;
        T am = 0 ;
        if (t-m-1 >= 0) { am = xat(t-m-1) ; } else { q1 = 0 ; }
        if (t+m < depth) { ap = xat(t+m) ; } else { q2 = depth - 1 ; }
        acc += ap*ap - am*am ;
        T L = kappa + alpha * acc ;
        T Lbeta = pow(L, -beta) ;
        T Lbeta1 = Lbeta / L ;

        if (0 <= t && t < depth) {
          yat(t) += zat(t) * Lbeta ;
          for (q = q1 ; q <= q2 ; ++ q) {
            yat(q) -= zat(t) * xat(t) * xat(q) * ab2 * Lbeta1 ;
          }
        }
      }
    }
  }
}

template
void normalizeBackward_cpu<float>(float* normalized,
                                  float const* data,
                                  float const* dzdy,
                                  size_t width,
                                  size_t height,
                                  size_t depth,
                                  size_t normDetph,
                                  float kappa, float alpha, float beta) ;

template
void normalizeBackward_cpu<double>(double* normalized,
                                   double const* data,
                                   double const* dzdy,
                                   size_t width,
                                   size_t height,
                                   size_t depth,
                                   size_t normDetph,
                                   double kappa, double alpha, double beta) ;

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/*                                                  normalize (GPU) */
/* ---------------------------------------------------------------- */

#undef xat
#undef yat
#undef zat
#define xat(t) x[(t) * offset]
#define yat(t) y[(t) * offset]
#define zat(t) z[(t) * offset]

template<typename T>
__global__
void normalize_gpu_kernel (int count,
                           T* normalized,
                           T const* data,
                           int width,
                           int height,
                           int depth,
                           int normDepth,
                           T kappa, T alpha, T beta)
{
  int index = threadIdx.x + blockIdx.x * blockDim.x;
  if (index < count) {
    int m = ((signed)normDepth-1)/2 ;
    int offset = width*height ;
    int h = index / width ;
    int w = index % width ;
    int t ;
    T const* x = data + w + h * width ;
    T* y = normalized + w + h * width ;
    T acc = 0 ;
    for (t = -m ; t < (signed)depth + m ; ++t) {
      T ap = 0 ;
      T am = 0 ;
      if (t-m-1 >= 0) { am = xat(t-m-1) ; }
      if (t+m < depth) { ap = xat(t+m) ; }
      acc += ap*ap - am*am ;
      if (0 <= t && t < depth) {
        yat(t) = xat(t) * pow(kappa + alpha * acc, -beta) ;
      }
    }
  }
}

template<typename T>
void normalize_gpu(T* normalized,
                   T const* data,
                   size_t width,
                   size_t height,
                   size_t depth,
                   size_t normDepth,
                   T kappa, T alpha, T beta)
{
  int count = width*height ;
  normalize_gpu_kernel<T>
  <<<CAFFE_GET_BLOCKS(count), CAFFE_CUDA_NUM_THREADS>>>
  (count, normalized, data, width, height, depth, normDepth, kappa, alpha, beta) ;
}

template
void normalize_gpu<float>(float* normalized,
                          float const* data,
                          size_t width,
                          size_t height,
                          size_t depth,
                          size_t normDetph,
                          float kappa, float alpha, float beta) ;

template
void normalize_gpu<double>(double* normalized,
                           double const* data,
                           size_t width,
                           size_t height,
                           size_t depth,
                           size_t normDetph,
                           double kappa, double alpha, double beta) ;


/* ---------------------------------------------------------------- */
/*                                                  normalize (gpu) */
/* ---------------------------------------------------------------- */

template<typename T>
__global__
void normalizeBackward_gpu_kernel(int count,
                                  T* normalized,
                                  T const* data,
                                  T const* dzdy,
                                  int width,
                                  int height,
                                  int depth,
                                  int normDepth,
                                  T kappa, T alpha, T beta)
{
  int index = threadIdx.x + blockIdx.x * blockDim.x;
  if (index < count) {
    int m = ((signed)normDepth-1)/2 ;
    int offset = width*height ;
    T ab2 = 2*alpha*beta ;
    int h = index / width ;
    int w = index % width ;
    int t, q ;
    T const* x = data + w + h * width ;
    T* y = normalized + w + h * width ;
    T const* z = dzdy + w + h * width ;
    T acc = 0 ;
    for (t = 0 ; t < (signed)depth ; ++t) {
      yat(t) = 0 ;
    }
    for (t = -m ; t < (signed)depth + m ; ++t) {
      int q1 = t-m ;
      int q2 = t+m ;
      T ap = 0 ;
      T am = 0 ;
      if (t-m-1 >= 0) { am = xat(t-m-1) ; } else { q1 = 0 ; }
      if (t+m < depth) { ap = xat(t+m) ; } else { q2 = depth - 1 ; }
      acc += ap*ap - am*am ;
      T L = kappa + alpha * acc ;
      T Lbeta = pow(L, -beta) ;
      T Lbeta1 = Lbeta / L ;

      if (0 <= t && t < depth) {
        yat(t) += zat(t) * Lbeta ;
        for (q = q1 ; q <= q2 ; ++ q) {
          yat(q) -= zat(t) * xat(t) * xat(q) * ab2 * Lbeta1 ;
        }
      }
    }
  }
}

template<typename T>
void normalizeBackward_gpu(T* normalized,
                           T const* data,
                           T const* dzdy,
                           size_t width,
                           size_t height,
                           size_t depth,
                           size_t normDepth,
                           T kappa, T alpha, T beta)
{
  int count = width*height ;
  normalizeBackward_gpu_kernel<T>
  <<<CAFFE_GET_BLOCKS(count), CAFFE_CUDA_NUM_THREADS>>>
  (count, normalized, data, dzdy, width, height, depth, normDepth, kappa, alpha, beta) ;
}

template
void normalizeBackward_gpu<float>(float* normalized,
                                  float const* data,
                                  float const* dzdy,
                                  size_t width,
                                  size_t height,
                                  size_t depth,
                                  size_t normDetph,
                                  float kappa, float alpha, float beta) ;

template
void normalizeBackward_gpu<double>(double* normalized,
                                   double const* data,
                                   double const* dzdy,
                                   size_t width,
                                   size_t height,
                                   size_t depth,
                                   size_t normDetph,
                                   double kappa, double alpha, double beta) ;


