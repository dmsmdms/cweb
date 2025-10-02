#include <global.h>

void api_jobly_file_cb(const http_srv_req_t *req, http_srv_resp_t *resp)
{
    app_t *app = req->app;
    const config_t *cfg = &app->cfg;
    if(html_gen(app, resp, cfg->html_jobly_path, req->path, NULL, 0)) {
        resp->code = HTTP_RESP_CODE_200_OK;
    }
}
