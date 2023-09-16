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

#include "CBOR.h"
#include "CommsTypes.h"
#include <avr/pgmspace.h>
#include <subprojects/QCBOR/inc/qcbor/qcbor_spiffy_decode.h>

#define ENABLE_SERIAL
#include "VirtualSerial.h"

static inline CheckError(QCBOREncodeContext* _pContext)
{
    QCBORError _err = QCBOREncode_GetErrorState(_pContext);
    if (_err != QCBOR_SUCCESS)
    {
        Serial_Print("QCBOR Error: ");
        Serial_PrintValue(_err);
        Serial_Print("\r\n");
    }
}

UsefulBufC CBOR_Encode_Message(const sMessage* pMessage, UsefulBuf Buffer)
{
    QCBOREncodeContext ctx;
    QCBOREncode_Init(&ctx, Buffer);

    // Create top level map for sMessage
    QCBOREncode_OpenMap(&ctx);

    // Create sub level map for sMessageHeader
    QCBOREncode_OpenMapInMap(&ctx, "h");
    QCBOREncode_AddUInt64ToMap(&ctx, "p", pMessage->Header.Protocol);
    QCBOREncode_AddUInt64ToMap(&ctx, "x", pMessage->Header.Priority);
    QCBOREncode_AddUInt64ToMap(&ctx, "m", pMessage->Header.ModuleParameter);

    // Create sub level map for source address
    QCBOREncode_OpenMapInMap(&ctx, "s");
    QCBOREncode_AddUInt64ToMap(&ctx, "t", pMessage->Header.Source.ClientType);
    QCBOREncode_AddUInt64ToMap(&ctx, "a", pMessage->Header.Source.ClientAddress);
    QCBOREncode_AddUInt64ToMap(&ctx, "m", pMessage->Header.Source.ModuleID);
    QCBOREncode_CloseMap(&ctx);

    // Create sub level map for destination address
    QCBOREncode_OpenMapInMap(&ctx, "d");
    QCBOREncode_AddUInt64ToMap(&ctx, "t", pMessage->Header.Destination.ClientType);
    QCBOREncode_AddUInt64ToMap(&ctx, "a", pMessage->Header.Destination.ClientAddress);
    QCBOREncode_AddUInt64ToMap(&ctx, "n", pMessage->Header.Destination.ModuleID);
    QCBOREncode_CloseMap(&ctx);

    // Close sMessageHeader map
    QCBOREncode_CloseMap(&ctx);

    QCBOREncode_AddUInt64ToMap(&ctx, "v", pMessage->Value);
    CheckError(&ctx);
    if (!UsefulBuf_IsNULLOrEmptyC(pMessage->Data))
    {
        QCBOREncode_AddEncodedToMap(&ctx, "d", pMessage->Data);
    }

    // Close sMessage map
    QCBOREncode_CloseMap(&ctx);

    UsefulBufC encodedData;
    QCBORError err = QCBOREncode_Finish(&ctx, &encodedData);
    if (err == QCBOR_SUCCESS)
    {
        return encodedData;
    }
    else
    {
        Serial_PrintValue(err);
    }

    return NULLUsefulBufC;
}

// UsefulBufC CBOREncode_Encoder(const sEncoderState* pEncoder, UsefulBuf Buffer)
// {
//     QCBOREncodeContext ctx;
//     QCBOREncode_Init(&ctx, Buffer);
//     QCBOREncode_AddUInt64(&ctx, pEncoder->CurrentValue);
//     QCBOREncode_AddUInt64(&ctx, pEncoder->PreviousValue);

//     UsefulBufC encodedData;
//     QCBORError err = QCBOREncode_Finish(&ctx, &encodedData);
//     if (err != QCBOR_SUCCESS)
//     {
//         return NULLUsefulBufC;
//     }
//     else
//     {
//         return encodedData;
//     }
// }

// QCBORError CBORDecode_Encoder(UsefulBufC EncodedStruct, sEncoderState* pDest)
// {
//     sEncoderState      pBuffer = {0};
//     QCBORDecodeContext ctx;
//     QCBORDecode_Init(&ctx, EncodedStruct, QCBOR_DECODE_MODE_NORMAL);
//     QCBORDecode_GetUInt64(&ctx, &pBuffer.CurrentValue);
//     QCBORDecode_GetUInt64(&ctx, &pBuffer.PreviousValue);
//     QCBORDecode_Finish(&ctx);
//     QCBORError err = QCBORDecode_GetAndResetError(&ctx);

//     if (err != QCBOR_SUCCESS)
//     {
//         return err;
//     }
//     else
//     {
//         *pDest = pBuffer;
//         return err;
//     }
// }