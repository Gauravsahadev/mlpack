/**
 * @file perceptron_main.cpp
 * @author Udit Saxena
 *
 * This program runs the Simple Perceptron Classifier.
 *
 * Perceptrons are simple single-layer binary classifiers, which solve linearly
 * separable problems with a linear decision boundary.
 *
 * mlpack is free software; you may redistribute it and/or modify it under the
 * terms of the 3-clause BSD license.  You should have received a copy of the
 * 3-clause BSD license along with mlpack.  If not, see
 * http://www.opensource.org/licenses/BSD-3-Clause for more information.
 */
#include <mlpack/prereqs.hpp>
#include <mlpack/core/util/cli.hpp>
#include <mlpack/core/data/normalize_labels.hpp>
#include <mlpack/core/util/mlpack_main.hpp>

#include "perceptron.hpp"

using namespace mlpack;
using namespace mlpack::perceptron;
using namespace std;
using namespace arma;

PROGRAM_INFO("Perceptron",
    "This program implements a perceptron, which is a single level neural "
    "network. The perceptron makes its predictions based on a linear predictor "
    "function combining a set of weights with the feature vector.  The "
    "perceptron learning rule is able to converge, given enough iterations "
    "(specified using the " + PRINT_PARAM_STRING("max_iterations") +
    " parameter), if the data supplied is linearly separable.  The perceptron "
    "is parameterized by a matrix of weight vectors that denote the numerical "
    "weights of the neural network."
    "\n\n"
    "This program allows loading a perceptron from a model (via the " +
    PRINT_PARAM_STRING("input_model") + " parameter) or training a perceptron "
    "given training data (via the " + PRINT_PARAM_STRING("training") +
    " parameter), or both those things at once.  In addition, this program "
    "allows classification on a test dataset (via the " +
    PRINT_PARAM_STRING("test") + " parameter) and the classification results "
    "on the test set may be saved with the " + PRINT_PARAM_STRING("output") +
    "output parameter.  The perceptron model may be saved with the " +
    PRINT_PARAM_STRING("output_model") + " output parameter."
    "\n\n"
    "The training data given with the " + PRINT_PARAM_STRING("training") +
    " option may have class labels as its last dimension (so, if the training "
    "data is in CSV format, labels should be the last column).  Alternately, "
    "the " + PRINT_PARAM_STRING("labels") + " parameter may be used to specify "
    "a separate matrix of labels."
    "\n\n"
    "All these options make it easy to train a perceptron, and then re-use that"
    " perceptron for later classification.  The invocation below trains a "
    "perceptron on " + PRINT_DATASET("training_data") + " with labels " +
    PRINT_DATASET("training_labels") + ", and saves the model to " +
    PRINT_MODEL("perceptron") + "."
    "\n\n" +
    PRINT_CALL("perceptron", "training", "training_data", "labels",
        "training_labels", "output_model", "perceptron") +
    "\n\n"
    "Then, this model can be re-used for classification on the test data " +
    PRINT_DATASET("test_data") + ".  The example below does precisely that, "
    "saving the predicted classes to " + PRINT_DATASET("predictions") + "."
    "\n\n" +
    PRINT_CALL("perceptron", "input_model", "perceptron", "test", "test_data",
        "output", "predictions") +
    "\n\n"
    "Note that all of the options may be specified at once: predictions may be "
    "calculated right after training a model, and model training can occur even"
    " if an existing perceptron model is passed with the " +
    PRINT_PARAM_STRING("input_model") + " parameter.  However, note that the "
    "number of classes and the dimensionality of all data must match.  So you "
    "cannot pass a perceptron model trained on 2 classes and then re-train with"
    " a 4-class dataset.  Similarly, attempting classification on a "
    "3-dimensional dataset with a perceptron that has been trained on 8 "
    "dimensions will cause an error.");

// When we save a model, we must also save the class mappings.  So we use this
// auxiliary structure to store both the perceptron and the mapping, and we'll
// save this.
class PerceptronModel
{
 private:
  Perceptron<> p;
  Col<size_t> map;

 public:
  Perceptron<>& P() { return p; }
  const Perceptron<>& P() const { return p; }

  Col<size_t>& Map() { return map; }
  const Col<size_t>& Map() const { return map; }

  template<typename Archive>
  void Serialize(Archive& ar, const unsigned int /* version */)
  {
    ar & data::CreateNVP(p, "perceptron");
    ar & data::CreateNVP(map, "mappings");
  }
};

// Training parameters.
PARAM_MATRIX_IN("training", "A matrix containing the training set.", "t");
PARAM_UROW_IN("labels", "A matrix containing labels for the training set.",
    "l");
PARAM_INT_IN("max_iterations", "The maximum number of iterations the "
    "perceptron is to be run", "n", 1000);

// Model loading/saving.
PARAM_MODEL_IN(PerceptronModel, "input_model", "Input perceptron model.", "m");
PARAM_MODEL_OUT(PerceptronModel, "output_model", "Output for trained perceptron"
    " model.", "M");

// Testing/classification parameters.
PARAM_MATRIX_IN("test", "A matrix containing the test set.", "T");
PARAM_UROW_OUT("output", "The matrix in which the predicted labels for the"
    " test set will be written.", "o");

void mlpackMain()
{
  // First, get all parameters and validate them.
  const size_t maxIterations = (size_t) CLI::GetParam<int>("max_iterations");

  // We must either load a model or train a model.
  if (!CLI::HasParam("input_model") && !CLI::HasParam("training"))
    Log::Fatal << "Either an input model must be specified with "
        << "--input_model_file or training data must be given "
        << "(--training_file)!" << endl;

  // If the user isn't going to save the output model or any predictions, we
  // should issue a warning.
  if (!CLI::HasParam("output_model") && !CLI::HasParam("test"))
    Log::Warn << "Output will not be saved!  (Neither --test_file nor "
        << "--output_model_file are specified.)" << endl;

  if (!CLI::HasParam("test") && CLI::HasParam("output"))
    Log::Warn << "--output_file will be ignored because --test_file is not "
        << "specified." << endl;

  if (CLI::HasParam("test") && !CLI::HasParam("output"))
    Log::Warn << "--output_file not specified, so the predictions for "
        << "--test_file will not be saved." << endl;

  // Now, load our model, if there is one.
  PerceptronModel p;
  if (CLI::HasParam("input_model"))
  {
    Log::Info << "Loading saved perceptron from model file '"
        << CLI::GetPrintableParam<PerceptronModel>("input_model") << "'."
        << endl;

    p = std::move(CLI::GetParam<PerceptronModel>("input_model"));
  }

  // Next, load the training data and labels (if they have been given).
  if (CLI::HasParam("training"))
  {
    Log::Info << "Training perceptron on dataset '"
        << CLI::GetPrintableParam<mat>("training");
    if (CLI::HasParam("labels"))
      Log::Info << "' with labels in '"
          << CLI::GetPrintableParam<Row<size_t>>("labels") << "'";
    else
      Log::Info << "'";
    Log::Info << " for a maximum of " << maxIterations << " iterations."
        << endl;

    mat trainingData = std::move(CLI::GetParam<mat>("training"));

    // Load labels.
    Row<size_t> labelsIn;

    // Did the user pass in labels?
    if (CLI::HasParam("labels"))
    {
      labelsIn = std::move(CLI::GetParam<Row<size_t>>("labels"));
    }
    else
    {
      // Use the last row of the training data as the labels.
      Log::Info << "Using the last dimension of training set as labels."
          << endl;
      labelsIn = arma::conv_to<Row<size_t>>::from(
          trainingData.row(trainingData.n_rows - 1));
      trainingData.shed_row(trainingData.n_rows - 1);
    }

    // Normalize the labels.
    Row<size_t> labels;
    data::NormalizeLabels(labelsIn, labels, p.Map());
    const size_t numClasses = p.Map().n_elem;

    // Now, if we haven't already created a perceptron, do it.  Otherwise, make
    // sure the dimensions are right, then continue training.
    if (!CLI::HasParam("input_model"))
    {
      // Create and train the classifier.
      Timer::Start("training");
      p.P() = Perceptron<>(trainingData, labels, numClasses, maxIterations);
      Timer::Stop("training");
    }
    else
    {
      // Check dimensionality.
      if (p.P().Weights().n_rows != trainingData.n_rows)
      {
        Log::Fatal << "Perceptron from '"
            << CLI::GetPrintableParam<PerceptronModel>("input_model")
            << "' is built on data with " << p.P().Weights().n_rows
            << " dimensions, but data in '"
            << CLI::GetPrintableParam<arma::mat>("training") << "' has "
            << trainingData.n_rows << "dimensions!" << endl;
      }

      // Check the number of labels.
      if (numClasses > p.P().Weights().n_cols)
      {
        Log::Fatal << "Perceptron from '"
            << CLI::GetPrintableParam<PerceptronModel>("input_model") << "' "
            << "has " << p.P().Weights().n_cols << " classes, but the training "
            << "data has " << numClasses + 1 << " classes!" << endl;
      }

      // Now train.
      Timer::Start("training");
      p.P().MaxIterations() = maxIterations;
      p.P().Train(trainingData, labels.t(), numClasses);
      Timer::Stop("training");
    }
  }

  // Now, the training procedure is complete.  Do we have any test data?
  if (CLI::HasParam("test"))
  {
    Log::Info << "Classifying dataset '"
        << CLI::GetPrintableParam<arma::mat>("test") << "'." << endl;
    mat testData = std::move(CLI::GetParam<arma::mat>("test"));

    if (testData.n_rows != p.P().Weights().n_rows)
    {
      Log::Fatal << "Test data dimensionality (" << testData.n_rows << ") must "
          << "be the same as the dimensionality of the perceptron ("
          << p.P().Weights().n_rows << ")!" << endl;
    }

    // Time the running of the perceptron classifier.
    Row<size_t> predictedLabels(testData.n_cols);
    Timer::Start("testing");
    p.P().Classify(testData, predictedLabels);
    Timer::Stop("testing");

    // Un-normalize labels to prepare output.
    Row<size_t> results;
    data::RevertLabels(predictedLabels, p.Map(), results);

    // Save the predicted labels.
    if (CLI::HasParam("output"))
      CLI::GetParam<arma::Row<size_t>>("output") = std::move(results);
  }

  // Lastly, do we need to save the output model?
  if (CLI::HasParam("output_model"))
    CLI::GetParam<PerceptronModel>("output_model") = std::move(p);
}
