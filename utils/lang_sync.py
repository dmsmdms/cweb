#!/bin/python

import requests, json, argparse

parser = argparse.ArgumentParser(description="Sync translations from localise.biz")
parser.add_argument("-p", "--path", required=True, type=str, help="Path to save the translations")
parser.add_argument("-t", "--token", required=True, type=str, help="API token for localise.biz")
args = parser.parse_args()

res = requests.get(f"https://localise.biz/api/export/all.json?key={args.token}")
if res.status_code == 200:
    data = json.loads(res.text)
    out = {}
    for lang_code, assets in data.items():
        short_code = lang_code.split("-")[0]
        for id, str in assets.items():
            if id not in out:
                out[id] = {}
            out[id] = {**out[id], **{short_code: str}}
    out = { "lang": out }
    with open(args.path, "w", encoding="utf-8") as f:
        f.write(json.dumps(out, ensure_ascii=False, indent=4))
    print("Translations updated successfully")
else:
    print(f"Failed to fetch translations. Status code: {res.status_code}")
