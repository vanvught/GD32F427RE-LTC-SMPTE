#ifndef CONTENT_H_
#define CONTENT_H_

#include <cstdint>
#include "httpd/httpd.h"
#include "layout.js.h"
#include "status_index.html.h"
#include "index.html.h"
#include "config_etc.js.h"
#include "styles.css.h"
#include "status_index.js.h"
#include "config_gps.js.h"
#include "index.js.h"
#include "config_ltc.js.h"
#include "config_tcnet.js.h"
#include "rtc_index.html.h"
#include "time_index.html.h"
#include "date.js.h"
#include "rtc_index.js.h"
#include "common.js.h"
#include "config_ltcdisplays.js.h"

struct FilesContent {
	uint32_t hash;
	const char *file_name;
	const uint8_t *content;
	uint32_t content_length;
	http::ContentTypes content_type;
	bool gzip;
};

inline constexpr struct FilesContent kHttpContent[] = {
	{ 2226674728,"layout.js", layout_js_gz, 570, static_cast<http::ContentTypes>(2), true },
	{ 4041167307,"status_index.html", status_index_html_gz, 604, static_cast<http::ContentTypes>(0), true },
	{ 4024653090,"index.html", index_html_gz, 710, static_cast<http::ContentTypes>(0), true },
	{ 109007079,"config_etc.js", config_etc_js_gz, 593, static_cast<http::ContentTypes>(2), true },
	{ 2557875310,"styles.css", styles_css_gz, 1089, static_cast<http::ContentTypes>(1), true },
	{ 4156522053,"status_index.js", status_index_js_gz, 273, static_cast<http::ContentTypes>(2), true },
	{ 610379173,"config_gps.js", config_gps_js_gz, 559, static_cast<http::ContentTypes>(2), true },
	{ 247271700,"index.js", index_js_gz, 1848, static_cast<http::ContentTypes>(2), true },
	{ 1947358610,"config_ltc.js", config_ltc_js_gz, 1868, static_cast<http::ContentTypes>(2), true },
	{ 3844231377,"config_tcnet.js", config_tcnet_js_gz, 618, static_cast<http::ContentTypes>(2), true },
	{ 1233896314,"rtc_index.html", rtc_index_html_gz, 529, static_cast<http::ContentTypes>(0), true },
	{ 2057353734,"time_index.html", time_index_html_gz, 509, static_cast<http::ContentTypes>(0), true },
	{ 1602327546,"date.js", date_js_gz, 325, static_cast<http::ContentTypes>(2), true },
	{ 3121716012,"rtc_index.js", rtc_index_js_gz, 581, static_cast<http::ContentTypes>(2), true },
	{ 3116920429,"common.js", common_js_gz, 1443, static_cast<http::ContentTypes>(2), true },
	{ 1194496967,"config_ltcdisplays.js", config_ltcdisplays_js_gz, 565, static_cast<http::ContentTypes>(2), true },
};


#endif /* CONTENT_H_ */
