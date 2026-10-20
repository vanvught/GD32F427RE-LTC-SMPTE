async function refreshRtc() {
  const [timedate, rtc] = await Promise.all([getJSON('timedate'), getJSON('rtc')]);
  if (timedate && timedate.date) {
    const d = new Date(timedate.date);
    setValue('nodeTime', Number.isNaN(d.getTime()) ? '' : formatDateTime(d));
  }
  if (!rtc) return;
  if (rtc.rtc) {
    const d = new Date(rtc.rtc);
    setValue('rtcTime', Number.isNaN(d.getTime()) ? '' : formatDateTime(d));
  }
  if (rtc.alarm) {
    const d = new Date(rtc.alarm);
    if (!Number.isNaN(d.getTime())) {
      const local = new Date(d.getTime() - d.getTimezoneOffset() * 60000);
      setValue('alarmInput', local.toISOString().slice(0, 19));
    }
  }
  $('chkbox').checked = rtc.enabled === '1' || rtc.enabled === 1 || rtc.enabled === true;
}

async function hcToSys() {
  if (await postJSON('rtc', { action: 'hctosys' })) await refreshRtc();
  else alert('Could not set Node time from RTC.');
}

async function sysToHc() {
  if (await postJSON('rtc', { action: 'systohc' })) await refreshRtc();
  else alert('Could not set RTC from Node time.');
}

async function setRtcAlarm() {
  const value = $('alarmInput').value;
  if (!value) { alert('Select an alarm date and time.'); return; }
  const date = new Date(value);
  if (Number.isNaN(date.getTime())) { alert('Invalid alarm date and time.'); return; }
  const data = { alarm: formatDateTime(date), enable: $('chkbox').checked ? '1' : '0' };
  if (await postJSON('rtc', data)) await refreshRtc();
  else alert('Could not update RTC alarm.');
}
