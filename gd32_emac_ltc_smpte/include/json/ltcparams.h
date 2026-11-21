/**
 * @file ltcparams.h
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

#include <array>
#include <utility>

#ifndef JSON_SOURCEPARAMS_H_
#define JSON_SOURCEPARAMS_H_

#include "json/ltcparamsconst.h"
#include "configurationstore.h"
#include "json/json_params_base.h"

namespace json::ltc {
	template <std::size_t... I>
	consteval auto MakeLtcKeys(
	    json::SimpleSetter set_source,
	    json::KeyedSetter set_output,
	    std::index_sequence<I...> /*unused*/) {

	    return std::array{
	        MakeKey(set_source, LtcParamsConst::kSource),
	        MakeKey(set_output, LtcParamsConst::kOutputs[I])...
	    };
	}

class LtcParams : public JsonParamsBase<LtcParams> {
   public:
    LtcParams();

    LtcParams(const LtcParams&) = delete;
    LtcParams& operator=(const LtcParams&) = delete;

    LtcParams(LtcParams&&) = delete;
    LtcParams& operator=(LtcParams&&) = delete;

    void Load() { JsonParamsBase::Load(LtcParamsConst::kFileName); }
    void Store(std::string_view buffer);
    void Set();

   protected:
    void Dump();

   private:
    static void SetSource(std::string_view value);
    static void SetOutput(std::string_view key, std::string_view value);

	static constexpr auto kLtcKeys = MakeLtcKeys(
	SetSource,
	SetOutput,
	std::make_index_sequence<std::size(LtcParamsConst::kOutputs)>{});

    inline static common::store::Ltc store_ltc;

    friend class JsonParamsBase<LtcParams>;
};
} // namespace json::ltc

#endif // JSON_SOURCEPARAMS_H_
