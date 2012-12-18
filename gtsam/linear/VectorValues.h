/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file    VectorValues.h
 * @brief   Factor Graph Values
 * @author  Richard Roberts
 */

#pragma once

#include <gtsam/base/Vector.h>
#include <gtsam/base/types.h>

#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <numeric>
#include <stdexcept>

namespace gtsam {

  // Forward declarations
  class Permutation;

  /**
   * This class represents a collection of vector-valued variables associated
   * each with a unique integer index.  It is typically used to store the variables
   * of a GaussianFactorGraph.  Optimizing a GaussianFactorGraph or GaussianBayesNet
   * returns this class.
   *
   * For basic usage, such as receiving a linear solution from gtsam solving functions,
   * or creating this class in unit tests and examples where speed is not important,
   * you can use a simple interface:
   *  - The default constructor VectorValues() to create this class
   *  - insert(Index, const Vector&) to add vector variables
   *  - operator[](Index) for read and write access to stored variables
   *  - \ref exists (Index) to check if a variable is present
   *  - Other facilities like iterators, size(), dim(), etc.
   *
   * Indices can be non-consecutive and inserted out-of-order, but you should not
   * use indices that are larger than a reasonable array size because the indices
   * correspond to positions in an internal array.
   *
   * Example:
   * \code
     VectorValues values;
     values.insert(3, Vector_(3, 1.0, 2.0, 3.0));
     values.insert(4, Vector_(2, 4.0, 5.0));
     values.insert(0, Vector_(4, 6.0, 7.0, 8.0, 9.0));

     // Prints [ 3.0 4.0 ]
     gtsam::print(values[1]);

     // Prints [ 8.0 9.0 ]
     values[1] = Vector_(2, 8.0, 9.0);
     gtsam::print(values[1]);
     \endcode
   *
   * <h2>Advanced Interface and Performance Information</h2>
   *
   * Internally, all vector values are stored as part of one large vector.  In
   * gtsam this vector is always pre-allocated for efficiency, using the
   * advanced interface described below.  Accessing and modifying already-allocated
   * values is \f$ O(1) \f$.  Using the insert() function of the standard interface
   * is slow because it requires re-allocating the internal vector.
   *
   * For advanced usage, or where speed is important:
   *  - Allocate space ahead of time using a pre-allocating constructor
   *    (\ref AdvancedConstructors "Advanced Constructors"), Zero(),
   *    SameStructure(), resize(), or append().  Do not use
   *    insert(Index, const Vector&), which always has to re-allocate the
   *    internal vector.
   *  - The vector() function permits access to the underlying Vector, for
   *    doing mathematical or other operations that require all values.
   *  - operator[]() returns a SubVector view of the underlying Vector,
   *    without copying any data.
   *
   * Access is through the variable index j, and returns a SubVector,
   * which is a view on the underlying data structure.
   *
   * This class is additionally used in gradient descent and dog leg to store the gradient.
   * \nosubgrouping
   */
  class VectorValues {
  protected:
    typedef std::vector<Vector> Values; ///< Typedef for the collection of Vectors making up a VectorValues
    Values values_; ///< Collection of Vectors making up this VectorValues

  public:
    typedef Values::iterator iterator; ///< Iterator over vector values
    typedef Values::const_iterator const_iterator; ///< Const iterator over vector values
    typedef Values::reverse_iterator reverse_iterator; ///< Reverse iterator over vector values
    typedef Values::const_reverse_iterator const_reverse_iterator; ///< Const reverse iterator over vector values
    typedef boost::shared_ptr<VectorValues> shared_ptr; ///< shared_ptr to this class

    /// @name Standard Constructors
    /// @{

    /**
     * Default constructor creates an empty VectorValues.
     */
    VectorValues() {}

    /** Named constructor to create a VectorValues of the same structure of the
     * specifed one, but filled with zeros.
     * @return
     */
    static VectorValues Zero(const VectorValues& model);

    /// @}
    /// @name Standard Interface
    /// @{

    /** Number of variables stored, always 1 more than the highest variable index,
     * even if some variables with lower indices are not present. */
    Index size() const { return values_.size(); }

    /** Return the dimension of variable \c j. */
    size_t dim(Index j) const { checkExists(j); return (*this)[j].rows(); }

    /** Return the summed dimensionality of all variables. */
    //size_t dim() const { return values_.rows(); }

    /** Return the dimension of each vector in this container */
    std::vector<size_t> dims() const;

    /** Check whether a variable with index \c j exists. */
    bool exists(Index j) const { return j < size() && (*this)[j].rows() > 0; }

    /** Read/write access to the vector value with index \c j, throws std::out_of_range if \c j does not exist, identical to operator[](Index). */
    Vector& at(Index j) { checkExists(j); return values_[j]; }

    /** Access the vector value with index \c j (const version), throws std::out_of_range if \c j does not exist, identical to operator[](Index). */
    const Vector& at(Index j) const { checkExists(j); return values_[j]; }

    /** Read/write access to the vector value with index \c j, throws std::out_of_range if \c j does not exist, identical to at(Index). */
    Vector& operator[](Index j) { return at(j); }

    /** Access the vector value with index \c j (const version), throws std::out_of_range if \c j does not exist, identical to at(Index). */
    const Vector& operator[](Index j) const { return at(j); }

    /** Insert a vector \c value with index \c j.
     * Causes reallocation. Can be used to insert values in any order, but
     * throws an invalid_argument exception if the index \c j is already used.
     * @param value The vector to be inserted.
     * @param j The index with which the value will be associated.
     */
    void insert(Index j, const Vector& value);

    iterator begin()                      { return values_.begin(); }  ///< Iterator over variables
    const_iterator begin() const          { return values_.begin(); }  ///< Iterator over variables
    iterator end()                         { return values_.end(); }    ///< Iterator over variables
    const_iterator end() const            { return values_.end(); }    ///< Iterator over variables
    reverse_iterator rbegin()              { return values_.rbegin(); } ///< Reverse iterator over variables
    const_reverse_iterator rbegin() const { return values_.rbegin(); } ///< Reverse iterator over variables
    reverse_iterator rend()                { return values_.rend(); }   ///< Reverse iterator over variables
    const_reverse_iterator rend() const   { return values_.rend(); }   ///< Reverse iterator over variables

    /** print required by Testable for unit testing */
    void print(const std::string& str = "VectorValues: ",
        const IndexFormatter& formatter =DefaultIndexFormatter) const;

    /** equals required by Testable for unit testing */
    bool equals(const VectorValues& x, double tol = 1e-9) const;

    /// @{
    /// \anchor AdvancedConstructors
    /// @name Advanced Constructors
    /// @}

    /** Construct from a container of variable dimensions (in variable order), without initializing any values. */
    template<class CONTAINER>
    explicit VectorValues(const CONTAINER& dimensions) { append(dimensions); }

    /** Construct to hold nVars vectors of varDim dimension each. */
    VectorValues(Index nVars, size_t varDim) { resize(nVars, varDim); }

    /** Named constructor to create a VectorValues that matches the structure of
     * the specified VectorValues, but do not initialize the new values. */
    static VectorValues SameStructure(const VectorValues& other);

    /** Named constructor to create a VectorValues from a container of variable
     * dimensions that is filled with zeros.
     * @param dimensions A container of the dimension of each variable to create.
     */
    template<class CONTAINER>
    static VectorValues Zero(const CONTAINER& dimensions);

    /** Named constructor to create a VectorValues filled with zeros that has
     * \c nVars variables, each of dimension \c varDim
     * @param nVars The number of variables to create
     * @param varDim The dimension of each variable
     * @return The new VectorValues
     */
    static VectorValues Zero(Index nVars, size_t varDim);

    /// @}
    /// @name Advanced Interface
    /// @{

    /** Resize this VectorValues to have identical structure to other, leaving
     * this VectorValues with uninitialized values.
     * @param other The VectorValues whose structure to copy
     */
    void resizeLike(const VectorValues& other);

    /** Resize the VectorValues to hold \c nVars variables, each of dimension
     * \c varDim, not preserving any data.  After calling this function, all
     * variables will be uninitialized.
     * @param nVars The number of variables to create
     * @param varDim The dimension of each variable
     */
    void resize(Index nVars, size_t varDim);

    /** Resize the VectorValues to contain variables of the dimensions stored
     * in \c dimensions, not preserving any data.  The new variables are
     * uninitialized, but this function is used to pre-allocate space for
     * performance.  After calling this function all variables will be uninitialized.
     * @param dimensions A container of the dimension of each variable to create.
     */
    template<class CONTAINER>
    void resize(const CONTAINER& dimensions);

    /** Append to the VectorValues to additionally contain variables of the
     * dimensions stored in \c dimensions.  The new variables are uninitialized,
     * but this function is used to pre-allocate space for performance.  This
     * function preserves the original data, so all previously-existing variables
     * are left unchanged.
     * @param dimensions A container of the dimension of each variable to create.
     */
    template<class CONTAINER>
    void append(const CONTAINER& dimensions);

    /** Set all entries to zero, does not modify the size. */
    void setZero();

    /** Reference the entire solution vector (const version). */
    //const Vector& asVector() const { return values_; }

    /** Reference the entire solution vector. */
    //Vector& asVector() { return values_; }

    /** Access a vector that is a subset of relevant indices */
    Vector vector(const std::vector<Index>& indices) const;

    /** Check whether this VectorValues has the same structure, meaning has the
     * same number of variables and that all variables are of the same dimension,
     * as another VectorValues
     * @param other The other VectorValues with which to compare structure
     * @return \c true if the structure is the same, \c false if not.
     */
    bool hasSameStructure(const VectorValues& other) const;

    /**
     * Permute the entries of this VectorValues in place
     */
    void permute(const Permutation& permutation);

    /**
     * Swap the data in this VectorValues with another.
     */
    void swap(VectorValues& other);

    /// @}
    /// @name Linear algebra operations
    /// @{

    /** Dot product with another VectorValues, interpreting both as vectors of
     * their concatenated values. */
    double dot(const VectorValues& V) const;

    /** Vector L2 norm */
    double norm() const;

    /**
     * + operator does element-wise addition.  Both VectorValues must have the
     * same structure (checked when NDEBUG is not defined).
     */
    VectorValues operator+(const VectorValues& c) const;

    /**
     * + operator does element-wise subtraction.  Both VectorValues must have the
     * same structure (checked when NDEBUG is not defined).
     */
    VectorValues operator-(const VectorValues& c) const;

    /**
     * += operator does element-wise addition.  Both VectorValues must have the
     * same structure (checked when NDEBUG is not defined).
     */
    void operator+=(const VectorValues& c);

    /// @}

  private:
    // Throw an exception if j does not exist
    void checkExists(Index j) const {
      if(!exists(j))
        throw std::out_of_range("VectorValues: requested variable index is not in this VectorValues.");
    }

    // Resize
    void copyStructureFrom(const VectorValues& other);

  public:

    /**
     * scale a vector by a scalar
     */
    friend VectorValues operator*(const double a, const VectorValues &V) {
      VectorValues result(VectorValues::SameStructure(V));
      result.values_ = a * V.values_;
      return result;
    }

    /// TODO: linear algebra interface seems to have been added for SPCG.
    friend size_t dim(const VectorValues& V) {
      return V.dim();
    }
    /// TODO: linear algebra interface seems to have been added for SPCG.
    friend double dot(const VectorValues& V1, const VectorValues& V2) {
      return gtsam::dot(V1.values_, V2.values_);
    }
    /// TODO: linear algebra interface seems to have been added for SPCG.
    friend void scal(double alpha, VectorValues& x) {
      gtsam::scal(alpha, x.values_);
    }
    /// TODO: linear algebra interface seems to have been added for SPCG.
    friend void axpy(double alpha, const VectorValues& x, VectorValues& y) {
      gtsam::axpy(alpha, x.values_, y.values_);
    }
    /// TODO: linear algebra interface seems to have been added for SPCG.
    friend void sqrt(VectorValues &x) {
      Vector y = gtsam::esqrt(x.values_);
      x.values_ = y;
    }

    /// TODO: linear algebra interface seems to have been added for SPCG.
    friend void ediv(const VectorValues& numerator,
        const VectorValues& denominator, VectorValues &result) {
      assert(
          numerator.dim() == denominator.dim() && denominator.dim() == result.dim());
      const size_t sz = result.dim();
      for (size_t i = 0; i < sz; ++i)
        result.values_[i] = numerator.values_[i] / denominator.values_[i];
    }

    /// TODO: linear algebra interface seems to have been added for SPCG.
    friend void edivInPlace(VectorValues& x, const VectorValues& y) {
      assert(x.dim() == y.dim());
      const size_t sz = x.dim();
      for (size_t i = 0; i < sz; ++i)
        x.values_[i] /= y.values_[i];
    }

  private:
    /** Serialization function */
    friend class boost::serialization::access;
    template<class ARCHIVE>
    void serialize(ARCHIVE & ar, const unsigned int version) {
      ar & BOOST_SERIALIZATION_NVP(values_);
    }
  }; // VectorValues definition

  // Implementations of template and inline functions

  /* ************************************************************************* */
  template<class CONTAINER>
  void VectorValues::resize(const CONTAINER& dimensions) {
    values_.clear();
    append(dimensions);
  }

  /* ************************************************************************* */
  template<class CONTAINER>
  void VectorValues::append(const CONTAINER& dimensions) {
    values_.resize(size() + dimensions.size());
    size_t i = size();
    BOOST_FOREACH(size_t dim, dimensions) {
      values_[i] = Vector(dim);
      ++ i;
    }
  }

  /* ************************************************************************* */
  template<class CONTAINER>
  VectorValues VectorValues::Zero(const CONTAINER& dimensions) {
    VectorValues ret;
    values_.resize(dimensions.size());
    size_t i = 0;
    BOOST_FOREACH(size_t dim, dimensions) {
      values_[i] = Vector::Zero(dim);
      ++ i;
    }
    return ret;
  }

  namespace internal {
  /* ************************************************************************* */
  // Helper function, extracts vectors with variable indices
  // in the first and last iterators, and concatenates them in that order into the
  // output.
  template<typename ITERATOR>
  Vector extractVectorValuesSlices(const VectorValues& values, ITERATOR first, ITERATOR last) {
    // Find total dimensionality
    int dim = 0;
    for(ITERATOR j = first; j != last; ++j)
      dim += values[*j].rows();

    // Copy vectors
    Vector ret(dim);
    int varStart = 0;
    for(ITERATOR j = first; j != last; ++j) {
      ret.segment(varStart, values[*j].rows()) = values[*j];
      varStart += values[*j].rows();
    }
    return ret;
  }

  /* ************************************************************************* */
  // Helper function, writes to the variables in values
  // with indices iterated over by first and last, interpreting vector as the
  // concatenated vectors to write.
  template<class VECTOR, typename ITERATOR>
  void writeVectorValuesSlices(const VECTOR& vector, VectorValues& values, ITERATOR first, ITERATOR last) {
    // Copy vectors
    int varStart = 0;
    for(ITERATOR j = first; j != last; ++j) {
      values[*j] = vector.segment(varStart, values[*j].rows());
      varStart += values[*j].rows();
    }
    assert(varStart == vector.rows());
  }
  }

} // \namespace gtsam
