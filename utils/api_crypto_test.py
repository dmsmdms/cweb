#!/bin/python

import requests, time

url = "http://localhost/api"
data_get_symbols = {
    "act": "get-symbols",
}
data_get_metrics = {
    "act": "get-metrics",
    "data": {
        "symbol": "btcusdt",
        "start": 1760775248,
        "end": int(time.time()),
        "interval": 1,
        "limit": 10 * 1000,
    },
}

res = requests.post(url, json=data_get_metrics)
print("Status code:", res.status_code)
print("Response body:", res.text)

