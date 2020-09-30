/*******************************************************************************
* Copyright 2020 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#include "gtest/gtest.h"
#include "oneapi/dal/algo/decision_forest/infer.hpp"
#include "oneapi/dal/algo/decision_forest/test/utils.hpp"
#include "oneapi/dal/algo/decision_forest/train.hpp"

using namespace oneapi;
namespace df = oneapi::dal::decision_forest;

TEST(infer_and_train_reg_kernels_test, can_process_simple_case_default_params) {
    const double mse_threshold = 0.05;
    constexpr std::int64_t row_count_train = 10;
    constexpr std::int64_t row_count_test = 5;
    constexpr std::int64_t column_count = 2;

    const float x_train[] = {
        0.1f,  0.25f, 0.15f, 0.35f, 0.25f, 0.55f, 0.3f, 0.65f, 0.4f, 0.85f,
        0.45f, 0.95f, 0.55f, 1.15f, 0.6f,  1.25f, 0.7f, 1.45f, 0.8f, 1.65f,
    };

    const float y_train[] = {
        0.0079f, 0.0160f, 0.0407f, 0.0573f, 0.0989f, 0.1240f, 0.1827f, 0.2163f, 0.2919f, 0.3789f,
    };

    const float x_test[] = {
        0.2f, 0.45f, 0.35f, 0.75f, 0.5f, 1.05f, 0.65f, 1.35f, 0.75f, 1.55f,
    };

    const float y_test[] = {
        0.0269f, 0.0767f, 0.1519f, 0.2527f, 0.3340f,
    };
    const auto x_train_table = dal::homogen_table::wrap(x_train, row_count_train, column_count);
    const auto y_train_table = dal::homogen_table::wrap(y_train, row_count_train, 1);

    const auto x_test_table = dal::homogen_table::wrap(x_test, row_count_test, column_count);

    const auto df_desc = df::descriptor<float, df::task::regression, df::method::dense>{};
    const auto result_train = dal::train(df_desc, x_train_table, y_train_table);
    ASSERT_EQ(!(result_train.get_var_importance().has_data()), true);
    ASSERT_EQ(!(result_train.get_oob_err().has_data()), true);
    ASSERT_EQ(!(result_train.get_oob_err_per_observation().has_data()), true);

    const auto result_infer = dal::infer(df_desc, result_train.get_model(), x_test_table);

    auto labels_table = result_infer.get_labels();
    ASSERT_EQ(labels_table.has_data(), true);
    ASSERT_EQ(labels_table.get_row_count(), row_count_test);
    ASSERT_EQ(labels_table.get_column_count(), 1);

    ASSERT_LE(calculate_mse(result_infer.get_labels(), y_test), mse_threshold);
}

TEST(infer_and_train_reg_kernels_test, can_process_simple_case_non_default_params) {
    const double mse_threshold = 0.05;
    constexpr std::int64_t row_count_train = 10;
    constexpr std::int64_t row_count_test = 5;
    constexpr std::int64_t column_count = 2;
    constexpr std::int64_t tree_count = 10;

    const float x_train[] = {
        0.1f,  0.25f, 0.15f, 0.35f, 0.25f, 0.55f, 0.3f, 0.65f, 0.4f, 0.85f,
        0.45f, 0.95f, 0.55f, 1.15f, 0.6f,  1.25f, 0.7f, 1.45f, 0.8f, 1.65f,
    };

    const float y_train[] = {
        0.0079f, 0.0160f, 0.0407f, 0.0573f, 0.0989f, 0.1240f, 0.1827f, 0.2163f, 0.2919f, 0.3789f,
    };

    const float x_test[] = {
        0.2f, 0.45f, 0.35f, 0.75f, 0.5f, 1.05f, 0.65f, 1.35f, 0.75f, 1.55f,
    };

    const float y_test[] = {
        0.0269f, 0.0767f, 0.1519f, 0.2527f, 0.3340f,
    };
    const auto x_train_table = dal::homogen_table::wrap(x_train, row_count_train, column_count);
    const auto y_train_table = dal::homogen_table::wrap(y_train, row_count_train, 1);

    const auto x_test_table = dal::homogen_table::wrap(x_test, row_count_test, column_count);

    const auto df_desc =
        df::descriptor<float, df::task::regression, df::method::dense>{}
            .set_tree_count(tree_count)
            .set_features_per_node(1)
            .set_min_observations_in_leaf_node(2)
            .set_variable_importance_mode(df::variable_importance_mode::mdi)
            .set_error_metric_mode(df::error_metric_mode::out_of_bag_error |
                                   df::error_metric_mode::out_of_bag_error_per_observation);

    const auto result_train = dal::train(df_desc, x_train_table, y_train_table);
    ASSERT_EQ(result_train.get_model().get_tree_count(), tree_count);
    ASSERT_EQ(result_train.get_var_importance().has_data(), true);
    ASSERT_EQ(result_train.get_var_importance().get_column_count(), column_count);
    ASSERT_EQ(result_train.get_var_importance().get_row_count(), 1);
    ASSERT_EQ(result_train.get_oob_err().has_data(), true);
    ASSERT_EQ(result_train.get_oob_err().get_row_count(), 1);
    ASSERT_EQ(result_train.get_oob_err().get_column_count(), 1);
    ASSERT_EQ(result_train.get_oob_err_per_observation().has_data(), true);
    ASSERT_EQ(result_train.get_oob_err_per_observation().get_row_count(), row_count_train);
    ASSERT_EQ(result_train.get_oob_err_per_observation().get_column_count(), 1);

    verify_oob_err_vs_oob_err_per_observation(result_train.get_oob_err(),
                                              result_train.get_oob_err_per_observation(),
                                              mse_threshold);

    const auto result_infer = dal::infer(df_desc, result_train.get_model(), x_test_table);

    auto labels_table = result_infer.get_labels();
    ASSERT_EQ(labels_table.has_data(), true);
    ASSERT_EQ(labels_table.get_row_count(), row_count_test);
    ASSERT_EQ(labels_table.get_column_count(), 1);

    ASSERT_LE(calculate_mse(result_infer.get_labels(), y_test), mse_threshold);
}
