/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2020 Scientific Computing and Imaging Institute,
   University of Utah.

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/


///
///@file  Tensor.cc
///@brief Symmetric, positive definite tensors (diffusion, conductivity)
///
///@author
///       David Weinstein
///       Department of Computer Science
///       University of Utah
///@date  March 2001
///


#include "Core/Datatypes/MatrixFwd.h"
#include <Core/GeometryPrimitives/Tensor.h>
#include <Core/Utils/Legacy/TypeDescription.h>
#include <Core/Utils/Legacy/Assert.h>
#include <Core/Datatypes/DenseMatrix.h>

#include <iostream>

#include <Core/Persistent/Persistent.h>

#include <teem/ten.h>

using namespace SCIRun;
using namespace Core::Geometry;
using namespace Core::Datatypes;

Tensor::Tensor() : l1_(0), l2_(0), l3_(0), have_eigens_(false)
{
  mat_[0][0] = 0.0;
  mat_[0][1] = 0.0;
  mat_[0][2] = 0.0;
  mat_[1][0] = 0.0;
  mat_[1][1] = 0.0;
  mat_[1][2] = 0.0;
  mat_[2][0] = 0.0;
  mat_[2][1] = 0.0;
  mat_[2][2] = 0.0;
}

Tensor::Tensor(const Tensor& copy)
{
  for(int i=0; i<3; i++)
    for(int j=0; j<3; j++)
      mat_[i][j]=copy.mat_[i][j];
  have_eigens_=copy.have_eigens_;
  if (have_eigens_) {
    e1_=copy.e1_; e2_=copy.e2_; e3_=copy.e3_;
    l1_=copy.l1_; l2_=copy.l2_; l3_=copy.l3_;
  }
}

Tensor::Tensor(const Array1<double> &t) : l1_(0), l2_(0), l3_(0)
{
  mat_[0][0]=t[0];
  mat_[0][1]=mat_[1][0]=t[1];
  mat_[0][2]=mat_[2][0]=t[2];
  mat_[1][1]=t[3];
  mat_[1][2]=mat_[2][1]=t[4];
  mat_[2][2]=t[5];

  have_eigens_=0;
}

Tensor::Tensor(const std::vector<double> &t) : l1_(0), l2_(0), l3_(0)
{
  ASSERT(t.size() > 5);

  mat_[0][0]=t[0];
  mat_[0][1]=mat_[1][0]=t[1];
  mat_[0][2]=mat_[2][0]=t[2];
  mat_[1][1]=t[3];
  mat_[1][2]=mat_[2][1]=t[4];
  mat_[2][2]=t[5];

  have_eigens_=0;
}

Tensor::Tensor(const double *t) : l1_(0), l2_(0), l3_(0)
{
  mat_[0][0]=t[0];
  mat_[0][1]=mat_[1][0]=t[1];
  mat_[0][2]=mat_[2][0]=t[2];
  mat_[1][1]=t[3];
  mat_[1][2]=mat_[2][1]=t[4];
  mat_[2][2]=t[5];

  have_eigens_=0;
}

/// Initialize the diagonal to this value
Tensor::Tensor(double v) : l1_(0), l2_(0), l3_(0)
{
  have_eigens_=0;
  for (int i=0; i<3; i++)
    for (int j=0; j<3; j++)
      if (i==j) mat_[i][j]=v;
      else mat_[i][j]=0;
}

Tensor::Tensor(double v1, double v2, double v3, double v4, double v5, double v6) : l1_(0), l2_(0), l3_(0)
{
  have_eigens_=0;
  mat_[0][0] = v1;
  mat_[0][1] = v2;
  mat_[0][2] = v3;
  mat_[1][0] = v2;
  mat_[1][1] = v4;
  mat_[1][2] = v5;
  mat_[2][0] = v3;
  mat_[2][1] = v5;
  mat_[2][2] = v6;
}

/// Initialize the diagonal to this value
Tensor::Tensor(int v) : l1_(0), l2_(0), l3_(0)
{
  have_eigens_=0;
  for (int i=0; i<3; i++)
    for (int j=0; j<3; j++)
      if (i==j) mat_[i][j]=v;
      else mat_[i][j]=0;
}

Tensor::Tensor(const Vector &e1, const Vector &e2, const Vector &e3) :
  e1_(e1), e2_(e2), e3_(e3),
  l1_(e1.length()), l2_(e2.length()), l3_(e3.length())
{
  have_eigens_ = 1;
  reorderTensorValues();
  build_mat_from_eigens();
}

Tensor::Tensor(const double **cmat) : l1_(0), l2_(0), l3_(0)
{
  for (int i=0; i<3; i++)
    for (int j=0; j<3; j++)
      mat_[i][j]=cmat[i][j];
  have_eigens_=0;
}

void Tensor::build_mat_from_eigens() {
  if (!have_eigens_) return;
  double E[3][3];
  double S[3][3];
  double SE[3][3];
  Vector e1n(e1_);
  Vector e2n(e2_);
  Vector e3n(e3_);
  if (l1_ != 0) e1n.normalize();
  if (l2_ != 0) e2n.normalize();
  if (l3_ != 0) e3n.normalize();

  E[0][0] = e1n.x(); E[0][1] = e1n.y(); E[0][2] = e1n.z();
  E[1][0] = e2n.x(); E[1][1] = e2n.y(); E[1][2] = e2n.z();
  E[2][0] = e3n.x(); E[2][1] = e3n.y(); E[2][2] = e3n.z();
  S[0][0] = l1_; S[0][1] = 0;   S[0][2] = 0;
  S[1][0] = 0;   S[1][1] = l2_; S[1][2] = 0;
  S[2][0] = 0;   S[2][1] = 0;   S[2][2] = l3_;
  int i,j,k;
  for (i=0; i<3; i++)
    for (j=0; j<3; j++) {
      SE[i][j]=0;
      for (k=0; k<3; k++)
        SE[i][j] += S[i][k] * E[j][k];  // S x E-transpose
    }
    for (i=0; i<3; i++)
      for (j=0; j<3; j++) {
        mat_[i][j]=0;
        for (k=0; k<3; k++)
          mat_[i][j] += E[i][k] * SE[k][j];
      }
}

bool Tensor::operator==(const Tensor& t) const
{
  for(int i=0;i<3;i++)
    for(int j=0;j<3;j++)
      if( mat_[i][j]!=t.mat_[i][j])
        return false;

  return true;
}

bool Tensor::operator!=(const Tensor& t) const
{
  for(int i=0;i<3;i++)
    for(int j=0;j<3;j++)
      if( mat_[i][j]!=t.mat_[i][j])
        return true;

  return false;
}

Tensor& Tensor::operator=(const Tensor& copy)
{
  for(int i=0;i<3;i++)
    for(int j=0;j<3;j++)
      mat_[i][j]=copy.mat_[i][j];
  have_eigens_=copy.have_eigens_;
  if (have_eigens_) {
    e1_=copy.e1_; e2_=copy.e2_; e3_=copy.e3_;
    l1_=copy.l1_; l2_=copy.l2_; l3_=copy.l3_;
  }
  return *this;
}

Tensor& Tensor::operator=(const double& d)
{
  for(int i=0;i<3;i++)
    for(int j=0;j<3;j++)
      mat_[i][j]=d;
  have_eigens_=0;
  return *this;
}

/* matrix max norm */
double Tensor::norm() const
{
  double a = 0.0;
  double sum;
  for (int i=0;i<3;i++)
  {
    sum = 0.0;
    for (int j=0;j<3;j++) sum += fabs(mat_[i][j]);
    if (sum > a) a = sum;
  }
  return (a);
}

Vector Tensor::normalized_eigvals() const
{
  auto eigvals = Vector(l1_, l2_, l3_);
  eigvals.normalize();
  return eigvals;
}

// Frobenius norm
double Tensor::magnitude()
{
  double eigenval1, eigenval2, eigenval3;
  get_eigenvalues(eigenval1, eigenval2, eigenval3);

  double magnitude = sqrt(eigenval1 * eigenval1 +
                          eigenval2 * eigenval2 +
                          eigenval3 * eigenval3);
  return magnitude;
}

std::string Tensor::type_name(int) {
  static const std::string str("Tensor");
  return str;
}

Tensor Tensor::operator-(const Tensor& t) const
{
  Tensor t1(*this);
  t1.have_eigens_=0;
  for (int i=0; i<3; i++)
    for (int j=0; j<3; j++)
      t1.mat_[i][j]-=t.mat_[i][j];
  return t1;
}

Tensor& Tensor::operator-=(const Tensor& t)
{
  have_eigens_=0;
  for (int i=0; i<3; i++)
    for (int j=0; j<3; j++)
      mat_[i][j]-=t.mat_[i][j];
  return *this;
}

Tensor Tensor::operator+(const Tensor& t) const
{
  Tensor t1(*this);
  t1.have_eigens_=0;
  for (int i=0; i<3; i++)
    for (int j=0; j<3; j++)
      t1.mat_[i][j]+=t.mat_[i][j];
  return t1;
}

Tensor& Tensor::operator+=(const Tensor& t)
{
  have_eigens_=0;
  for (int i=0; i<3; i++)
    for (int j=0; j<3; j++)
      mat_[i][j]+=t.mat_[i][j];
  return *this;
}

Tensor Tensor::operator*(const double s) const
{
  Tensor t1(*this);
  for (int i=0; i<3; i++)
    for (int j=0; j<3; j++)
      t1.mat_[i][j]*=s;
  if (t1.have_eigens_) {
    t1.e1_*=s; t1.e2_*=s; t1.e3_*=s;
    t1.l1_*=s; t1.l2_*=s; t1.l3_*=s;
  }
  return t1;
}

Vector Tensor::operator*(const Vector& v) const
{
  return Vector(v.x()*mat_[0][0]+v.y()*mat_[0][1]+v.z()*mat_[0][2],
		v.x()*mat_[1][0]+v.y()*mat_[1][1]+v.z()*mat_[1][2],
		v.x()*mat_[2][0]+v.y()*mat_[2][1]+v.z()*mat_[2][2]);
}

Tensor Tensor::operator/(const double s) const
{
  Tensor t1(*this);
  for (int i=0; i<3; i++)
    for (int j=0; j<3; j++)
      t1.mat_[i][j]/=s;
  if (t1.have_eigens_) {
    t1.e1_/=s; t1.e2_/=s; t1.e3_/=s;
    t1.l1_/=s; t1.l2_/=s; t1.l3_/=s;
  }
  return t1;
}

void Tensor::build_eigens_from_mat()
{
  if (have_eigens_) return;
  DenseMatrix dm = DenseMatrix(3, 3);
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      dm(i,j) = mat_[i][j];

  auto es = Eigen::EigenSolver<Eigen::Matrix3d>(dm);
  auto vecs = es.eigenvectors();
  auto vals = es.eigenvalues();

  e1_ = Vector(real(vecs(0, 0)), real(vecs(0, 1)), real(vecs(0, 2)));
  e2_ = Vector(real(vecs(1, 0)), real(vecs(1, 1)), real(vecs(1, 2)));
  e3_ = Vector(real(vecs(2, 0)), real(vecs(2, 1)), real(vecs(2, 2)));
  l1_ = real(vals(0));
  l2_ = real(vals(1));
  l3_ = real(vals(2));
  have_eigens_ = 1;
  reorderTensorValues();
}

void Tensor::reorderTensorValues()
{
  if (!have_eigens_) return;
  std::vector<Vector> eigvecs = { e1_, e2_, e3_ };
  std::vector<double> eigvals = { l1_, l2_, l3_ };
  std::vector<std::pair<double, Vector>> sortList(3);
  for (int d = 0; d < 3; ++d)
    sortList[d] = std::make_pair(eigvals[d], eigvecs[d]);

  std::sort(std::begin(sortList), std::end(sortList),
            std::greater<std::pair<double, Vector>>());

  for (int d = 0; d < 3; ++d)
  {
    eigvals[d] = sortList[d].first;
    eigvecs[d] = sortList[d].second;
  }

  e1_ = eigvecs[0];
  e2_ = eigvecs[1];
  e3_ = eigvecs[2];
  l1_ = eigvals[0];
  l2_ = eigvals[1];
  l3_ = eigvals[2];
}

void Tensor::get_eigenvectors(Vector &e1, Vector &e2, Vector &e3)
{
  if (!have_eigens_) build_eigens_from_mat();
  e1=e1_; e2=e2_; e3=e3_;
}

const Vector Tensor::get_eigenvector1()
{
  if (!have_eigens_) build_eigens_from_mat();
  return e1_;
}

const Vector Tensor::get_eigenvector2()
{
  if (!have_eigens_) build_eigens_from_mat();
  return e2_;
}

const Vector Tensor::get_eigenvector3()
{
  if (!have_eigens_) build_eigens_from_mat();
  return e3_;
}

void Tensor::get_eigenvalues(double &l1, double &l2, double &l3)
{
  if (!have_eigens_) build_eigens_from_mat();
  l1=l1_; l2=l2_; l3=l3_;
}

void Tensor::set_eigens(const Vector &e1, const Vector &e2, const Vector &e3) {
  l1_ = e1.length(); l2_ = e2.length(); l3_ = e3.length();
  e1_ = e1 / l1_; e2_ = e2 / l2_; e3_ = e3 / l3_;
  have_eigens_ = 1;
  reorderTensorValues();
  build_mat_from_eigens();
}

void Tensor::set_outside_eigens(const Vector &e1, const Vector &e2,
				const Vector &e3,
				double v1, double v2, double v3)
{
  e1_ = e1; e2_ = e2; e3_ = e3;
  l1_ = v1; l2_ = v2; l3_ = v3;
  have_eigens_ = 1;
  reorderTensorValues();
  build_mat_from_eigens();
}

DenseColumnMatrix Tensor::mandel()
{
  if (!have_eigens_) build_eigens_from_mat();
  std::vector<Vector> eigvecs(3);
  get_eigenvectors(eigvecs[0], eigvecs[1], eigvecs[2]);
  std::vector<double> eigvals(3);
  get_eigenvalues(eigvals[0], eigvals[1], eigvals[2]);
  const static double sqrt2 = std::sqrt(2);
  for (int i = 0; i < 3; ++i)
    eigvecs[i] *= eigvals[i];

  auto mandel = DenseColumnMatrix(6);
  // eigvec1.normalize();
  // eigvec2.normalize();
  // eigvec3.normalize();
  for(int i = 0; i < 3; ++i)
    // mandel.put(i, eigvecs[i][i]);
    mandel.put(i, mat_[i][i]);
  // mandel.put(3, eigvecs[0][1] * sqrt2);
  // mandel.put(4, eigvecs[0][2] * sqrt2);
  // mandel.put(5, eigvecs[1][2] * sqrt2);
  mandel.put(3, mat_[0][1] * sqrt2);
  mandel.put(4, mat_[0][2] * sqrt2);
  mandel.put(5, mat_[1][2] * sqrt2);

  return mandel;
}

double Tensor::eigenValueSum() const
{
  return l1_ + l2_ + l3_;
}

double Tensor::linearCertainty() const
{
  return (l1_ - l2_) / eigenValueSum();
}

double Tensor::planarCertainty() const
{
  return 2.0 * (l2_ - l3_) / eigenValueSum();
}

double Tensor::sphericalCertainty() const
{
  return 3.0 * (l3_) / eigenValueSum();
}

void Core::Geometry::Pio(Piostream& stream, Tensor& t)
{
  stream.begin_cheap_delim();

  Pio(stream, t.mat_[0][0]);
  Pio(stream, t.mat_[0][1]);
  Pio(stream, t.mat_[0][2]);
  Pio(stream, t.mat_[1][1]);
  Pio(stream, t.mat_[1][2]);
  Pio(stream, t.mat_[2][2]);

  t.mat_[1][0]=t.mat_[0][1];
  t.mat_[2][0]=t.mat_[0][2];
  t.mat_[2][1]=t.mat_[1][2];

  Pio(stream, t.have_eigens_);
  if (t.have_eigens_)
  {
    Pio(stream, t.e1_);
    Pio(stream, t.e2_);
    Pio(stream, t.e3_);
    Pio(stream, t.l1_);
    Pio(stream, t.l2_);
    Pio(stream, t.l3_);
  }

  stream.end_cheap_delim();
}

const std::string&
Tensor::get_h_file_path() {
  static const std::string path(TypeDescription::cc_to_h(__FILE__));
  return path;
}

const TypeDescription* Core::Geometry::get_type_description(Tensor*)
{
  static TypeDescription* td = 0;
  if(!td){
    td = new TypeDescription("Tensor", Tensor::get_h_file_path(),
				"SCIRun",
				TypeDescription::DATA_E);
  }
  return td;
}


std::ostream& Core::Geometry::operator<<( std::ostream& os, const Tensor& t )
{
  os << '[' << t.mat_[0][0] << ' ' << t.mat_[0][1] << ' ' << t.mat_[0][2]
     << ' ' << t.mat_[1][0] << ' ' << t.mat_[1][1] << ' ' << t.mat_[1][2]
     << ' ' << t.mat_[2][0] << ' ' << t.mat_[2][1] << ' ' << t.mat_[2][2]
     << ']';

  return os;
}

std::istream& Core::Geometry::operator>>(std::istream& is, Tensor& t)
{
  t = Tensor();
  is >> t.mat_[0][0] >> t.mat_[0][1] >> t.mat_[0][2]
     >> t.mat_[1][0] >> t.mat_[1][1] >> t.mat_[1][2]
     >> t.mat_[2][0] >> t.mat_[2][1] >> t.mat_[2][2];

  return is;
}
