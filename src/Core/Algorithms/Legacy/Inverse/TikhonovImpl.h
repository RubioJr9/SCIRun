/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2015 Scientific Computing and Imaging Institute,
   University of Utah.

   License for the specific language governing rights and limitations under
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

#ifndef BioPSE_TikhonovImpl_H__
#define BioPSE_TikhonovImpl_H__

#include <Core/Algorithms/Field/share.h>


namespace SCIRun {
namespace Core {
namespace Algorithms {
namespace Inverse {


	class SCISHARE TikhonovImpl
	{

	public:

		// constructor
		TikhonovImpl() {};

		virtual SCIRun::Core::Datatypes::DenseMatrix computeInverseSolution( double lambda_sq, bool inverseCalculation) const = 0;

		// default lambda step. Can ve overriden if necessary (see TSVD as reference)
		virtual std::vector<double> computeLambdaArray( double lambdaMin, double lambdaMax, int nLambda ) const {

			std::vector<double> lambdaArray(nLambda,0.0);
			double lam_step = (log10(lambdaMax) - log10(lambdaMin)) / (nLambda-1);

			lambdaArray[0] = lambdaMin;
			for (int j = 1; j < nLambda; j++)
		    {
		        lambdaArray[j] = lambdaArray[j-1] * pow(10.0,lam_step);
			}

			return lambdaArray;
		};

	};

	}}}}

#endif
