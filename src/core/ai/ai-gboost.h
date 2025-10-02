#pragma once

#include <common.h>

typedef enum {
    AI_GB_ERR_OK,     ///< No error
    AI_GB_ERR_NO_MEM, ///< Memory allocation error
    AI_GB_ERR_CREATE, ///< Model creation error
    AI_GB_ERR_PARAM,  ///< Parameter setting error
    AI_GB_ERR_TRAIN,  ///< Training error
    AI_GB_ERR_SAVE,   ///< Model saving error
    AI_GB_ERR_MAX,
} ai_gb_err_t;

/**
 * @brief Train a gradient boosting model and save it to a file
 * @param train_data - [in] Pointer to the training data (row-major order)
 * @param labels - [in] Pointer to the labels
 * @param num_rows - [in] Number of rows in the training data
 * @param num_cols - [in] Number of columns (features) in the training data
 * @param model_path - [in] Path to save the trained model
 * @return AI_GB_ERR_OK on success, error code otherwise
 */
ai_gb_err_t ai_gb_train_model(const float *train_data, const float *labels, uint32_t num_rows, uint32_t num_cols,
                              const char *model_path);
