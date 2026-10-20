async function loadNetwork() {
	const j = await getJSON('config/network');
	if (!j) return;

	const ids = ['hostname', 'ip_address', 'net_mask', 'default_gateway', 'ntp_server', 'secondary_ip', 'use_static_ip'];
	for (const id of ids) {
		const e = $(id);
		if (!e) continue;

		if (id === 'ntp_server' && !(id in j)) {
			e.value = 'Not supported.';
			e.disabled = true;
			continue;
		}
		if (id === 'ntp_server') e.disabled = false;
		setValue(id, j[id]);
	}
}

async function saveNetwork() {
	const required = ['hostname', 'ip_address', 'net_mask', 'default_gateway'];
	for (const id of required) {
		const e = $(id);
		if (!e.checkValidity()) {
			e.reportValidity();
			return;
		}
	}

	const data = {
		hostname: $('hostname').value,
		ip_address: $('ip_address').value,
		net_mask: $('net_mask').value,
		default_gateway: $('default_gateway').value,
		use_static_ip: $('use_static_ip').checked
	};

	const ntp = $('ntp_server').value.trim();
	if (ntp !== '' && !$('ntp_server').disabled) data.ntp_server = ntp;
	if (await postJSON('config/network', data)) loadNetwork();
}

function fillDataKeys(root, data) {
	if (!data) return;

	root = root || document;

	const elements = root.querySelectorAll("[data-key]");

	for (let i = 0; i < elements.length; i++) {
		const el = elements[i];
		const value = data[el.dataset.key];

		if (value === undefined || value === null) {
			continue;
		}

		if (el.type === "checkbox") {
			el.checked = value === true || value === 1 || value === "1";
		} else {
			el.value = value;
		}
	}
}

async function saveDataKeyForm(path, root, options = {}) {
    const values = {};
    const elements = root.querySelectorAll("[data-key]");

    for (let i = 0; i < elements.length; i++) {
        const el = elements[i];
        const key = el.dataset.key;

        if (!key || el.disabled) {
            continue;
        }

        if (el.type === "checkbox") {
            values[key] = el.checked;
        } else if (el.type === "number") {
            values[key] = Number(el.value);
        } else {
            values[key] = el.value;
        }
    }

    if (options.beforePost) {
        if (options.beforePost(values, root) === false) {
            return false;
        }
    }

    await postJSON(path, values);

    // Default reload
    if (options.reload !== false) {
        const json = await getJSON(path);
        if (json) {
            fillDataKeys(root, json);
        }
    }

    if (options.afterPost) {
        await options.afterPost(root, values);
    }

    return true;
}

// LTC configuration: labels follow ltc.h kInputPretty / kOutputPretty.
const ltcInputs = [
    ['ltc', 'LTC'], ['artnet', 'Art-Net'], ['midi', 'DIN MIDI'],
    ['tcnet', 'TCNet'], ['internal', 'Internal'], ['rtp-midi', 'Apple MIDI'],
    ['usb-midi', 'USB MIDI'], ['systime', 'System-Time'], ['etc', 'ETC']
];
const ltcOutputLabels = {
    'ltc': 'LTC', 'artnet': 'Art-Net', 'midi': 'DIN MIDI',
    'tcnet': 'TCNet', 'internal': 'Internal', 'rtp-midi': 'Apple MIDI',
    'usb-midi': 'USB MIDI', 'systime': 'System-Time', 'etc': 'ETC',
    'ntp-server': 'NTP Server', 'display-oled': 'Display OLED',
    'display-max7219': 'Display MAX7219', 'display-pixel': 'Display Pixel'
};
let ltcOutputKeys = [];

async function loadLtc() {
    const status = $('ltc_status');
    const json = await getJSON('config/ltc');
    if (!json) {
        status.textContent = 'Unable to load LTC configuration.';
        return;
    }
    const source = $('ltc_source');
    source.replaceChildren();
    for (const [value, label] of ltcInputs) {
        source.add(new Option(label, value));
    }
    if (ltcInputs.some(([value]) => value === json.source)) {
        source.value = json.source;
    } else {
        source.add(new Option(String(json.source ?? 'Unknown'), String(json.source ?? ''), true, true));
    }

    const tbody = $('ltc_outputs');
    tbody.replaceChildren();
    ltcOutputKeys = Object.keys(json).filter(key =>
        key.startsWith('output_') &&
        (json[key] === 'Enabled' || json[key] === 'Disabled'));
    for (const [index, key] of ltcOutputKeys.entries()) {
        const row = tbody.insertRow();
        row.insertCell().textContent = ltcOutputLabels[key.slice(7)] || key.slice(7);
        for (const value of ['Enabled', 'Disabled']) {
            const cell = row.insertCell();
            const radio = document.createElement('input');
            radio.type = 'radio';
            radio.name = 'ltc_output_' + index;
            radio.value = value;
            radio.checked = json[key] === value;
            radio.setAttribute('aria-label', (ltcOutputLabels[key.slice(7)] || key) + ': ' + value);
            cell.appendChild(radio);
        }
    }
    status.textContent = '';
}

async function saveLtc() {
    const button = $('ltc_save');
    const status = $('ltc_status');
    const data = { source: $('ltc_source').value };
    for (const [index, key] of ltcOutputKeys.entries()) {
        const selected = document.querySelector('input[name="ltc_output_' + index + '"]:checked');
        if (selected) data[key] = selected.value;
    }
    button.disabled = true;
    status.textContent = 'Saving…';
    try {
        if (await postJSON('config/ltc', data)) {
            await loadLtc();
            if (!status.textContent) status.textContent = 'Configuration saved and reloaded.';
        } else {
            status.textContent = 'Save failed.';
        }
    } finally {
        button.disabled = false;
    }
}
