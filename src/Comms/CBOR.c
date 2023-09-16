/*
 * File: CBOREncoding.c ( 8th April 2022 )
 * Project: Muffin
 * Copyright 2022 Nicolaus Starke  
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

// For more information on CBOR see - https://www.rfc-editor.org/rfc/rfc8949.html

/**
 * @brief Prints an error to serial if the QCBOR context is in an error state
 * 
 * @param _pContext The QCBOR context to check
 */
static inline PrintIfError(QCBOREncodeContext* _pContext)
{
    QCBORError _err = QCBOREncode_GetErrorState(_pContext);
    if (_err != QCBOR_SUCCESS)
    {
        Serial_Print("QCBOR Error: ");
        Serial_PrintValue(_err);
        Serial_Print("\r\n");
    }
}

/**
 * @brief Encodes an sMessage, any additional raw data must be pre-encoded.
 * Any attached data must be valid CBOR encoded.
 * Note that this encoding uses tags for each item.
 * 
 * To calculate the required size for Buffer - (sizeof(sMessage) + SIZEOF_CBORLABEL * NUM_LABELS_MESSAGE)
 * 
 * @param pMessage A pointer to the message to encode.
 * @param Buffer The UsefulBuf that will store the encoded messages. Must be large enough to store the encoded message.
 * @return UsefulBufC A pointer to the encoded byte stream - will be NULL if encoding failed.
 */
UsefulBufC CBOR_Encode_Message(const sMessage* const pMessage, UsefulBuf Buffer)
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
    PrintIfError(&ctx);
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
        Serial_Print("QCBOR enc err:");
        Serial_PrintValue(err);
        Serial_Print("\r\n");
    }

    return NULLUsefulBufC;
}

// TODO Decode a message.