/*
 * (C) Copyright 2015 Makoto Kato and others.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "nj_lib.h"
#include "nj_ext.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "api.h"
#include "predef_table.h"

#define TRUE 1
#define FALSE 0

// global variable for OpenWnn library
static void*           gDicLibHandle;
static NJ_DIC_HANDLE   gDicHandle[NJ_MAX_DIC];
static NJ_UINT32       gDicSize[NJ_MAX_DIC];
static NJ_UINT8        gDicType[NJ_MAX_DIC];
static NJ_CHAR         gKeyString[NJ_MAX_LEN + NJ_TERM_LEN];
static NJ_RESULT       gResult;
static NJ_CURSOR       gCursor;
static NJ_SEARCH_CACHE gSearchCache[NJ_MAX_DIC];
static NJ_DIC_SET      gDicSet;
static NJ_CLASS        gWnnClass;
static NJ_CHARSET      gApproxSet;
static NJ_CHAR         gApproxStr[NJ_MAX_CHARSET * NJ_APPROXSTORE_SIZE];
static NJ_CHAR         gPreviousStroke[NJ_MAX_LEN + NJ_TERM_LEN];
static NJ_CHAR         gPreviousCandidate[NJ_MAX_RESULT_LEN + NJ_TERM_LEN];

static uint8_t         gHasResult = FALSE;
static uint8_t         gHasCursor = FALSE;
static NJ_CHAR         gResultBuffer[NJ_MAX_RESULT_LEN + NJ_TERM_LEN];

// dictionary data
// XXX we should use sepalated file?
extern NJ_UINT8*       con_data[];
extern NJ_UINT8*       dic_data[];
extern NJ_UINT32       dic_size[];
extern NJ_UINT8        dic_type[];

enum OpenWnnOperation {
  SEARCH_EXACT = 0,
  SEARCH_PREFIX = 1,
  SEARCH_LINK = 2
};

enum OpenWnnOrder {
  ORDER_BY_FREQUENCY = 0,
  ORDER_BY_KEY = 1
};

enum ApproxPattern {
  APPROX_PATTERN_EN_TOUPPER = 0,
  APPROX_PATTERN_EN_TOLOWER = 1,
  APPROX_PATTERN_EN_QWERTY_NEAR = 2,
  APPROX_PATTERN_EN_QWERTY_NEAR_UPPER = 3,
  APPROX_PATTERN_JAJP_12KEY_NORMAL = 4
};

static NJ_CHAR
convertUTFCharToNjChar( NJ_UINT8* src )
{
  NJ_CHAR     ret;
  NJ_UINT8*   dst;

  /* convert UTF-16BE character to NJ_CHAR format */
  dst = ( NJ_UINT8* )&ret;
  dst[ 0 ] = src[ 0 ];
  dst[ 1 ] = src[ 1 ];

  return ret;
}

// NJ_CHAR is UTF16BE, but Emscrpten is UTF16LE
static void
ConvertUTF16LEToNjChar(const uint16_t* aSrc, NJ_CHAR* aDst)
{
  while (*aSrc != 0) {
    NJ_CHAR ch = *aSrc++;
    *aDst++ = (ch >> 8) | ((ch & 0xff) << 8);
  }
  *aDst = 0;
}

static void
ConvertNjCharToUTF16LE(const NJ_CHAR* aSrc, uint16_t* aDst)
{
  while (*aSrc != 0) {
    uint16_t ch = *aSrc++;
    *aDst++ = (ch >> 8) | ((ch & 0xff) << 8);
  }
  *aDst = 0;
}

int
InitOpenWnn()
{
  for (size_t i = 0; i < NJ_MAX_DIC; i++) {
    gDicHandle[i] = dic_data[i];
    gDicSize[i] = dic_size[i];
    gDicType[i] = dic_type[i];
  }
  gDicSet.rHandle[NJ_MODE_TYPE_HENKAN] = con_data[0];

  NJ_INT16 result = njx_init(&gWnnClass);
  return result;
}

static void
ClearDictionaryStructure(NJ_DIC_INFO* aDicInfo)
{
  aDicInfo->type = 0;
  aDicInfo->handle = NULL;

  aDicInfo->dic_freq[NJ_MODE_TYPE_HENKAN].base = 0;
  aDicInfo->dic_freq[NJ_MODE_TYPE_HENKAN].high = 0;
}

void
ClearDictionaryParameters()
{
  for (size_t i = 0; i < NJ_MAX_DIC; i++) {
    ClearDictionaryStructure(&gDicSet.dic[i]);
  }

  gHasCursor = FALSE;
  gHasResult = FALSE;
  memset(gDicSet.keyword, 0x00, sizeof(gDicSet.keyword));
}

int
SetDictionaryParameter(int aIndex, int aBase, int aHigh)
{
  if ((aIndex < 0 || aIndex > NJ_MAX_DIC - 1) ||
      (aBase < -1 || aBase > 1000) ||
      (aHigh < -1 || aHigh > 1000)) {
    return -1;
  }

  if (aBase < 0 || aHigh < 0 || aBase > aHigh) {
    ClearDictionaryStructure(&gDicSet.dic[aIndex]);
  } else {
    gDicSet.dic[aIndex].type = gDicType[aIndex];
    gDicSet.dic[aIndex].handle = gDicHandle[aIndex];
    gDicSet.dic[aIndex].srhCache = &gSearchCache[aIndex];
    gDicSet.dic[aIndex].dic_freq[NJ_MODE_TYPE_HENKAN].base = aBase;
    gDicSet.dic[aIndex].dic_freq[NJ_MODE_TYPE_HENKAN].high = aHigh;
  }

  gHasCursor = FALSE;
  gHasResult = FALSE;
  return 0;
}

int
SearchWord(enum OpenWnnOperation aOperation, enum OpenWnnOrder aOrder,
           const uint16_t* aKeyString)
{
  int result;

  NJ_CHAR yomi[NJ_MAX_LEN + 1];
  ConvertUTF16LEToNjChar(aKeyString, yomi);

  memset (&gCursor, 0, sizeof(NJ_CURSOR));
  gCursor.cond.operation = (NJ_UINT8) aOperation;
  gCursor.cond.mode = (NJ_UINT8) aOrder;
  gCursor.cond.ds = &gDicSet;
  gCursor.cond.yomi = yomi;
  gCursor.cond.charset = &gApproxSet;

  if (aOperation == SEARCH_LINK) {
    gCursor.cond.yomi = gPreviousStroke;
    gCursor.cond.kanji = gPreviousCandidate;
  }

  memcpy(&gWnnClass.dic_set, &gDicSet, sizeof(NJ_DIC_SET));
  result = njx_search_word(&gWnnClass, &gCursor);

  gHasCursor = (result == 1);
  gHasResult = FALSE;

  return result;
}

int
GetNextWord(int aLength)
{
  int result;

  if (!gHasCursor) {
    return 0;
  }

  if (aLength <= 0) {
    result = njx_get_word(&gWnnClass, &gCursor, &gResult);
  } else {
    do {
      result = njx_get_word(&gWnnClass, &gCursor, &gResult);
      if (aLength == (NJ_GET_YLEN_FROM_STEM(&gResult.word) +
                      NJ_GET_YLEN_FROM_FZK(&gResult.word))) {
        break;
     }
    } while (result > 0);
  }

  gHasResult = (result > 0);
  return result;
}

const uint16_t*
GetStroke()
{
  if (!gHasResult) {
    return NULL;
  }

  NJ_CHAR stroke[NJ_MAX_RESULT_LEN];
  stroke[0] = 0;
  njx_get_stroke(&gWnnClass, &gResult, stroke,
                 sizeof(NJ_CHAR) * (NJ_MAX_RESULT_LEN + NJ_TERM_LEN));
  ConvertNjCharToUTF16LE(stroke, gResultBuffer);
  return gResultBuffer;
}

const uint16_t*
GetCandidate()
{
  if (!gHasResult) {
    return NULL;
  }

  NJ_CHAR candidate[NJ_MAX_RESULT_LEN];
  candidate[0] = 0;
  njx_get_candidate(&gWnnClass, &gResult, candidate,
                    sizeof(NJ_CHAR) * (NJ_MAX_RESULT_LEN + NJ_TERM_LEN));
  ConvertNjCharToUTF16LE(candidate, gResultBuffer);
  return gResultBuffer;
}

int
GetFrequency()
{
  if (!gHasResult) {
    return 0;
  }
  return gResult.word.stem.hindo;
}

void
ClearApproxPatterns()
{
  for(size_t i = 0; i < NJ_MAX_CHARSET; i++) {
    gApproxSet.from[i] = NULL;
    gApproxSet.to[i] = NULL;
  }
  memset(gDicSet.keyword, 0, sizeof(gDicSet.keyword));
}

static void
SetApproxPattern(NJ_CHAR* aSrc, NJ_CHAR* aDst)
{
  if (gApproxSet.charset_count >= NJ_MAX_CHARSET) {
    return;
  }

  NJ_CHAR* from =
    gApproxStr + NJ_APPROXSTORE_SIZE * gApproxSet.charset_count;
  NJ_CHAR* to =
    gApproxStr + NJ_APPROXSTORE_SIZE * gApproxSet.charset_count +
    NJ_MAX_CHARSET_FROM_LEN + NJ_TERM_LEN;
  gApproxSet.from[gApproxSet.charset_count] = from;
  gApproxSet.to[gApproxSet.charset_count] = to;

  from = aSrc;
  to = aDst;
  gApproxSet.charset_count++;
  gHasCursor = FALSE;
  gHasResult = FALSE;
}

void
SetApproxPatternByPattern(enum ApproxPattern aApproxPattern)
{
  const PREDEF_APPROX_PATTERN* pattern =
    predefinedApproxPatterns[aApproxPattern];
  if (gApproxSet.charset_count + pattern->size <= NJ_MAX_CHARSET) {
    for (size_t i = 0; i < pattern->size; i++) {
      NJ_CHAR* from =
        gApproxStr + NJ_APPROXSTORE_SIZE * (gApproxSet.charset_count + i);
      NJ_CHAR* to =
        gApproxStr + NJ_APPROXSTORE_SIZE * (gApproxSet.charset_count + i) +
        NJ_MAX_CHARSET_FROM_LEN + NJ_TERM_LEN;
      gApproxSet.from[gApproxSet.charset_count + i] = from;
      gApproxSet.to[gApproxSet.charset_count + i] = to;

      from[0] = convertUTFCharToNjChar(pattern->from + i * 2);
      from[1] = 0x0;
      to[0] = convertUTFCharToNjChar(pattern->to + i * 2);
      to[1] = 0x0;
    }
    gApproxSet.charset_count = pattern->size;

    gHasCursor = FALSE;
    gHasResult = FALSE;
  }
}

static void
ClearResult()
{
  memset(&gResult, 0, sizeof(NJ_RESULT));
  memset(gPreviousStroke, 0, sizeof(gPreviousStroke));
  memset(gPreviousCandidate, 0, sizeof(gPreviousCandidate));
}

void
SetStroke(const uint16_t* aStroke)
{
  ConvertUTF16LEToNjChar(aStroke, gPreviousStroke);
}

void
SetCandidate(const uint16_t* aCandidate)
{
  ConvertUTF16LEToNjChar(aCandidate, gPreviousCandidate);
}

void
SelectWord()
{
  memcpy(&gWnnClass.dic_set, &gDicSet, sizeof(NJ_DIC_SET));
  njx_select(&gWnnClass, &gResult);
}

int
GetLeftPartOfSpeech()
{
  return NJ_GET_FPOS_FROM_STEM(&(gResult.word));
}

int
GetRightPartOfSpeech()
{
  return NJ_GET_BPOS_FROM_STEM(&(gResult.word));
}

int
SetLeftPartOfSpeech(NJ_UINT16 aPartOfSpeech)
{
  NJ_UINT16 lcount = 0;
  NJ_UINT16 rcount = 0;

  if (!gDicSet.rHandle[NJ_MODE_TYPE_HENKAN]) {
    return -1;
  }

  njd_r_get_count(gDicSet.rHandle[NJ_MODE_TYPE_HENKAN], &lcount, &rcount);
  if (aPartOfSpeech < 1 || aPartOfSpeech > lcount) {
    return -1;
  }

  NJ_SET_FPOS_TO_STEM(&(gResult.word ), aPartOfSpeech);
  return 0;
}

int
SetRightPartOfSpeech(NJ_UINT16 aPartOfSpeech)
{
  NJ_UINT16 lcount = 0;
  NJ_UINT16 rcount = 0;

  if (!gDicSet.rHandle[NJ_MODE_TYPE_HENKAN]) {
    return -1;
  }

  njd_r_get_count(gDicSet.rHandle[NJ_MODE_TYPE_HENKAN], &lcount, &rcount);
  if (aPartOfSpeech < 1 || aPartOfSpeech > rcount) {
    return -1;
  }

  NJ_SET_BPOS_TO_STEM(&(gResult.word ), aPartOfSpeech);
  return 0;
}

static NJ_CHAR*
getApproxPattern(NJ_CHAR* aSrc)
{
  int i;

  for (i = 0; i < gApproxSet.charset_count; i++) {
    if (!nj_strcmp(aSrc, gApproxSet.from[i])) {
      return gApproxSet.to[i];
    }
  }
  return NULL;
}

void
SetDictionaryForPrediction(size_t len)
{
  ClearDictionaryParameters();
  ClearApproxPatterns();

  if (len == 0) {
    SetDictionaryParameter(2, 245, 245);
    SetDictionaryParameter(3, 100, 244);
  } else {
    SetDictionaryParameter(0, 100, 400);
    if (len > 1) {
      SetDictionaryParameter(1, 100, 400);
    }

    SetDictionaryParameter(2, 245, 245);
    SetDictionaryParameter(3, 100, 244);

    SetApproxPatternByPattern(APPROX_PATTERN_JAJP_12KEY_NORMAL);
  }
}

void
SetDictionaryForAncillaryPattern()
{
  ClearDictionaryParameters();
  ClearApproxPatterns();

  SetDictionaryParameter(6, 400, 500);
}

void
SetDictionaryForIndependentWords()
{
  ClearDictionaryParameters();
  ClearApproxPatterns();

  SetDictionaryParameter(4, 0, 10);
  SetDictionaryParameter(5, 400, 500);
}
