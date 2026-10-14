#include "layout.js.h"
#include "status_index.html.h"
#include "index.html.h"
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
#if !defined (CONFIG_HTTP_HTML_NO_RTC) && !defined (DISABLE_RTC)
#include "rtc_index.js.h"
#endif // !defined (CONFIG_HTTP_HTML_NO_RTC) && !defined (DISABLE_RTC)
#include "common.js.h"
#if defined (NODE_LTC_SMPTE)
#include "config_ltcdisplays.js.h"
#endif // (NODE_LTC_SMPTE)
