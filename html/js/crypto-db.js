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
        res.sym.sort();
        for (const sym of res.sym) {
            const option = document.createElement('option');
            option.value = sym;
            option.text = sym.toUpperCase();
            symbol.appendChild(option);
        }
    }
}

function crypto_form_init() {
    const form = document.getElementById('form');
    const start_date = document.getElementById('start-date');
    const end_date = document.getElementById('end-date');

    form.onsubmit = async function (event) {
        event.preventDefault();
        const data = new FormData(form);
        const req = {
            symbol: data.get('symbol'),
            start_date: time_get_unix(data.get('start-date')),
            end_date: time_get_unix(data.get('end-date')),
            interval: parseInt(data.get('interval')),
            limit: 10000,
        };
        const res = await api_request('get-metrics', req);
        if (res) {
            console.log('Response:', res);
        }
    };

    start_date.onchange = function () {
        form_validate(
            start_date,
            new Date(start_date.value) >= new Date(end_date.value),
            'Start date must be before end date.',
        );
    };

    end_date.onchange = function () {
        form_validate(
            end_date,
            new Date(end_date.value) <= new Date(start_date.value),
            'End date must be after start date.',
        );
    };
}

crypto_symbol_init();
crypto_form_init();
