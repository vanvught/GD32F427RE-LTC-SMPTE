#ifndef CONTENT_H_
#define CONTENT_H_

#include <cstdint>
#include "httpd/httpd.h"
#include "storage_index.js.h"
#include "layout.js.h"
#include "status_index.html.h"
#include "index.html.h"
#if defined (CONFIG_HTTPD_ENABLE_UPLOAD)
#include "upload_index.js.h"
#endif // (CONFIG_HTTPD_ENABLE_UPLOAD)
#if defined (NODE_LTC_SMPTE)
#include "config_etc.js.h"
#endif // (NODE_LTC_SMPTE)
#include "styles.css.h"
#include "status_index.js.h"
#if defined (NODE_LTC_SMPTE)
#include "config_gps.js.h"
#endif // (NODE_LTC_SMPTE)
#include "index.js.h"
#if defined (NODE_LTC_SMPTE)
#include "config_ltc.js.h"
#endif // (NODE_LTC_SMPTE)
#if defined (NODE_LTC_SMPTE)
#include "config_tcnet.js.h"
#endif // (NODE_LTC_SMPTE)
#if !defined (CONFIG_HTTP_HTML_NO_RTC) && !defined (DISABLE_RTC)
#include "rtc_index.html.h"
#endif // !defined (CONFIG_HTTP_HTML_NO_RTC) && !defined (DISABLE_RTC)
#if !defined (CONFIG_HTTP_HTML_NO_TIME)
#include "time_index.html.h"
#endif // !defined (CONFIG_HTTP_HTML_NO_TIME)
#include "date.js.h"
#include "common.js.h"
#if defined (NODE_LTC_SMPTE)
#include "config_ltcdisplays.js.h"
#endif // (NODE_LTC_SMPTE)
#include "storage_index.html.h"
#if defined (CONFIG_HTTPD_ENABLE_UPLOAD)
#include "upload_index.html.h"
#endif // (CONFIG_HTTPD_ENABLE_UPLOAD)

struct FilesContent {
	uint32_t hash;
	const char *file_name;
	const uint8_t *content;
	uint32_t content_length;
	http::ContentTypes content_type;
	bool gzip;
};

inline constexpr struct FilesContent kHttpContent[] = {
	{ 3991234706,"storage_index.js", storage_index_js_gz, 899, static_cast<http::ContentTypes>(2), true },
	{ 2226674728,"layout.js", layout_js_gz, 570, static_cast<http::ContentTypes>(2), true },
	{ 4041167307,"status_index.html", status_index_html_gz, 604, static_cast<http::ContentTypes>(0), true },
	{ 4024653090,"index.html", index_html_gz, 735, static_cast<http::ContentTypes>(0), true },
#if defined (CONFIG_HTTPD_ENABLE_UPLOAD)
	{ 1750204712,"upload_index.js", upload_index_js_gz, 1631, static_cast<http::ContentTypes>(2), true },
#endif // (CONFIG_HTTPD_ENABLE_UPLOAD)
#if defined (NODE_LTC_SMPTE)
	{ 109007079,"config_etc.js", config_etc_js_gz, 593, static_cast<http::ContentTypes>(2), true },
#endif // (NODE_LTC_SMPTE)
	{ 2557875310,"styles.css", styles_css_gz, 969, static_cast<http::ContentTypes>(1), true },
	{ 4156522053,"status_index.js", status_index_js_gz, 273, static_cast<http::ContentTypes>(2), true },
#if defined (NODE_LTC_SMPTE)
	{ 610379173,"config_gps.js", config_gps_js_gz, 559, static_cast<http::ContentTypes>(2), true },
#endif // (NODE_LTC_SMPTE)
	{ 247271700,"index.js", index_js_gz, 1045, static_cast<http::ContentTypes>(2), true },
#if defined (NODE_LTC_SMPTE)
	{ 1947358610,"config_ltc.js", config_ltc_js_gz, 1868, static_cast<http::ContentTypes>(2), true },
#endif // (NODE_LTC_SMPTE)
#if defined (NODE_LTC_SMPTE)
	{ 3844231377,"config_tcnet.js", config_tcnet_js_gz, 618, static_cast<http::ContentTypes>(2), true },
#endif // (NODE_LTC_SMPTE)
#if !defined (CONFIG_HTTP_HTML_NO_RTC) && !defined (DISABLE_RTC)
	{ 1233896314,"rtc_index.html", rtc_index_html_gz, 239, static_cast<http::ContentTypes>(0), true },
#endif // !defined (CONFIG_HTTP_HTML_NO_RTC) && !defined (DISABLE_RTC)
#if !defined (CONFIG_HTTP_HTML_NO_TIME)
	{ 2057353734,"time_index.html", time_index_html_gz, 520, static_cast<http::ContentTypes>(0), true },
#endif // !defined (CONFIG_HTTP_HTML_NO_TIME)
	{ 1602327546,"date.js", date_js_gz, 325, static_cast<http::ContentTypes>(2), true },
	{ 3116920429,"common.js", common_js_gz, 1448, static_cast<http::ContentTypes>(2), true },
#if defined (NODE_LTC_SMPTE)
	{ 1194496967,"config_ltcdisplays.js", config_ltcdisplays_js_gz, 565, static_cast<http::ContentTypes>(2), true },
#endif // (NODE_LTC_SMPTE)
	{ 2148610488,"storage_index.html", storage_index_html_gz, 513, static_cast<http::ContentTypes>(0), true },
#if defined (CONFIG_HTTPD_ENABLE_UPLOAD)
	{ 137651886,"upload_index.html", upload_index_html_gz, 563, static_cast<http::ContentTypes>(0), true },
#endif // (CONFIG_HTTPD_ENABLE_UPLOAD)
};


#endif /* CONTENT_H_ */
