/***************************************************************************
 * libslink.h:
 *
 * Interface declarations for the SeedLink library (libslink).
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License (GNU-LGPL) for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2016 Chad Trabant, IRIS Data Management Center
 *
 * modified: 2019.283
 ***************************************************************************/

#ifndef FSDH_H
#define FSDH_H 1

#include <stdint.h>
#include <arpa/inet.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SLRECSIZE           512      /* Default Mini-SEED record size */
#define MAX_HEADER_SIZE     128      /* Max record header size */
#define SLHEADSIZE          8        /* SeedLink header size */
#define SELSIZE             8        /* Maximum selector size */
#define BUFSIZE             8192     /* Size of receiving buffer */
#define SIGNATURE           "SL"     /* SeedLink header signature */
#define INFOSIGNATURE       "SLINFO" /* SeedLink INFO packet signature */
#define MAX_LOG_MSG_LENGTH  200      /* Maximum length of log messages */

/* SeedLink packet types */
#define SLDATA 0     /* waveform data record */
#define SLDET  1     /* detection record */
#define SLCAL  2     /* calibration record */
#define SLTIM  3     /* timing record */
#define SLMSG  4     /* message record */
#define SLBLK  5     /* general record */
#define SLNUM  6     /* used as the error indicator (same as SLCHA) */
#define SLCHA  6     /* for requesting channel info or detectors */
#define SLINF  7     /* a non-terminating XML formatted message in a miniSEED
			log record, used for INFO responses */
#define SLINFT 8     /* a terminating XML formatted message in a miniSEED log
			record, used for INFO responses */
#define SLKEEP 9     /* an XML formatted message in a miniSEED log
			record, used for keepalive/heartbeat responses */

/* SEED structures */

/* Portability to the XScale (ARM) architecture requires a packed
 * attribute in certain places but this only works with GCC for now. */
#if defined (__GNUC__)
  #define SLP_PACKED __attribute__ ((packed))
#else
  #define SLP_PACKED
#endif

/* Generic struct for head of blockettes */
struct sl_blkt_head_s
{
  uint16_t  blkt_type;
  uint16_t  next_blkt;
} SLP_PACKED;

/* SEED binary time (10 bytes) */
struct sl_btime_s
{
  uint16_t  year;
  uint16_t  day;
  uint8_t   hour;
  uint8_t   min;
  uint8_t   sec;
  uint8_t   unused;
  uint16_t  fract;
} SLP_PACKED;

/* 100 Blockette (12 bytes) */
struct sl_blkt_100_s
{
  uint16_t  blkt_type;
  uint16_t  next_blkt;
  float     sample_rate;
  int8_t    flags;
  uint8_t   reserved[3];
} SLP_PACKED;

/* 1000 Blockette (8 bytes) */
struct sl_blkt_1000_s
{
  uint16_t  blkt_type;
  uint16_t  next_blkt;
  uint8_t   encoding;
  uint8_t   word_swap;
  uint8_t   rec_len;
  uint8_t   reserved;
} SLP_PACKED;

/* 1001 Blockette (8 bytes) */
struct sl_blkt_1001_s
{
  uint16_t  blkt_type;
  uint16_t  next_blkt;
  int8_t    timing_qual;
  int8_t    usec;
  uint8_t   reserved;
  int8_t    frame_cnt;
} SLP_PACKED;

/* 2000 Blockette (15 bytes) */
struct sl_blkt_2000_s
{
  uint16_t  blkt_type;
  uint16_t  next_blkt;
  uint16_t  total_len;
  uint16_t  data_offset;
  uint32_t  record_no;
  uint8_t   word_swap;
  uint8_t   flags;
  uint8_t   header_cnt;
} SLP_PACKED;

/* Fixed section data of header (48 bytes) */
struct sl_fsdh_s
{
  char        sequence_number[6];
  char        dhq_indicator;
  char        reserved;
  char        station[5];
  char        location[2];
  char        channel[3];
  char        network[2];
  struct sl_btime_s start_time;
  uint16_t    num_samples;
  int16_t     samprate_fact;
  int16_t     samprate_mult;
  uint8_t     act_flags;
  uint8_t     io_flags;
  uint8_t     dq_flags;
  uint8_t     num_blockettes;
  int32_t     time_correct;
  uint16_t    begin_data;
  uint16_t    begin_blockette;
} SLP_PACKED;

#ifdef __cplusplus
}
#endif

#endif /* FSDH_H */
