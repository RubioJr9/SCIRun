/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2012 Scientific Computing and Imaging Institute,
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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <Core/Dataflow/Network/Network.h>
#include <Core/Dataflow/Network/ModuleInterface.h>
#include <Core/Dataflow/Network/ConnectionId.h>
#include <Core/Dataflow/Network/Tests/MockNetwork.h>
#include <Core/Datatypes/DenseMatrix.h>
#include <Core/Datatypes/MatrixComparison.h>
#include <Core/Datatypes/MatrixIO.h>
#include <Modules/Basic/SendTestMatrix.h>
#include <Modules/Basic/ReceiveTestMatrix.h>
#include <Modules/Math/EvaluateLinearAlgebraUnary.h>
#include <Modules/Factory/HardCodedModuleFactory.h>
#include <Algorithms/Math/EvaluateLinearAlgebraUnary.h>

using namespace SCIRun;
using namespace SCIRun::Modules::Basic;
using namespace SCIRun::Modules::Math;
using namespace SCIRun::Modules::Factory;
using namespace SCIRun::Domain::Datatypes;
using namespace SCIRun::Domain::Networks;
using namespace SCIRun::Domain::Networks::Mocks;
using namespace SCIRun::Algorithms::Math;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::DefaultValue;
using ::testing::Return;

//TODO DAN

namespace
{
  DenseMatrixHandle matrix1()
  {
    DenseMatrixHandle m(new DenseMatrix(3, 3));
    for (size_t i = 0; i < m->nrows(); ++i)
      for (size_t j = 0; j < m->ncols(); ++j)
        (*m)(i, j) = 3.0 * i + j;
    return m;
  }
  DenseMatrixHandle matrix2()
  {
    DenseMatrixHandle m(new DenseMatrix(3, 3));
    for (size_t i = 0; i < m->nrows(); ++i)
      for (size_t j = 0; j < m->ncols(); ++j)
        (*m)(i, j) = -2.0 * i + j;
    return m;
  }
  const DenseMatrix Zero(DenseMatrix::zero_matrix(3,3));

  ModuleHandle addModuleToNetwork(Network& network, const std::string& moduleName)
  {
    ModuleLookupInfo info;
    info.module_name_ = moduleName;
    return network.add_module(info);
  }
}

TEST(EvaluateLinearAlgebraUnaryFunctionalTest, CanExecuteManuallyWithHardCodedOperation)
{
  ModuleFactoryHandle mf(new HardCodedModuleFactory);
  Network matrixUnaryNetwork(mf);
  
  ModuleHandle send = addModuleToNetwork(matrixUnaryNetwork, "SendTestMatrix");
  ModuleHandle process = addModuleToNetwork(matrixUnaryNetwork, "EvaluateLinearAlgebraUnary");
  ModuleHandle receive = addModuleToNetwork(matrixUnaryNetwork, "ReceiveTestMatrix");

  EXPECT_EQ(3, matrixUnaryNetwork.nmodules());

  matrixUnaryNetwork.connect(send, 0, process, 0);
  EXPECT_EQ(1, matrixUnaryNetwork.nconnections());
  matrixUnaryNetwork.connect(process, 0, receive, 0);
  EXPECT_EQ(2, matrixUnaryNetwork.nconnections());

  SendTestMatrixModule* sendModule = dynamic_cast<SendTestMatrixModule*>(send.get());
  ASSERT_TRUE(sendModule != 0);
  EvaluateLinearAlgebraUnaryModule* evalModule = dynamic_cast<EvaluateLinearAlgebraUnaryModule*>(process.get());
  ASSERT_TRUE(evalModule != 0);

  DenseMatrixHandle input = matrix1();
  sendModule->setMatrix(input);

  //manually execute the network, in the correct order.
  send->execute();
  process->execute();
  receive->execute();

  ReceiveTestMatrixModule* receiveModule = dynamic_cast<ReceiveTestMatrixModule*>(receive.get());
  ASSERT_TRUE(receiveModule != 0);
  ASSERT_TRUE(receiveModule->latestReceivedMatrix());

  EXPECT_EQ(-*input, *receiveModule->latestReceivedMatrix());
}

TEST(EvaluateLinearAlgebraUnaryFunctionalTest, CanExecuteManuallyWithChoiceOfOperation)
{
  ModuleFactoryHandle mf(new HardCodedModuleFactory);
  Network matrixUnaryNetwork(mf);

  ModuleHandle send = addModuleToNetwork(matrixUnaryNetwork, "SendTestMatrix");
  ModuleHandle process = addModuleToNetwork(matrixUnaryNetwork, "EvaluateLinearAlgebraUnary");
  ModuleHandle receive = addModuleToNetwork(matrixUnaryNetwork, "ReceiveTestMatrix");

  EXPECT_EQ(3, matrixUnaryNetwork.nmodules());

  matrixUnaryNetwork.connect(send, 0, process, 0);
  EXPECT_EQ(1, matrixUnaryNetwork.nconnections());
  matrixUnaryNetwork.connect(process, 0, receive, 0);
  EXPECT_EQ(2, matrixUnaryNetwork.nconnections());

  SendTestMatrixModule* sendModule = dynamic_cast<SendTestMatrixModule*>(send.get());
  ASSERT_TRUE(sendModule != 0);
  EvaluateLinearAlgebraUnaryModule* evalModule = dynamic_cast<EvaluateLinearAlgebraUnaryModule*>(process.get());
  ASSERT_TRUE(evalModule != 0);

  DenseMatrixHandle input = matrix1();
  sendModule->setMatrix(input);

  //evalModule->set_operation(EvaluateLinearAlgebraUnaryAlgorithm::NEGATE);
  //manually execute the network, in the correct order.
  send->execute();
  process->execute();
  receive->execute();

  ReceiveTestMatrixModule* receiveModule = dynamic_cast<ReceiveTestMatrixModule*>(receive.get());
  ASSERT_TRUE(receiveModule != 0);
  ASSERT_TRUE(receiveModule->latestReceivedMatrix());

  EXPECT_EQ(-*input, *receiveModule->latestReceivedMatrix());

  //evalModule->set_operation(EvaluateLinearAlgebraUnaryAlgorithm::TRANSPOSE);
  process->execute();
  receive->execute();
  EXPECT_EQ(transpose(*input), *receiveModule->latestReceivedMatrix());

  //evalModule->set_operation(EvaluateLinearAlgebraUnaryAlgorithm::Parameters(EvaluateLinearAlgebraUnaryAlgorithm::SCALAR_MULTIPLY, 2.0));
  process->execute();
  receive->execute();
  EXPECT_EQ(2.0 * *input, *receiveModule->latestReceivedMatrix());
}


TEST(MatrixCalculatorFunctionalTest, ManualExecutionOfMultiNodeNetwork)
{
  std::cout << "m1" << std::endl;
  std::cout << *matrix1() << std::endl;
  std::cout << "m2" << std::endl;
  std::cout << *matrix2() << std::endl;
  std::cout << "(-m1 * 4m2) + trans(m1)" << std::endl;
  std::cout << (-*matrix1()) * (4* *matrix2()) + transpose(*matrix1()) << std::endl;

  //Test network:
  /*
  send m1             send m2
  |         |         |
  transpose negate    scalar mult *4
  |         |         |
  |           multiply
  |           |
        add
        |      |
        report receive
  */

  ModuleFactoryHandle mf(new HardCodedModuleFactory);
  Network matrixMathNetwork(mf);
  ModuleHandle matrix1Send = addModuleToNetwork(matrixMathNetwork, "SendTestMatrix");
  ModuleHandle matrix2Send = addModuleToNetwork(matrixMathNetwork, "SendTestMatrix");
  
  ModuleHandle transpose = addModuleToNetwork(matrixMathNetwork, "EvaluateLinearAlgebraUnary");
  ModuleHandle negate = addModuleToNetwork(matrixMathNetwork, "EvaluateLinearAlgebraUnary");
  ModuleHandle scalar = addModuleToNetwork(matrixMathNetwork, "EvaluateLinearAlgebraUnary");

  ModuleHandle multiply = addModuleToNetwork(matrixMathNetwork, "EvaluateLinearAlgebraBinary");
  ModuleHandle add = addModuleToNetwork(matrixMathNetwork, "EvaluateLinearAlgebraBinary");

  ModuleHandle report = addModuleToNetwork(matrixMathNetwork, "ReportMatrixInfo");
  ModuleHandle receive = addModuleToNetwork(matrixMathNetwork, "ReceiveTestMatrix");
  
  EXPECT_EQ(9, matrixMathNetwork.nmodules());

  for (size_t i = 0; i < matrixMathNetwork.nmodules(); ++i)
  {
    ModuleHandle m = matrixMathNetwork.module(i);
    std::cout << m->get_module_name() << std::endl;
    std::cout << "inputs: " << m->num_input_ports() << std::endl;
    std::cout << "outputs: " << m->num_output_ports() << std::endl;
    std::cout << "has ui: " << m->has_ui() << std::endl;
  }

  EXPECT_EQ(0, matrixMathNetwork.nconnections());
  matrixMathNetwork.connect(matrix1Send, 0, transpose, 0);
  EXPECT_EQ(1, matrixMathNetwork.nconnections());
  matrixMathNetwork.connect(matrix1Send, 0, negate, 0);
  EXPECT_EQ(2, matrixMathNetwork.nconnections());
  matrixMathNetwork.connect(matrix2Send, 0, scalar, 0);
  EXPECT_EQ(3, matrixMathNetwork.nconnections());
  matrixMathNetwork.connect(negate, 0, multiply, 0);
  EXPECT_EQ(4, matrixMathNetwork.nconnections());
  matrixMathNetwork.connect(scalar, 0, multiply, 1);
  EXPECT_EQ(5, matrixMathNetwork.nconnections());
  matrixMathNetwork.connect(transpose, 0, add, 0);
  EXPECT_EQ(6, matrixMathNetwork.nconnections());
  matrixMathNetwork.connect(multiply, 0, add, 1);
  EXPECT_EQ(7, matrixMathNetwork.nconnections());
  matrixMathNetwork.connect(add, 0, report, 0);
  EXPECT_EQ(8, matrixMathNetwork.nconnections());
  matrixMathNetwork.connect(add, 0, receive, 0);
  EXPECT_EQ(9, matrixMathNetwork.nconnections());

  //Set module parameters.
  matrix1Send->get_state()["MatrixToSend"] = matrix1();
  matrix2Send->get_state()["MatrixToSend"] = matrix2();
  transpose->get_state()["Operation"] = EvaluateLinearAlgebraUnaryAlgorithm::TRANSPOSE;
  negate->get_state()["Operation"] = EvaluateLinearAlgebraUnaryAlgorithm::NEGATE;
  scalar->get_state()["Operation"] = EvaluateLinearAlgebraUnaryAlgorithm::Parameters(EvaluateLinearAlgebraUnaryAlgorithm::SCALAR_MULTIPLY, 4.0);
  multiply->get_state()["Operation"] = EvaluateLinearAlgebraBinaryAlgorithm::MULTIPLY;
  add->get_state()["Operation"] = EvaluateLinearAlgebraBinaryAlgorithm::ADD;
  
  //execute all manually, in order
  matrix1Send->execute();
  matrix2Send->execute();
  transpose->execute();
  scalar->execute();
  negate->execute();
  multiply->execute();
  add->execute();
  report->execute();
  receive->execute();

  //grab reporting module state
  ReportMatrixInfoAlgorithm::Outputs reportOutput = report->get_state()["ReportedInfo"];
  DenseMatrixHandle receivedMatrix = receive->get_state()["ReceivedMatrix"];

  //verify results

  EXPECT_TRUE(false);
}