# Web UI cleanup

Files are renamed to their deployed URL paths, without upload suffixes.

## Removed unused code
- Removed `loadField`, `saveField` and `display` from index.js: the supplied configuration page has no `display_name` or `utc_offset` controls. The `display()` call is replaced by `loadNetwork()`.
- Removed the unused configuration-page `loadModules()` wrapper; the supplied page does not call it.
- Removed the unnecessary status/index.js include from time/index.html.
- Removed the obsolete RTC comment.

## Correctness fix found during review
- rtc/index.js previously requested only `timedate`, so `rtc` was always undefined. It now requests both `timedate` and `rtc` concurrently. Verify the firmware's actual RTC JSON endpoint is `/json/rtc`.

## Intentionally retained
- config/*.js and the data-key form helpers: these are dynamically initialized by the firmware's directory/module loader and cannot safely be classified as unused from static HTML references.
- Status dynamic module rendering, generic JSON table helpers, and CSS for potential firmware/upload pages not supplied.
- API field names and endpoint contracts were not changed.
