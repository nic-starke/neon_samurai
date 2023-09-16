/*
 * File: CBOREncoding.c ( 8th April 2022 )
 * Project: Muffin
 * Copyright 2022 bxzn (mail@bxzn.one)
 * -----
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#include <subprojects/QCBOR/inc/qcbor/qcbor_spiffy_decode.h>
#include "CBOR.h"

UsefulBufC CBOREncode_Encoder(const sEncoderState* pEncoder, UsefulBuf Buffer)
{
    QCBOREncodeContext ctx;
    QCBOREncode_Init(&ctx, Buffer);
    QCBOREncode_AddUInt64(&ctx, pEncoder->CurrentValue);
    QCBOREncode_AddUInt64(&ctx, pEncoder->PreviousValue);

    UsefulBufC encodedData;
    QCBORError err = QCBOREncode_Finish(&ctx, &encodedData);
    if(err != QCBOR_SUCCESS) {
        return NULLUsefulBufC;
    } else {
        return encodedData;
    }
}

QCBORError CBORDecode_Encoder(UsefulBufC EncodedStruct, sEncoderState* pDest)
{
    sEncoderState pBuffer = {0};
    QCBORDecodeContext ctx;
    QCBORDecode_Init(&ctx, EncodedStruct, QCBOR_DECODE_MODE_NORMAL);
    QCBORDecode_GetUInt64(&ctx, &pBuffer.CurrentValue);
    QCBORDecode_GetUInt64(&ctx, &pBuffer.PreviousValue);
    QCBORDecode_Finish(&ctx);
    QCBORError err = QCBORDecode_GetAndResetError(&ctx);

    if(err != QCBOR_SUCCESS) {
        return err;
    } else {
        *pDest = pBuffer;
        return err;
    }
}