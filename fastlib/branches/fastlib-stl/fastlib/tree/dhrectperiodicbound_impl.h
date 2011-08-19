/**
 * @file tree/dhrectperiodicbound_impl.h
 *
 * Implementation of hyper-rectangle bound policy class.
 * Template parameter t_pow is the metric to use; use 2 for Euclidian (L2).
 *
 * @experimental
 */

#ifndef TREE_DHRECTPERIODICBOUND_IMPL_H
#define TREE_DHRECTPERIODICBOUND_IMPL_H

#include <math.h>

#include "../math/math_lib.h"

using arma::vec;

/**
 * Empty constructor
 */
template<int t_pow>
DHrectPeriodicBound<t_pow>::DHrectPeriodicBound() {
  bounds_ = NULL;
  dim_ = 0;
}

/**
 * Initializes to specified dimensionality with each dimension the empty
 * set.
 */
template<int t_pow>
DHrectPeriodicBound<t_pow>::DHrectPeriodicBound(index_t dimension) {
  //DEBUG_ASSERT_MSG(dim_ == BIG_BAD_NUMBER, "Already initialized");
  bounds_ = new DRange[dimension];

  dim_ = dimension;
  Reset();
}

/**
 * Destructor: clean up memory
 */
template<int t_pow>
DHrectPeriodicBound<t_pow>::~DHrectPeriodicbound() {
  if(bounds_)
    delete[] bounds_;
}

/**
 * Modifies the box_size__ to the desired dimenstions.
 */
template<int t_pow>
void DHrectPeriodicBound<t_pow>::SetBoxSize(vec& box) {
  box_size__ = box; 
}

/**
 * Returns the box_size__ vector.
 */
template<int t_pow>
vec& DHrectPeriodicBound<t_pow>::GetBoxSize() {
  return box_size__
} 

/**
 * Makes this (uninitialized) box the average of the two arguments, 
 * i.e. the max and min of each range is the average of the maxes and mins 
 * of the arguments.  
 *
 * Added by: Bill March, 5/7
 */
template<int t_pow>
void DHrectPeriodicBound<t_pow>::AverageBoxesInit(const DHrectBound& box1,
                                                  const DHrectBound& box2) {

  dim_ = box1.dim();
  DEBUG_ASSERT(dim_ == box2.dim());

  if(bounds_)
    delete[] bounds_;
  bounds_ = new DRange[dim_];

  for (index_t i = 0; i < dim_; i++) {
    DRange range;
    range = box1.get(i) +  box2.get(i);
    range *= 0.5;
    bounds_[i] = range;
  }
}

/**
 * Resets all dimensions to the empty set.
 */
template<int t_pow>
void DHrectPeriodicBound<t_pow>::Reset() {
  for (index_t i = 0; i < dim_; i++) {
    bounds_[i].InitEmptySet();
  }
}

/**
 * Sets the dimensionality.
 */
template<int t_pow>
void DHrectPeriodicBound<t_pow>::SetSize(index_t dim) {
  if(bounds_)
    delete[] bounds_;

  bounds_ = new DRange[dim];
  dim_ = dim;
  Reset();
}

/**
 * Determines if a point is within this bound.
 */
template<int t_pow>
bool DHrectPeriodicBound<t_pow>::Contains(const vec& point) const {
  for (index_t i = 0; i < point.n_elem; i++) {
    if (!bounds_[i].Contains(point(i))) {
      return false;
    }
  }

  return true;
}

/**
 * Gets the range for a particular dimension.
 */
template<int t_pow>
const DRange DHrectPeriodicBound<t_pow>::operator[](index_t i) const {
  return bounds_[i];
}

/**
 * Calculates the maximum distance within the rectangle
 */
template<int t_pow>
double DHrectPeriodicBound<t_pow>::CalculateMaxDistanceSq() const {
  double max_distance = 0;
  for (index_t i = 0; i < dim_; i++)
    max_distance += pow(bounds_[i].width(), 2);

  return max_distance;  
}

/** Calculates the midpoint of the range */
template<int t_pow>
void DHrectPeriodicBound<t_pow>::CalculateMidpoint(vec& centroid) const {
  // set size correctly if necessary
  if(!(centroid.n_elem == dim_))
    centroid.set_size(dim_);

  for(index_t i = 0; i < dim_; i++) {
    centroid(i) = bounds_[i].mid();
  }
}

/**
 * Calculates minimum bound-to-point squared distance.
 */
template<int t_pow>
double DHrectPeriodicBound<t_pow>::MinDistanceSq(const vec& point) const {
  double sum = 0;

  for (index_t d = 0; d < dim_; d++){
    double a = point[d];
    double v = 0, bh;
    bh = bounds_[d].hi - bounds_[d].lo;
    bh = bh - floor(bh / box_size__[d]) * box_size__[d];
    a = a - bounds_[d].lo;
    a = a - floor(a / box_size__[d]) * box_size__[d];
    if (bh > a)
      v = min(a - bh, box_size__[d]-a);
    
    sum += pow(v, (double) t_pow);
  }

  return pow(sum, 2.0 / (double) t_pow);
}

/**
 * Calculates minimum bound-to-bound squared distance.
 *
 * Example: bound1.MinDistanceSq(other) for minimum squared distance.
 */
template<int t_pow>
double DHrectPeriodicBound<t_pow>::MinDistanceSq(const DHrectBound& other) const {
  double sum = 0;

  DEBUG_SAME_SIZE(dim_, other.dim_);

  for (index_t d = 0; d < dim_; d++){      
    double v = 0, d1, d2, d3;
    d1 = (bounds_[d].hi > bounds_[d].lo | other.bounds_[d].hi > other.bounds_[d].lo) *
      min(other.bounds_[d].lo - bounds_[d].hi, bounds_[d].lo - other.bounds_[d].hi);
    d2 = (bounds_[d].hi > bounds_[d].lo & other.bounds_[d].hi > other.bounds_[d].lo) *
      min(other.bounds_[d].lo - bounds_[d].hi, bounds_[d].lo - other.bounds_[d].hi + box_size__[d]);
    d3 = (bounds_[d].hi > bounds_[d].lo & other.bounds_[d].hi > other.bounds_[d].lo) *
      min(other.bounds_[d].lo - bounds_[d].hi + box_size__[d], bounds_[d].lo - other.bounds_[d].hi);
    v = (d1 + fabs(d1)) + (d2 + fabs(d2)) + (d3 + fabs(d3));
    sum += pow(v, (double) t_pow);
  }
  return pow(sum, 2.0 / (double) t_pow) / 4.0;
}

/**
 * Calculates maximum bound-to-point squared distance.
 */
template<int t_pow>
double DHrectPeriodicBound<t_pow>::MaxDistanceSq(const vec& point) const {
  double sum = 0;

  for (index_t d = 0; d < dim_; d++) {
    double b = point[d];
    double v = box_size__[d] / 2.0;
    double ah, al;
    ah = bounds_[d].hi - b;
    ah = ah - floor(ah / box_size__[d]) * box_size__[d];
    if (ah < v) {
      v = ah;
    } else {
      al = bounds_[d].lo - b;
      al = al - floor(al / box_size__[d]) * box_size__[d];
      if (al > v) {
        v = (2 * v) - al;
      }
    }
    sum += pow(fabs(v), (double) t_pow);
  }
  return pow(sum, 2.0 / (double) t_pow);
}

/**
 * Calculates maximum bound-to-point squared distance.
 */
/*
template<int t_pow>
double DHrectBound<t_pow>::MaxDistanceSq(const double *point) const {
  double sum = 0;

  for (index_t d = 0; d < dim_; d++) {
    double v = std::max(point[d] - bounds_[d].lo, bounds_[d].hi - point[d]);
    sum += std::pow(v, t_pow); // v is non-negative
  }

  return std::pow(sum, 2.0 / (double) t_pow);
}
*/

/**
 * Computes maximum distance.
 */
template<int t_pow>
double DHrectPeriodicBound<t_pow>::MaxDistanceSq(const DHrectBound& other) const {
  double sum = 0;

  DEBUG_SAME_SIZE(dim_, other.dim_);

  for (index_t d = 0; d < dim_; d++){
    double v = box_size__[d] / 2.0;
    double dh, dl;
    dh = bounds_[d].hi - other.bounds_[d].lo;
    dh = dh - floor(dh / box_size__[d]) * box_size__[d];
    dl = other.bounds_[d].hi - bounds_[d].lo;
    dl = dl - floor(dl / box_size__[d]) * box_size__[d];
    v = fabs(max(min(dh, v), min(dl, v)));

    sum += pow(v, (double) t_pow);
  }
  return pow(sum, 2.0 / (double) t_pow); 
}



template<int t_pow>
double DHrectPeriodicBound<t_pow>::MaxDelta(const DHrectBound& other, 
                                            double box_width, int dim) const {
  double result = 0.5 * box_width;
  double temp = other.bounds_[dim].hi - bounds_[dim].lo;
  temp = temp - floor(temp / box_width) * box_width;
  if (temp > box_width / 2) {
    temp = other.bounds_[dim].lo - bounds_[dim].hi;
    temp = temp - floor(temp / box_width) * box_width;
    if (temp > box_width / 2) {
      result = other.bounds_[dim].hi - bounds_[dim].lo;
      result = result - floor(temp / box_width + 1) * box_width;
    }
  } else {
    result = temp;
  }

  return result;
}

template<int t_pow>
double DHrectPeriodicBound<t_pow>::MinDelta(const DHrectBound& other,
                                            double box_width, int dim) const {
  double result = -0.5 * box_width;
  double temp = other.bounds_[dim].hi - bounds_[dim].lo;
  temp = temp - floor(temp / box_width) * box_width;
  if (temp > box_width / 2) {
    temp = other.bounds_[dim].hi - bounds_[dim].hi;
    temp -= floor(temp / box_width) * box_width;
    if (temp > box_width / 2)
      result = temp - box_width;

  } else {
    temp = other.bounds_[dim].hi - bounds_[dim].hi;
    result = temp - floor(temp / box_width) * box_width;
  }

  return result;
}

/**
 * Calculates minimum and maximum bound-to-bound squared distance.
 */
template<int t_pow>
DRange DHrectPeriodicBound<t_pow>::RangeDistanceSq(const DHrectBound& other) const {
  double sum_lo = 0;
  double sum_hi = 0;

  DEBUG_SAME_SIZE(dim_, other.dim_);

  double v1, v2, v_lo, v_hi;
  for (index_t d = 0; d < dim_; d++) {
    v1 = other.bounds_[d].lo - bounds_[d].hi;
    v2 = bounds_[d].lo - other.bounds_[d].hi;
    // one of v1 or v2 is negative
    if(v1 >= v2) {
      v_hi = -v2; // make it nonnegative
      v_lo = (v1 > 0) ? v1 : 0; // force to be 0 if negative
    } else {
      v_hi = -v1; // make it nonnegative
      v_lo = (v2 > 0) ? v2 : 0; // force to be 0 if negative
    }
    
    sum_lo += pow(v_lo, (double) t_pow);
    sum_hi += pow(v_hi, (double) t_pow);
  }

  return DRange(pow(sum_lo, 2.0 / (double) t_pow),
                pow(sum_hi, 2.0 / (double) t_pow));
}

/**
 * Calculates minimum and maximum bound-to-point squared distance.
 */
template<int t_pow>
DRange DHrectPeriodicBound<t_pow>::RangeDistanceSq(const vec& point) const {
  double sum_lo = 0;
  double sum_hi = 0;

  DEBUG_SAME_SIZE(point.n_elem, dim_);

  double v1, v2, v_lo, v_hi;
  for(index_t d = 0; d < dim_; d++) {
    v1 = bounds_[d].lo - point[d];
    v2 = point[d] - bounds_[d].hi;
    // one of v1 or v2 is negative
    if(v1 >= 0) {
      v_hi = -v2;
      v_lo = v1;
    } else {
      v_hi = -v1;
      v_lo = v2;
    }

    sum_lo += pow(v_lo, (double) t_pow);
    sum_hi += pow(v_hi, (double) t_pow);
  }

  return DRange(pow(sum_lo, 2.0 / (double) t_pow),
                pow(sum_hi, 2.0 / (double) t_pow));
}

/**
 * Calculates closest-to-their-midpoint bounding box distance,
 * i.e. calculates their midpoint and finds the minimum box-to-point
 * distance.
 *
 * Equivalent to:
 * <code>
 * other.CalcMidpoint(&other_midpoint)
 * return MinDistanceSqToPoint(other_midpoint)
 * </code>
 */
template<int t_pow>
double DHrectPeriodicBound<t_pow>::MinToMidSq(const DHrectBound& other) const {
  double sum = 0;

  DEBUG_SAME_SIZE(dim_, other.dim_);

  for (index_t d = 0; d < dim_; d++) {
    double v = other.bounds_[d].mid();
    double v1 = bounds_[d].lo - v;
    double v2 = v - bounds_[d].hi;

    v = (v1 + fabs(v1)) + (v2 + fabs(v2));

    sum += pow(v, (double) t_pow); // v is non-negative
  }

  return pow(sum, 2.0 / (double) t_pow) / 4.0;
}

/**
 * Computes minimax distance, where the other node is trying to avoid me.
 */
template<int t_pow>
double DHrectPeriodicBound<t_pow>::MinimaxDistanceSq(const DHrectBound& other) const {
  double sum = 0;

  DEBUG_SAME_SIZE(dim_, other.dim_);

  for(index_t d = 0; d < dim_; d++) {
    double v1 = other.bounds_[d].hi - bounds_[d].hi;
    double v2 = bounds_[d].lo - other.bounds_[d].lo;
    double v = std::max(v1, v2);
    v = (v + fabs(v)); /* truncate negatives to zero */
    sum += pow(v, (double) t_pow); // v is non-negative
  }

  return pow(sum, 2.0 / (double) t_pow) / 4.0;
}

/**
 * Calculates midpoint-to-midpoint bounding box distance.
 */
template<int t_pow>
double DHrectPeriodicBound<t_pow>::MidDistanceSq(const DHrectBound& other) const {
  double sum = 0;
  const DRange *a = this->bounds_;
  const DRange *b = other.bounds_;

  DEBUG_SAME_SIZE(dim_, other.dim_);

  for (index_t d = 0; d < dim_; d++) {
    // take the midpoint of each dimension (left multiplied by two for
    // calculation speed) and subtract from each other, then raise to t_pow
    sum += pow(fabs(bounds_[d].hi + bounds_[d].lo - other.bounds_[d].hi - other.bounds_[d].lo), (double) t_pow);
  }

  // take t_pow/2'th root and divide by constant of 4 (leftover from previous
  // step)
  return pow(sum, 2.0 / (double) t_pow) / 4.0;
}

/**
 * Expands this region to include a new point.
 */
template<int t_pow>
DHrectPeriodicBound<t_pow>& DHrectPeriodicBound<t_pow>::operator|=(const vec& vector) {
  DEBUG_SAME_SIZE(vector.n_elem, dim_);

  for (index_t i = 0; i < dim_; i++) {
    bounds_[i] |= vector[i];
  }

  return *this;
}

/**
 * Expands this region to encompass another bound.
 */
template<int t_pow>
DHrectPeriodicBound<t_pow>& DHrectPeriodicBound<t_pow>::operator|=(const DHrectBound& other) {
  DEBUG_SAME_SIZE(other.dim_, dim_);

  for (index_t i = 0; i < dim_; i++) {
    bounds_[i] |= other.bounds_[i];
  }

  return *this;
}


/**
 * Expand this bounding box to encompass another point. Done to 
 * minimize added volume in periodic coordinates.
 */
template<int t_pow>
DHrectPeriodicBound<t_pow>& DHrectPeriodicBound<t_pow>::Add(const vec& other, 
                                                            const vec& size) {
  DEBUG_SAME_SIZE(other.n_elem, dim_);
  // Catch case of uninitialized bounds
  if (bounds_[0].hi < 0){
    for (index_t i = 0; i < dim_; i++){
      bounds_[i] |= other[i];
    }
  }

  for (index_t i= 0; i < dim_; i++){
    double ah, al;
    ah = bounds_[i].hi - other[i];
    al = bounds_[i].lo - other[i];
    ah = ah - floor(ah / size[i]) * size[i];
    al = al - floor(al / size[i]) * size[i];
    if (ah < al) {
      if (size[i] - ah < al) {
        bounds_[i].hi = other[i];
      } else {
        bounds_[i].lo = other[i];
      }
    }
  }
  return *this;
}

/**
 * Expand this bounding box in periodic coordinates, minimizing added volume.
 */
template<int t_pow>
DHrectPeriodicBound<t_pow>& DHrectPeriodicBound<t_pow>::Add(const DHrectBound& other,
                                                            const vec& size){
  if (bounds_[0].hi < 0){
    for (index_t i = 0; i < dim_; i++){
      bounds_[i] |= other.bounds_[i];
    }
  }

  for (index_t i = 0; i < dim_; i++) {
    double ah, al, bh, bl;
    ah = bounds_[i].hi;
    al = bounds_[i].lo;
    bh = other.bounds_[i].hi;
    bl = other.bounds_[i].lo;
    ah = ah - al;    
    bh = bh - al;
    bl = bl - al;              
    ah = ah - floor(ah / size[i]) * size[i];
    bh = bh - floor(bh / size[i]) * size[i];
    bl = bl - floor(bl / size[i]) * size[i];

    if (((bh > ah) & (bh < bl | ah > bl )) ||
        (bh >= bl & bl > ah & bh < ah -bl + size[i])){
      bounds_[i].hi = other.bounds_[i].hi;
    }

    if (bl > ah && ((bl > bh) || (bh >= ah -bl + size[i]))){
      bounds_[i].lo = other.bounds_[i].lo;
    }   

    if (unlikely(ah > bl & bl > bh)){
      bounds_[i].lo = 0;
      bounds_[i].hi = size[i];
    }
  }

  return *this;
}

#endif
