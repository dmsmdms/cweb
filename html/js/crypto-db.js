'use strict';

async function api_request(act, data = null) {
    let req_data = {
        act: act,
    };
    if (data) {
        req_data.data = data;
    }
    const info = {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify(req_data),
    };
    const res = await window.fetch('/api', info);
    return res.ok ? await res.json() : null;
}

function time_get_unix(time) {
    return Math.floor(new Date(time).getTime() / 1000);
}

function form_validate(el, cond, text) {
    return el.setCustomValidity(cond ? text : '');
}

async function crypto_symbol_init() {
    const symbol = document.getElementById('symbol');
    symbol.onchange = function () {
        form_validate(symbol, symbol.value == 'none', 'Please select a symbol.');
    };
    symbol.onchange();

    const res = await api_request('get-symbols');
    if (res) {
        const frag = document.createDocumentFragment();
        res.sym.sort();
        for (const sym of res.sym) {
            const option = document.createElement('option');
            option.value = sym;
            option.text = sym.toUpperCase();
            frag.appendChild(option);
        }
        symbol.appendChild(frag);

        const symbol_val = sessionStorage.getItem('form_symbol');
        if (symbol_val) {
            symbol.value = symbol_val;
        }
    }
}

function crypto_form_init() {
    const form = document.getElementById('form');
    const start = document.getElementById('start');
    const end = document.getElementById('end');
    const interval = document.getElementById('interval');
    const tbody = document.getElementById('tbody');
    let start_val = sessionStorage.getItem('form_start');
    let end_val = sessionStorage.getItem('form_end');
    let interval_val = sessionStorage.getItem('form_interval');

    if (start_val) {
        start.value = start_val;
    }
    if (end_val) {
        end.value = end_val;
    }
    if (interval_val) {
        interval.value = interval_val;
    }

    form.onsubmit = async function (event) {
        event.preventDefault();

        const data = new FormData(form);
        const symbol_val = data.get('symbol');
        start_val = data.get('start');
        end_val = data.get('end');
        interval_val = data.get('interval');
        sessionStorage.setItem('form_symbol', symbol_val);
        sessionStorage.setItem('form_start', start_val);
        sessionStorage.setItem('form_end', end_val);
        sessionStorage.setItem('form_interval', interval_val);

        const req = {
            symbol: symbol_val,
            start: time_get_unix(start_val),
            end: time_get_unix(end_val),
            interval: parseInt(interval_val),
            limit: 10000,
        };
        const res = await api_request('get-metrics', req);
        if (res) {
            const frag = document.createDocumentFragment();
            for (const metric of res.metrics) {
                const time = document.createElement('td');
                const rsi = document.createElement('td');
                const tail = document.createElement('td');
                const slope = document.createElement('td');
                const whales = document.createElement('td');
                const liquidity = document.createElement('td');
                const liq_bid = document.createElement('td');
                const liq_ask = document.createElement('td');
                const liq_delta = document.createElement('td');
                const bid_ask_ratio = document.createElement('td');
                const volume = document.createElement('td');
                const volume_surge = document.createElement('td');
                const volume_accel = document.createElement('td');
                const close_price = document.createElement('td');
                time.textContent = new Date(metric.ts * 1000).toISOString();
                rsi.textContent = 0;
                tail.textContent = 0;
                slope.textContent = 0;
                whales.textContent = metric.w;
                liquidity.textContent = metric.la + metric.lb;
                liq_bid.textContent = metric.lb;
                liq_ask.textContent = metric.la;
                liq_delta.textContent = metric.lb - metric.la / (metric.la + metric.lb);
                bid_ask_ratio.textContent = metric.lb / metric.la;
                volume.textContent = metric.v;
                volume_surge.textContent = 0;
                volume_accel.textContent = 0;
                close_price.textContent = metric.c;

                const tr = document.createElement('tr');
                tr.appendChild(time);
                tr.appendChild(rsi);
                tr.appendChild(tail);
                tr.appendChild(slope);
                tr.appendChild(whales);
                tr.appendChild(liquidity);
                tr.appendChild(liq_bid);
                tr.appendChild(liq_ask);
                tr.appendChild(liq_delta);
                tr.appendChild(bid_ask_ratio);
                tr.appendChild(volume);
                tr.appendChild(volume_surge);
                tr.appendChild(volume_accel);
                tr.appendChild(close_price);
                frag.appendChild(tr);
            }
            tbody.replaceChildren(frag);
        }
    };

    start.onchange = function () {
        form_validate(
            start,
            new Date(start.value) >= new Date(end.value),
            'Start date must be before end date.',
        );
    };

    end.onchange = function () {
        form_validate(
            end,
            new Date(end.value) <= new Date(start.value),
            'End date must be after start date.',
        );
    };
}

crypto_symbol_init();
crypto_form_init();
