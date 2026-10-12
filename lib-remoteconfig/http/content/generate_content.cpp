/**
 * @file generate_content.cpp
 *
 */
/* Copyright (C) 2021-2026 by Arjan van Vught mailto:info@gd32-dmx.org
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#undef NDEBUG
#if !defined(USE_GZIP)
#define USE_GZIP
#endif

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <cassert>

#include "http/http.h"
#include "common/utils/utils_hash.h"

struct SupportedExtension {
    const char* extension;
    http::ContentTypes content_type;
};

static constexpr SupportedExtension kSupportedExtensions[] = {
    {.extension = "html", .content_type = http::ContentTypes::kTextHtml},
    {.extension = "css", .content_type = http::ContentTypes::kTextCss},
    {.extension = "js", .content_type = http::ContentTypes::kTextJs},
    {.extension = "json", .content_type = http::ContentTypes::kApplicationJson},
};

static constexpr char kContentHeader[] =
    "\n"
    "struct FilesContent {\n"
    "\tuint32_t hash;\n"
    "\tconst char *file_name;\n"
    "\tconst uint8_t *content;\n"
    "\tuint32_t content_length;\n"
    "\thttp::ContentTypes content_type;\n"
    "\tbool gzip;\n"
    "};\n\n"
    "inline constexpr struct FilesContent kHttpContent[] = {\n";

// Node Type
static constexpr char kHaveNodeLtcSmpteBegin[] = "#if defined (NODE_LTC_SMPTE)\n";
static constexpr char kHaveNodeltcSmpteEnd[] = "#endif // (NODE_LTC_SMPTE)\n";

static constexpr char kHaveTimeBegin[] = "#if !defined (CONFIG_HTTP_HTML_NO_TIME)\n";
static constexpr char kHaveTimeEnd[] = "#endif // !defined (CONFIG_HTTP_HTML_NO_TIME)\n";

static constexpr char kHaveRtcBegin[] = "#if !defined (CONFIG_HTTP_HTML_NO_RTC) && !defined (DISABLE_RTC)\n";
static constexpr char kHaveRtcEnd[] = "#endif // !defined (CONFIG_HTTP_HTML_NO_RTC) && !defined (DISABLE_RTC)\n";

static constexpr char kHaveUploadBegin[] = "#if defined (CONFIG_HTTPD_ENABLE_UPLOAD)\n";
static constexpr char kHaveUploadEnd[] = "#endif // (CONFIG_HTTPD_ENABLE_UPLOAD)\n";

struct FeatureGuard {
    const char* match;
    const char* begin;
    const char* end;
};

static constexpr FeatureGuard kFeatureGuards[] = {
    {.match = "ltc.js", .begin = kHaveNodeLtcSmpteBegin, .end = kHaveNodeltcSmpteEnd},
    {.match = "ltcdisplays.js", .begin = kHaveNodeLtcSmpteBegin, .end = kHaveNodeltcSmpteEnd},
    {.match = "tcnet", .begin = kHaveNodeLtcSmpteBegin, .end = kHaveNodeltcSmpteEnd},
    {.match = "etc", .begin = kHaveNodeLtcSmpteBegin, .end = kHaveNodeltcSmpteEnd},
    {.match = "gps", .begin = kHaveNodeLtcSmpteBegin, .end = kHaveNodeltcSmpteEnd},
    {.match = "time", .begin = kHaveTimeBegin, .end = kHaveTimeEnd},
    {.match = "rtc", .begin = kHaveRtcBegin, .end = kHaveRtcEnd},
    {.match = "upload", .begin = kHaveUploadBegin, .end = kHaveUploadEnd},
};

static FILE* file_content;
static FILE* file_includes;

static http::ContentTypes GetContentType(const char* file_name) {
    assert(file_name != nullptr);

    const auto* dot = strrchr(file_name, '.');

    if ((dot == nullptr) || (*(dot + 1) == '\0')) {
        return http::ContentTypes::kNotDefined;
    }

    const auto* extension = dot + 1;

    for (const auto& entry : kSupportedExtensions) {
        if (strcmp(extension, entry.extension) == 0) {
            return entry.content_type;
        }
    }

    return http::ContentTypes::kNotDefined;
}

static bool HasMatchingFeature(const char* file_name, const char* match) {
    assert(file_name != nullptr);
    assert(match != nullptr);

    return strstr(file_name, match) != nullptr;
}

static void WriteFeatureGuardsBegin(FILE* out, const char* file_name) {
    assert(out != nullptr);
    assert(file_name != nullptr);

    for (const auto& guard : kFeatureGuards) {
        if (HasMatchingFeature(file_name, guard.match)) {
            fwrite(guard.begin, sizeof(char), strlen(guard.begin), out);
        }
    }
}

static void WriteFeatureGuardsEnd(FILE* out, const char* file_name) {
    assert(out != nullptr);
    assert(file_name != nullptr);

    for (const auto& guard : kFeatureGuards) {
        if (HasMatchingFeature(file_name, guard.match)) {
            fwrite(guard.end, sizeof(char), strlen(guard.end), out);
        }
    }
}

static void MakeConstantName(const char* file_name, char* constant_name, size_t length) {
    assert(file_name != nullptr);
    assert(constant_name != nullptr);
    assert(length > strlen(file_name));

    strncpy(constant_name, file_name, length);
    constant_name[length - 1] = '\0';

    auto* p = strchr(constant_name, '.');

    if (p != nullptr) {
        *p = '_';
    }
}

static void AppendFile(FILE* out, const char* file_name) {
    assert(out != nullptr);
    assert(file_name != nullptr);

    auto* in = fopen(file_name, "r");
    assert(in != nullptr);

    char buffer[256];
    size_t read;

    while ((read = fread(buffer, sizeof(char), sizeof(buffer), in)) > 0) {
        fwrite(buffer, sizeof(char), read, out);
    }

    fclose(in);
}

static void BuildFinalContentHeader() {
    auto* out = fopen("tmp.h", "w");
    assert(out != nullptr);

    fwrite("#ifndef CONTENT_H_\n", sizeof(char), 19, out);
    fwrite("#define CONTENT_H_\n\n", sizeof(char), 20, out);
    fwrite("#include <cstdint>\n", sizeof(char), 19, out);
    fwrite("#include \"httpd/httpd.h\"\n", sizeof(char), 25, out);

    AppendFile(out, "includes.h");
    AppendFile(out, "content.h");

    fwrite("\n\n#endif /* CONTENT_H_ */\n", sizeof(char), 26, out);

    fclose(out);

    const auto kI = remove("content.h");
    assert(kI == 0);

    const auto kJ = rename("tmp.h", "content.h");
    assert(kJ == 0);
}

static int ConvertToHCompressed(const char* dir_name, const char* file_name) {
    assert(dir_name != nullptr);
    assert(file_name != nullptr);

    printf("File to convert: %s/%s, ", dir_name, file_name);

    char path[128];
    auto n = snprintf(path, sizeof(path), "%s/%s", dir_name, file_name);
    assert(n > 0);
    assert(n < static_cast<int>(sizeof(path)));

    const auto kFileNameLength = strlen(file_name);

    char file_name_out[128];
    n = snprintf(file_name_out, sizeof(file_name_out), "%s.h", file_name);
    assert(n > 0);
    assert(n < static_cast<int>(sizeof(file_name_out)));

    printf("Header file: \"%s\", ", file_name_out);

    auto* file_out = fopen(file_name_out, "w");
    if (file_out == nullptr) {
        perror(file_name_out);
        return -1;
    }

    char constant_name[128];
    assert((kFileNameLength + 4) < sizeof(constant_name));

    MakeConstantName(file_name, constant_name, sizeof(constant_name));

    char constant_name_gz[128];
    n = snprintf(constant_name_gz, sizeof(constant_name_gz), "%s_gz", constant_name);
    assert(n > 0);
    assert(n < static_cast<int>(sizeof(constant_name_gz)));

    printf("Constant name: %s, ", constant_name_gz);

    WriteFeatureGuardsBegin(file_includes, file_name_out);

    char buffer[128];
    n = snprintf(buffer, sizeof(buffer), "#include \"%s\"\n", file_name_out);
    assert(n > 0);
    assert(n < static_cast<int>(sizeof(buffer)));

    fwrite(buffer, sizeof(char), n, file_includes);

    WriteFeatureGuardsEnd(file_includes, file_name_out);

    fprintf(file_out, "#pragma once\n\n");
    fprintf(file_out, "#include <cstdint>\n\n");

    fprintf(file_out, "static constexpr uint8_t %s[] = {\n", constant_name_gz);

    char command[256];
    n = snprintf(command, sizeof(command), "gzip -n -9 -c \"%s\"", path);
    assert(n > 0);
    assert(n < static_cast<int>(sizeof(command)));

    auto* gzip_pipe = popen(command, "r");
    if (gzip_pipe == nullptr) {
        fclose(file_out);
        perror("gzip");
        return -1;
    }

    unsigned offset = 0;
    int c;

    while ((c = fgetc(gzip_pipe)) != EOF) {
        n = snprintf(buffer, sizeof(buffer), "0x%02X,%c", static_cast<unsigned>(static_cast<uint8_t>(c)), (++offset % 16U == 0U) ? '\n' : ' ');
        assert(n > 0);
        assert(n < static_cast<int>(sizeof(buffer)));

        fwrite(buffer, sizeof(char), n, file_out);
    }

    const auto kGzipStatus = pclose(gzip_pipe);

    if (kGzipStatus != 0) {
        fclose(file_out);
        fprintf(stderr, "gzip failed for %s\n", path);
        return -1;
    }

    if ((offset % 16U) != 0U) {
        fwrite("\n", sizeof(char), 1, file_out);
    }

    fprintf(file_out, "};\n\n");
    fprintf(file_out, "static constexpr uint32_t %s_len = sizeof(%s);\n", constant_name_gz, constant_name_gz);

    fclose(file_out);

    fwrite(constant_name_gz, sizeof(char), strlen(constant_name_gz), file_content);

    printf("Compressed size: %u\n", offset);

    return static_cast<int>(offset);
}

static uint32_t CreateHash(const char* filename) {
    char hash_name[128];
    const auto kLength = strlen(filename);
    assert(kLength + 1 <= sizeof(hash_name));
    strncpy(hash_name, filename, sizeof(hash_name));
    hash_name[kLength] = '\0';

    auto* p = strchr(hash_name, '_');

    if (p != nullptr) {
        *p = '/';
    }

    printf("Hash name: %s ", hash_name);
    return Fnv1a32Runtime(hash_name, kLength);
}

int main(int argc, char* argv[]) { // NOLINT
    puts("Using gzip.");
    file_includes = fopen("includes.h", "w");
    assert(file_includes != nullptr);

    file_content = fopen("content.h", "w");
    assert(file_content != nullptr);

    fwrite(kContentHeader, sizeof(char), strlen(kContentHeader), file_content);

    char dir_name[32];
    strcpy(dir_name, "light");

    if (argc == 2) {
        strncpy(dir_name, argv[1], sizeof dir_name);
        dir_name[(sizeof dir_name) - 1] = '\0';
    }

    auto* dir = opendir(dir_name);

    if (dir != nullptr) {
        struct dirent* dir_entry;

        while ((dir_entry = readdir(dir)) != nullptr) {
            if (dir_entry->d_name[0] == '.') {
                continue;
            }

            const auto kContentType = GetContentType(dir_entry->d_name);
            const auto kIsSupported = (kContentType != http::ContentTypes::kNotDefined);

            printf("%s -> %c\n", dir_entry->d_name, kIsSupported ? 'Y' : 'N');

            if (!kIsSupported) {
                continue;
            }

            WriteFeatureGuardsBegin(file_content, dir_entry->d_name);

            char buffer[256];
            auto i = snprintf(buffer, sizeof(buffer), "\t{ %u,\"%s\", ", CreateHash(dir_entry->d_name), dir_entry->d_name);
            assert(i > 0);
            assert(i < static_cast<int>(sizeof(buffer)));

            fwrite(buffer, sizeof(char), i, file_content);

            auto use_gzip = false;
            use_gzip = true;
            const auto kContentLength = ConvertToHCompressed(dir_name, dir_entry->d_name);

            i = snprintf(buffer, sizeof(buffer), ", %d, static_cast<http::ContentTypes>(%d), %s },\n", kContentLength, static_cast<int>(kContentType), use_gzip ? "true" : "false");
            assert(i > 0);
            assert(i < static_cast<int>(sizeof(buffer)));

            fwrite(buffer, sizeof(char), i, file_content);

            WriteFeatureGuardsEnd(file_content, dir_entry->d_name);
        }

        closedir(dir);
    }

    fclose(file_includes);

    fwrite("};\n", sizeof(char), 3, file_content);
    fclose(file_content);

    BuildFinalContentHeader();

    return EXIT_SUCCESS;
}