/**
 * @file json_sourceparams.h
 *
 */
/* Copyright (C) 2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef JSON_SOURCEPARAMS_H_
#define JSON_SOURCEPARAMS_H_

#include "json/ltcparamsconst.h"
#include "configurationstore.h"
#include "json/json_params_base.h"

namespace json::ltc {
class LtcParams : public JsonParamsBase<LtcParams> {
   public:
    LtcParams();

    LtcParams(const LtcParams&) = delete;
    LtcParams& operator=(const LtcParams&) = delete;

    LtcParams(LtcParams&&) = delete;
    LtcParams& operator=(LtcParams&&) = delete;

    void Load() { JsonParamsBase::Load(LtcParamsConst::kFileName); }
    void Store(const char* buffer, uint32_t buffer_size);

   protected:
    void Dump();

   private:
    static void SetSource(const char* val, uint32_t len);

    static constexpr json::Key kLtcKeys[] = {
        MakeKey(SetSource, LtcParamsConst::kSource), //
    };

    inline static common::store::Ltc store_ltc;

    friend class JsonParamsBase<LtcParams>;
};
} // namespace json::ltc

#endif // INCLUDE_JSON_SOURCEPARAMS_H_
