/*
* Copyright (C) 2008 The Android Open Source Project
* All rights reserved.
* Copyright (c) 2013, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*  * Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in
*    the documentation and/or other materials provided with the
*    distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
* COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
* OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
* AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
* OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*/

//#define ALOG_NDEBUG 0
#define ALOG_NIDEBUG 0
#define LOG_TAG "QCameraLibJpegDecode"
#include <utils/Log.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <setjmp.h>

extern "C" {
#include "jpeglib.h"
#include "jpeg_buffer.h"
#include "jpeg_common.h"
#include "jpegd.h"
}

#include "QCameraMjpegDecode.h"

#define FILE_DUMP_HUFFMANINPUT 0
#define FILE_DUMP_HUFFMANOUTPUT 0

typedef struct {
    struct jpeg_source_mgr  jpeg_mgr;
    char*                   base;
    char*                   cursor;
    char*                   end;
} SourceMgrRec, *SourceMgr;


static void
_source_init_source(j_decompress_ptr cinfo)
{
    SourceMgr  src = (SourceMgr) cinfo->src;

    src->jpeg_mgr.next_input_byte = (unsigned char*)src->base,
    src->jpeg_mgr.bytes_in_buffer = src->end - src->base;
}

static int
_source_fill_input_buffer(j_decompress_ptr cinfo)
{
    SourceMgr  src = (SourceMgr) cinfo->src;

    cinfo->err->error_exit((j_common_ptr)cinfo);
    return FALSE;
}

static void
_source_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
    SourceMgr  src = (SourceMgr) cinfo->src;

    if (src->jpeg_mgr.next_input_byte + num_bytes > (unsigned char*)src->end ) {
        cinfo->err->error_exit((j_common_ptr)cinfo);
    }

    src->jpeg_mgr.next_input_byte += num_bytes;
    src->jpeg_mgr.bytes_in_buffer -= num_bytes;
}

static int
_source_resync_to_restart( j_decompress_ptr cinfo, int desired)
{
    SourceMgr  src = (SourceMgr) cinfo->src;

    src->jpeg_mgr.next_input_byte = (unsigned char*)src->base;
    src->jpeg_mgr.bytes_in_buffer = src->end - src->base;
    return TRUE;
}

static void
_source_term_source(j_decompress_ptr  cinfo)
{
    // nothing to do
}

static void
_source_init( SourceMgr  src, char*  base, long  size )
{
    src->base   = base;
    src->cursor = base;
    src->end    = base + size;

    src->jpeg_mgr.init_source       = _source_init_source;
    src->jpeg_mgr.fill_input_buffer = _source_fill_input_buffer;
    src->jpeg_mgr.skip_input_data   = _source_skip_input_data;
    src->jpeg_mgr.resync_to_restart = _source_resync_to_restart;
    src->jpeg_mgr.term_source       = _source_term_source;
}

typedef struct {
    struct jpeg_error_mgr   jpeg_mgr;
    jmp_buf                 jumper;
    int volatile            error;

} ErrorMgrRec, *ErrorMgr;

static void _error_exit(j_common_ptr cinfo)
{
    ErrorMgr error = (ErrorMgr) cinfo->err;
    (*error->jpeg_mgr.output_message) (cinfo);
    ALOGE("%s Error reported by JPEG decoder Msg Code: %d Err: %s",
        __func__, error->jpeg_mgr.msg_code,
        error->jpeg_mgr.jpeg_message_table[error->jpeg_mgr.msg_code]);

    /* Let the memory manager delete any temp files before we die */
    longjmp(error->jumper, -1);
}

char mjpeg_huffman_table[] = {
    0xFF,0xC4,0x01,0xA2,0x00,0x00,0x01,0x05,0x01,0x01,0x01,0x01,
    0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x02,
    0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x01,0x00,0x03,
    0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,
    0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,
    0x0A,0x0B,0x10,0x00,0x02,0x01,0x03,0x03,0x02,0x04,0x03,0x05,
    0x05,0x04,0x04,0x00,0x00,0x01,0x7D,0x01,0x02,0x03,0x00,0x04,
    0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,0x22,
    0x71,0x14,0x32,0x81,0x91,0xA1,0x08,0x23,0x42,0xB1,0xC1,0x15,
    0x52,0xD1,0xF0,0x24,0x33,0x62,0x72,0x82,0x09,0x0A,0x16,0x17,
    0x18,0x19,0x1A,0x25,0x26,0x27,0x28,0x29,0x2A,0x34,0x35,0x36,
    0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,
    0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,
    0x67,0x68,0x69,0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,
    0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x92,0x93,0x94,0x95,
    0x96,0x97,0x98,0x99,0x9A,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,
    0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xC2,
    0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,
    0xD6,0xD7,0xD8,0xD9,0xDA,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,
    0xE8,0xE9,0xEA,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,
    0xFA,0x11,0x00,0x02,0x01,0x02,0x04,0x04,0x03,0x04,0x07,0x05,
    0x04,0x04,0x00,0x01,0x02,0x77,0x00,0x01,0x02,0x03,0x11,0x04,
    0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,0x13,0x22,
    0x32,0x81,0x08,0x14,0x42,0x91,0xA1,0xB1,0xC1,0x09,0x23,0x33,
    0x52,0xF0,0x15,0x62,0x72,0xD1,0x0A,0x16,0x24,0x34,0xE1,0x25,
    0xF1,0x17,0x18,0x19,0x1A,0x26,0x27,0x28,0x29,0x2A,0x35,0x36,
    0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,
    0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,
    0x67,0x68,0x69,0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,
    0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x92,0x93,0x94,
    0x95,0x96,0x97,0x98,0x99,0x9A,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,
    0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,
    0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,
    0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,
    0xE8,0xE9,0xEA,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,
};

/******************************************************************************
 * Function: fileDump
 * Description: This is a utility function to dump buffers into a file
 *
 * Input parameters:
 *  fn              - File name string
 *  data            - pointer to character buffer that needs to be dumped
 *  length          - Length of the buffer to be dumped
 *  frm_cnt         - Pointer to frame count. This count is incremented by this
 *                      function on successful file write
 * Return values:
 *      0   Success
 *      -1  Error
 * Notes: none
 *****************************************************************************/
static int fileDump(const char* fn, char* data, int length, int* frm_cnt)
{

    FILE *fp = NULL;
    if (0 == *frm_cnt) {
        fp = fopen(fn, "wb");
        if (NULL == fp) {
            ALOGE("%s: Error in opening %s", __func__, fn);
        }
        fclose(fp);
    }
    fp = fopen(fn, "ab");
    if (NULL == fp) {
        ALOGE("%s: Error in opening %s", __func__, fn);
    }
    fwrite(data, 1, length, fp);
    fclose(fp);
    (*frm_cnt)++;
    ALOGD("%s: Written %d bytes for frame:%d, in %s",
        __func__, length, *frm_cnt, fn);

    return 0;
}

/* This function parses MJPEG frame to check whether huffman code is present.*/
/* If not present, then inserts default table for MJPEG */
static int insertHuffmanTable(char* inputMjpegBuffer,
                                int   inputMjpegBufferSize,
                                int*  pInputHasHT,
                                char** ppNewMjpegBufferWithHT,
                                int*  pNewMjpegBufferWithHTsize)
{
    int rc = 0;
    int bytes_written = 0;
    int bytes_parsed = 0;
    char hdr[4];
    int dht_present = 0;
    int jump_size = 0;

    ALOGD("%s: E", __func__ );
    /* Check if frame starts with 0xff 0xd8 */
    memcpy(hdr, inputMjpegBuffer + bytes_parsed, 2);
    bytes_parsed += 2;
    if ((hdr[0] != 0xff) || (hdr[1] != 0xd8)){
        ALOGE("%s: Invalid MJPEG frame header 1", __func__);
        return -1;
    }
    /* scan through the frame whether huffman table is present */
    while(!dht_present){
        memcpy(hdr, inputMjpegBuffer + bytes_parsed, 4);
        bytes_parsed += 4;
        if(hdr[0] != 0xff){
            ALOGE("%s: Invalid MJPEG frame header 2", __func__);
            return -1;
        }

        if(hdr[1] == 0xc4){
            /* Huffman table is present */
            dht_present = 1;
        }
        else if (hdr[1] == 0xda)
            break;

        /* Jump to the next marker before iterating the loop */
        jump_size = (hdr[2] << 8) + hdr[3];
        ALOGD("%s: hdr[2] = %d, hdr[3] = %d, jump_size = %d",
            __func__, hdr[2], hdr[3], jump_size);
        jump_size -= 2;

        bytes_parsed += jump_size;
    }

    if(dht_present)
    {
        ALOGD("%s: No need to insert Huffman Table", __func__);
        *pInputHasHT = 1;
    }
    else
    {
        /* If huffman table is not present, then insert it */
        char* pNewMjpegBufferWithHT = NULL;
        int   newMjpegBufferWithHTsize;

        ALOGD("%s: Input does not have HT. Need to insert HT", __func__);
        newMjpegBufferWithHTsize = inputMjpegBufferSize + sizeof(mjpeg_huffman_table);

        /* Can allocate this buffer from heap as libjpeg is s/w decoder */
        /* Note: If h/w decoder is employed, this needs to be allocated from ION memory */
        pNewMjpegBufferWithHT = (char *) malloc(newMjpegBufferWithHTsize);
        if(NULL == pNewMjpegBufferWithHT){
            ALOGE("%s: Malloc for pNewMjpegBufferWithHT returned NULL", __func__);
            return -1;
        }
        /* Copy all the bytes parsed so far except the last 4 bytes marker */
        /* to the new output mjpeg buffer. The last 4 bytes marker will be copied after */
        /* inserting the huffman table */
        memcpy(pNewMjpegBufferWithHT, inputMjpegBuffer, bytes_parsed - 4);
        bytes_written += (bytes_parsed - 4);

        /* Copy huffman table to the new output mjpeg buffer */
        memcpy(pNewMjpegBufferWithHT + bytes_written, mjpeg_huffman_table, sizeof(mjpeg_huffman_table));
        bytes_written += sizeof(mjpeg_huffman_table);

        /* Now copy the 4 byte marker saved in hdr[] at the end of the huffman table */
        memcpy(pNewMjpegBufferWithHT + bytes_written, hdr, 4);
        bytes_written += 4;

        /* Copy rest of the frame to the new output mjpeg buffer */
        memcpy(pNewMjpegBufferWithHT + bytes_written, inputMjpegBuffer + bytes_parsed, inputMjpegBufferSize - bytes_parsed);
        bytes_written   += (inputMjpegBufferSize - bytes_parsed);

        *pInputHasHT = 0;
        *ppNewMjpegBufferWithHT = pNewMjpegBufferWithHT;
        *pNewMjpegBufferWithHTsize = newMjpegBufferWithHTsize;
    }
    ALOGD("%s: X", __func__ );
    return rc;
}

MJPEGD_ERR libJpegDecode(
            char*   inputMjpegBuffer,
            int     inputMjpegBufferSize,
            char*   outputYptr,
            char*   outputUVptr,
            int     outputFormat)
{
    ErrorMgrRec             errmgr;
    SourceMgrRec            sourcemgr;
    struct jpeg_decompress_struct  cinfo;
    int volatile            error = 0;
    JSAMPLE*                temprow;
    int                     inputHasHT = 1;
    char*                   newMjpegBufferWithHT = NULL;
    int                     newMjpegBufferWithHTsize = 0;

    ALOGD("%s: E", __func__ );

    memset( &cinfo, 0, sizeof(cinfo) );
    memset( &errmgr, 0, sizeof(errmgr) );

    jpeg_create_decompress(&cinfo);
    cinfo.err         = jpeg_std_error(&errmgr.jpeg_mgr);

    errmgr.jpeg_mgr.error_exit = _error_exit;
    errmgr.error      = 0;

    if (setjmp(errmgr.jumper) != 0) {
        ALOGE("%s: setjmp failed", __func__ );
        goto Exit;
    }

    error = insertHuffmanTable(inputMjpegBuffer,
                            inputMjpegBufferSize,
                            &inputHasHT,
                            &newMjpegBufferWithHT,
                            &newMjpegBufferWithHTsize);
    if(0 != error){
        ALOGE("%s: Error returned from insertHuffmanTable", __func__);
    }
    ALOGD("%s: inputHasHT: %d inputMjpegBufferSize: %d "
        "newMjpegBufferWithHT: %p newMjpegBufferWithHTsize: %d", __func__,
        inputHasHT, inputMjpegBufferSize, newMjpegBufferWithHT,
        newMjpegBufferWithHTsize);

#if FILE_DUMP_HUFFMANINPUT
    {
        static int frame_cnt = 0;
        fileDump("/data/insertHTinput.jpeg",
            (char*)inputMjpegBuffer, inputMjpegBufferSize, &frame_cnt);
    }
#endif

#if FILE_DUMP_HUFFMANOUTPUT
    if(newMjpegBufferWithHT)
    {
        static int frame_cnt = 0;
        /* currently hardcoded for Bytes-Per-Pixel = 1.5 */
        fileDump("/data/insertHToutput.jpeg",
            (char*)newMjpegBufferWithHT, newMjpegBufferWithHTsize, &frame_cnt);
    }
#endif
    if(inputHasHT)
        _source_init( &sourcemgr, inputMjpegBuffer, inputMjpegBufferSize);
    else
        _source_init( &sourcemgr, newMjpegBufferWithHT, newMjpegBufferWithHTsize);

    cinfo.src = &sourcemgr.jpeg_mgr;

    jpeg_read_header(&cinfo, 1);
    ALOGD("%s: jpeg header info: "
            "num_components: %d, jpeg_color_space: %d, JCS_RGB: %d, "
            "JCS_GRAYSCALE: %d JCS_YCbCr: %d output scale_num: %d scale_denom %d "
            "original_image_width: %d, image_width: %d, image_height: %d ",
            __func__, cinfo.num_components, cinfo.jpeg_color_space, JCS_RGB,
            JCS_GRAYSCALE, JCS_YCbCr, cinfo.scale_num, cinfo.scale_denom,
            cinfo.original_image_width, cinfo.image_width, cinfo.image_height);

    cinfo.scale_num = 1;
    cinfo.scale_denom = 1;
    //cinfo.raw_data_out = TRUE;// TRUE=downsampled data wanted
    /* TBD: Try JDCT_IFAST for performance? */
    cinfo.dct_method = JDCT_DEFAULT;
    cinfo.out_color_space = JCS_YCbCr;

    jpeg_start_decompress(&cinfo);

    ALOGD("%s: after start compress: out_color_components:%d, output_components: %d "
         "rec_outbuf_height: %d output_scanline: %d outwidth: %d, outheight: %d ",
        __func__, cinfo.out_color_components, cinfo.output_components,
        cinfo.rec_outbuf_height, cinfo.output_scanline, cinfo.output_width, cinfo.output_height);

    temprow = (JSAMPLE*) calloc( cinfo.out_color_components * cinfo.output_width, sizeof(JSAMPLE) );
    if(NULL == temprow)
        ALOGE("%s: Unable to allocate memory for row buffer", __func__);

    /* LibJpeg provides YCrCb444 packed data - Y1Cr1Cb1Y2Cr2Cb2Y3Cr3Cb3... */
    /* Parse this data and rearrange as per output format = NV12 */
    {
        char *yptr  = outputYptr;
        char *uvptr = outputUVptr;
        unsigned  row, col;
        for (row = 0; row < cinfo.output_height; row++) {
            JSAMPLE*  rowptr = temprow;
            (void)jpeg_read_scanlines(&cinfo, &rowptr, 1);

            for(col = 0; col < cinfo.output_width * cinfo.out_color_components;  col += 3){
               *yptr++ = rowptr[col];
            }
            /* Horizontal and vertical subsampling: Drop CrCb from odd numbered lines */
            if((row & 1) == 0){
                for(col = 1; col < cinfo.output_width * cinfo.out_color_components; col += 6){
                    *uvptr++ = rowptr[col + 1];
                    *uvptr++ = rowptr[col];
                }
            }
        }
    }

    jpeg_finish_decompress(&cinfo);
    if( temprow )
        free( temprow );
    if(newMjpegBufferWithHT)
        free (newMjpegBufferWithHT);

Exit:
    jpeg_destroy_decompress(&cinfo);
    ALOGD("%s: X", __func__ );
    return error;
}

