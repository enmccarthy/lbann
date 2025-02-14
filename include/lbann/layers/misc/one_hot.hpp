////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014-2019, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory.
// Written by the LBANN Research Team (B. Van Essen, et al.) listed in
// the CONTRIBUTORS file. <lbann-dev@llnl.gov>
//
// LLNL-CODE-697807.
// All rights reserved.
//
// This file is part of LBANN: Livermore Big Artificial Neural Network
// Toolkit. For details, see http://software.llnl.gov/LBANN or
// https://github.com/LLNL/LBANN.
//
// Licensed under the Apache License, Version 2.0 (the "Licensee"); you
// may not use this file except in compliance with the License.  You may
// obtain a copy of the License at:
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the license.
////////////////////////////////////////////////////////////////////////////////

#ifndef LBANN_LAYERS_MISC_ONE_HOT_HPP_INCLUDED
#define LBANN_LAYERS_MISC_ONE_HOT_HPP_INCLUDED

#include "lbann/layers/layer.hpp"

namespace lbann {

/** @brief Convert index to a one-hot vector
 *
 *  Expects a scalar input tensor and outputs a 1-D output tensor with
 *  @c size entries. The input is interpreted as an index, and output
 *  entries are one if they correspond to that index and zero
 *  otherwise. If the input is outside @f$[0,\text{size})@f$, then the
 *  output is all zeros.
 */
template <data_layout Layout, El::Device Device>
class one_hot_layer : public Layer {
public:

  one_hot_layer(lbann_comm* comm, size_t size) : Layer(comm) {
    set_output_dims({static_cast<int>(size)});
    static_assert(Layout == data_layout::DATA_PARALLEL,
                  "one-hot layer only supports data-parallel layout");
  }
  one_hot_layer* copy() const override { return new one_hot_layer(*this); }
  std::string get_type() const override { return "one-hot"; }
  data_layout get_data_layout() const override { return Layout; }
  El::Device get_device_allocation() const override { return Device; }

protected:

  void setup_dims() override {
    Layer::setup_dims();

    // Make sure input tensor is scalar
    if (get_input_size() != 1) {
      const auto input_dims = get_input_dims();
      std::ostringstream dim_ss;
      for (size_t i = 0; i < input_dims.size(); ++i) {
        dim_ss << (i > 0 ? "x" : "") << input_dims[i];
      }
      LBANN_ERROR(get_type()," layer \"",get_name(),"\" ",
                  "received an input tensor with invalid dimensions ",
                  "(expected 1, got ",dim_ss.str(),")");
    }

  }

  void fp_compute() override;

};

} // namespace lbann

#endif // LBANN_LAYERS_MISC_ONE_HOT_HPP_INCLUDED
