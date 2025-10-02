'use strict';

const METRIC_LIMIT = 10000;
const ROUND_DECIMALS = 2;
const PRICE_ARR_SIZE = 15;
const VOLUME_ARR_SIZE = 5;
const PAGE_ITEMS_MAX = 100;
const PAGE_STEP = 5;

let metrics = [];
let calc = null;

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

function time_get_str(unix) {
    const time = new Date(unix * 1000);
    const day = String(time.getDate()).padStart(2, '0');
    const month = String(time.getMonth() + 1).padStart(2, '0');
    const year = time.getFullYear();
    const hours = String(time.getHours()).padStart(2, '0');
    const minutes = String(time.getMinutes()).padStart(2, '0');
    const seconds = String(time.getSeconds()).padStart(2, '0');
    return `${day}-${month}-${year}\n${hours}:${minutes}:${seconds}`;
}

function form_validate(el, cond, text) {
    return el.setCustomValidity(cond ? text : '');
}

async function wasm_load(name) {
    let wasm = await WebAssembly.instantiateStreaming(fetch(name));
    const instance = wasm.instance;
    const memory = instance.exports.memory;
    wasm.mem_buf = memory.buffer;
    wasm.mem_off = 0;
    wasm.func = instance.exports;
    return wasm;
}

class CryptoCalc {
    constructor(wasm) {
        this.wasm = wasm;
        this.last_volume_surge = NaN;

        this.prices = new Float32Array(wasm.mem_buf, wasm.mem_off, PRICE_ARR_SIZE);
        this.prices_ptr = wasm.mem_off;
        this.prices_off = -1;
        this.prices_cnt = 0;
        wasm.mem_off += PRICE_ARR_SIZE * 4;

        this.volumes = new Float32Array(wasm.mem_buf, wasm.mem_off, VOLUME_ARR_SIZE);
        this.volumes_ptr = wasm.mem_off;
        this.volumes_off = -1;
        this.volumes_cnt = 0;
        wasm.mem_off += VOLUME_ARR_SIZE * 4;
    }

    static async init() {
        const wasm = await wasm_load('js/calc-crypto.wasm');
        return new CryptoCalc(wasm);
    }

    clear() {
        this.prices_off = -1;
        this.prices_cnt = 0;
        this.volumes_off = -1;
        this.volumes_cnt = 0;
        this.last_volume_surge = NaN;
    }

    push_price(close_price) {
        this.prices_off = (this.prices_off + 1) % PRICE_ARR_SIZE;
        this.prices[this.prices_off] = close_price;
        if (this.prices_cnt < PRICE_ARR_SIZE) {
            this.prices_cnt++;
        }
    }

    push_volume(volume) {
        this.volumes_off = (this.volumes_off + 1) % VOLUME_ARR_SIZE;
        this.volumes[this.volumes_off] = volume;
        if (this.volumes_cnt < VOLUME_ARR_SIZE) {
            this.volumes_cnt++;
        }
    }

    get_rsi() {
        return this.wasm.func.calc_crypto_rsi(this.prices_ptr, this.prices_cnt, this.prices_off);
    }

    get_tail() {
        return this.wasm.func.calc_crypto_tail(this.prices_ptr, this.prices_cnt, this.prices_off);
    }

    get_slope() {
        return this.wasm.func.calc_crypto_slope(this.prices_ptr, this.prices_cnt, this.prices_off);
    }

    get_volume_surge(volume) {
        return this.wasm.func.calc_crypto_volume_surge(
            this.volumes_ptr,
            this.volumes_cnt,
            this.volumes_off,
            volume,
        );
    }

    get_volume_accel(volume_surge) {
        if (isNaN(volume_surge)) {
            return NaN;
        }
        if (isNaN(this.last_volume_surge)) {
            this.last_volume_surge = volume_surge;
            return NaN;
        }
        const accel = volume_surge - this.last_volume_surge;
        this.last_volume_surge = volume_surge;
        return accel;
    }
}

async function crypto_symbol_init() {
    const symbol = document.getElementById('symbol');
    symbol.onchange = function () {
        form_validate(symbol, symbol.value == 'none', 'Please select a symbol.');
    };

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
        symbol.onchange();
    }
}

function crypto_table_page_set(idx) {
    const frag = document.createDocumentFragment();
    for (const metric of metrics[idx]) {
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

        time.textContent = time_get_str(metric.ts);
        rsi.textContent = isNaN(metric.rsi_val) ? '-' : metric.rsi_val.toFixed(ROUND_DECIMALS);
        tail.textContent = isNaN(metric.tail_val) ? '-' : metric.tail_val.toFixed(6);
        slope.textContent = isNaN(metric.slope_val) ? '-' : metric.slope_val.toFixed(6);
        whales.textContent = metric.w;
        liquidity.textContent = (metric.la + metric.lb).toFixed(ROUND_DECIMALS);
        liq_bid.textContent = metric.lb.toFixed(ROUND_DECIMALS);
        liq_ask.textContent = metric.la.toFixed(ROUND_DECIMALS);
        liq_delta.textContent = metric.liq_delta_val.toFixed(ROUND_DECIMALS);
        bid_ask_ratio.textContent = (metric.lb / metric.la).toFixed(ROUND_DECIMALS);
        volume.textContent = metric.v.toFixed(ROUND_DECIMALS);
        volume_surge.textContent = isNaN(metric.volume_surge_val)
            ? '-'
            : metric.volume_surge_val.toFixed(ROUND_DECIMALS);
        volume_accel.textContent = isNaN(metric.volume_accel_val)
            ? '-'
            : metric.volume_accel_val.toFixed(ROUND_DECIMALS);
        close_price.textContent = metric.c.toFixed(ROUND_DECIMALS);

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

function crypto_table_page_update(cur_idx, page_count) {
    const frag = document.createDocumentFragment();
    let li_arr = [],
        need_split = false;
    for (let i = 0; i < page_count; i++) {
        if (
            i < PAGE_STEP ||
            i > page_count - PAGE_STEP - 1 ||
            Math.abs(i - cur_idx) < PAGE_STEP / 2
        ) {
            const div = document.createElement('div');
            div.className = 'page-link';
            div.textContent = i + 1;

            const li = document.createElement('li');
            li.className = 'page-item cursor-pointer';
            li.onclick = function () {
                if (i != cur_idx) {
                    crypto_table_page_set(i);
                    li_arr[cur_idx].classList.remove('active');
                    crypto_table_page_update(i, page_count);
                }
            };
            li_arr[i] = li;
            li.appendChild(div);
            frag.appendChild(li);
            need_split = true;
        } else if (need_split) {
            const div = document.createElement('div');
            div.className = 'page-link';
            div.textContent = '...';
            const li = document.createElement('li');
            li.className = 'page-item disabled';
            li.appendChild(div);
            frag.appendChild(li);
            need_split = false;
        }
    }
    crypto_table_page_set(cur_idx);
    li_arr[cur_idx].classList.add('active');
    const nav = document.getElementById('nav');
    nav.replaceChildren(frag);
    nav.hidden = false;
}

async function crypto_form_init() {
    const form = document.getElementById('form');
    const start = document.getElementById('start');
    const end = document.getElementById('end');
    const interval = document.getElementById('interval');
    const tbody = document.getElementById('tbody');
    let start_val = sessionStorage.getItem('form_start');
    let end_val = sessionStorage.getItem('form_end');
    let interval_val = sessionStorage.getItem('form_interval');
    calc = await CryptoCalc.init();

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
        calc.clear();
        metrics = [];

        const req = {
            symbol: symbol_val,
            start: time_get_unix(start_val),
            end: time_get_unix(end_val),
            interval: parseInt(interval_val),
            limit: METRIC_LIMIT,
        };
        const res = await api_request('get-metrics', req);
        if (res) {
            let mi = 0,
                mj = 0;
            for (let metric of res.metrics) {
                if (!metrics[mi]) {
                    metrics[mi] = [];
                }

                calc.push_price(metric.c);
                calc.push_volume(metric.v);
                metric.rsi_val = calc.get_rsi();
                metric.tail_val = calc.get_tail();
                metric.slope_val = calc.get_slope();
                metric.volume_surge_val = calc.get_volume_surge(metric.v);
                metric.volume_accel_val = calc.get_volume_accel(metric.volume_surge_val);
                metric.liq_delta_val = metric.lb - metric.la / (metric.la + metric.lb);
                metrics[mi][mj++] = metric;

                if (mj >= PAGE_ITEMS_MAX) {
                    mj = 0;
                    mi++;
                }
            }
            crypto_table_page_update(0, mi);
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
