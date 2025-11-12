#include <core/ai/ai-gboost.h>
#include <core/base/log.h>
#include <xgboost/c_api.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

static BoosterHandle g_boost = NULL;

static void free_model(BoosterHandle boost, DMatrixHandle dtrain)
{
    XGBoosterFree(boost);
    XGDMatrixFree(dtrain);
}

static ai_gb_err_t set_param(BoosterHandle boost, const char *param, const char *value)
{
    if(XGBoosterSetParam(boost, param, value) < 0) {
        log_error("set param failed - %s", XGBGetLastError());
        return AI_GB_ERR_PARAM;
    }
    return AI_GB_ERR_OK;
}

ai_gb_err_t ai_gb_train_model(const float *train_data, const float *labels, uint32_t num_rows, uint32_t num_cols,
                              const char *model_path)
{
    DMatrixHandle dtrain = NULL;
    if(XGDMatrixCreateFromMat(train_data, num_rows, num_cols, 0, &dtrain) < 0) {
        log_error("create DMatrix failed - %s", XGBGetLastError());
        free_model(NULL, dtrain);
        return AI_GB_ERR_NO_MEM;
    }
    if(XGDMatrixSetFloatInfo(dtrain, "label", labels, num_rows) < 0) {
        log_error("set label failed - %s", XGBGetLastError());
        XGDMatrixFree(dtrain);
        return AI_GB_ERR_NO_MEM;
    }

    // Create booster //
    BoosterHandle boost = NULL;
    if(XGBoosterCreate(&dtrain, 1, &boost) < 0) {
        log_error("create booster failed - %s", XGBGetLastError());
        XGDMatrixFree(dtrain);
        return AI_GB_ERR_CREATE;
    }
    ai_gb_err_t res = AI_GB_ERR_OK;
    res |= set_param(boost, "objective", "binary:logistic");
    res |= set_param(boost, "eval_metric", "logloss");
    res |= set_param(boost, "seed", "42");
    res |= set_param(boost, "max_depth", "6");
    res |= set_param(boost, "eta", "0.08");
    res |= set_param(boost, "subsample", "0.8");
    res |= set_param(boost, "colsample_bytree", "0.8");
    res |= set_param(boost, "verbosity", "0");
    if(res != AI_GB_ERR_OK) {
        free_model(boost, dtrain);
        return res;
    }

    // Train for 150 iterations //
    for(uint32_t i = 0; i < 150; i++) {
        if(XGBoosterUpdateOneIter(boost, i, dtrain) < 0) {
            log_error("iteration %u failed - %s", i, XGBGetLastError());
            free_model(boost, dtrain);
            return AI_GB_ERR_TRAIN;
        }
        log_info("iteration %u", i);
    }

    // Save model //
    if(XGBoosterSaveModel(boost, model_path) < 0) {
        log_error("save model failed - %s", XGBGetLastError());
        free_model(boost, dtrain);
        return AI_GB_ERR_SAVE;
    }

    XGDMatrixFree(dtrain);
    g_boost = boost;

    return AI_GB_ERR_OK;
}

ai_gb_err_t ai_gb_predict(const float *data, float label, uint32_t num_cols)
{
    DMatrixHandle dtrain = NULL;
    if(XGDMatrixCreateFromMat(data, 1, num_cols, 0, &dtrain) < 0) {
        log_error("create DMatrix failed - %s", XGBGetLastError());
        return AI_GB_ERR_NO_MEM;
    }

    bst_ulong out_len = 0;
    const float *out_result = NULL;
    if(XGBoosterPredict(g_boost, dtrain, 0, 0, 0, &out_len, &out_result) < 0) {
        log_error("predict failed - %s", XGBGetLastError());
        XGDMatrixFree(dtrain);
        return AI_GB_ERR_TRAIN;
    }

    log_info("true label: %.3f, predicted: %.6f", label, out_result[0]);
    XGDMatrixFree(dtrain);
    return AI_GB_ERR_OK;
}
