// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFAPI_PARSER_FPDF_PARSER_DECODE_H_
#define CORE_FPDFAPI_PARSER_FPDF_PARSER_DECODE_H_

#include <stdint.h>

#include <array>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "core/fxcrt/data_vector.h"
#include "core/fxcrt/fx_memory_wrappers.h"
#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/span.h"

class CPDF_Array;
class CPDF_Dictionary;
class CPDF_Object;

namespace fxcodec {
class ScanlineDecoder;
}

// Indexed by 8-bit char code, contains unicode code points.
extern const std::array<uint16_t, 256> kPDFDocEncoding;

bool ValidateDecoderPipeline(const CPDF_Array* pDecoders);

ByteString PDF_EncodeString(ByteStringView src);
ByteString PDF_HexEncodeString(ByteStringView src);
WideString PDF_DecodeText(pdfium::span<const uint8_t> span);
ByteString PDF_EncodeText(WideStringView str);

std::unique_ptr<fxcodec::ScanlineDecoder> CreateFaxDecoder(
    pdfium::span<const uint8_t> src_span,
    int width,
    int height,
    const CPDF_Dictionary* pParams);

std::unique_ptr<fxcodec::ScanlineDecoder> CreateFlateDecoder(
    pdfium::span<const uint8_t> src_span,
    int width,
    int height,
    int nComps,
    int bpc,
    const CPDF_Dictionary* pParams);

uint32_t RunLengthDecode(pdfium::span<const uint8_t> src_span,
                         std::unique_ptr<uint8_t, FxFreeDeleter>* dest_buf,
                         uint32_t* dest_size);

uint32_t A85Decode(pdfium::span<const uint8_t> src_span,
                   std::unique_ptr<uint8_t, FxFreeDeleter>* dest_buf,
                   uint32_t* dest_size);

uint32_t HexDecode(pdfium::span<const uint8_t> src_span,
                   std::unique_ptr<uint8_t, FxFreeDeleter>* dest_buf,
                   uint32_t* dest_size);

uint32_t FlateOrLZWDecode(bool bLZW,
                          pdfium::span<const uint8_t> src_span,
                          const CPDF_Dictionary* pParams,
                          uint32_t estimated_size,
                          std::unique_ptr<uint8_t, FxFreeDeleter>* dest_buf,
                          uint32_t* dest_size);

// Returns std::nullopt if the filter in |pDict| is the wrong type or an
// invalid decoder pipeline.
// Returns an empty vector if there is no filter, or if the filter is an empty
// array.
// Otherwise, returns a vector of decoders.
using DecoderArray =
    std::vector<std::pair<ByteString, RetainPtr<const CPDF_Object>>>;
std::optional<DecoderArray> GetDecoderArray(
    RetainPtr<const CPDF_Dictionary> pDict);

bool PDF_DataDecode(pdfium::span<const uint8_t> src_span,
                    uint32_t estimated_size,
                    bool bImageAcc,
                    const DecoderArray& decoder_array,
                    std::unique_ptr<uint8_t, FxFreeDeleter>* dest_buf,
                    uint32_t* dest_size,
                    ByteString* ImageEncoding,
                    RetainPtr<const CPDF_Dictionary>* pImageParams);

#endif  // CORE_FPDFAPI_PARSER_FPDF_PARSER_DECODE_H_
