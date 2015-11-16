/* -*- Mode: js; tab-width: 2; indent-tab-mode: nul; c-base-offset: 2 -*- */
'use strict';

const CANDIDATE_COUNT_PER_POST = 40;

importScripts('emscripten_openwnn.js')

// Helpers

function postCandidateListMessage(aStroke, aArray)
{
  if (!aArray.length) {
    return;
  }

  postMessage({
    cmd: 'candidatelist',
    stroke: aStroke,
    candidates: aArray
  });
}

function showCandidateList(aStroke, aCount)
{
  var candidates = [];
  var allCount = 0;
  var addedHiraganaOnly = false;

  var count = 0;
  while (true) {
    if (OpenWnn.getNextWord(0) <= 0) {
      break;
    }

    var result = {
      stroke: OpenWnn.getStroke(),
      candidate: OpenWnn.getCandidate(),
      frequency: OpenWnn.getFrequency(),
      partOfSpeech: {
        left: OpenWnn.getLeftPartOfSpeech(),
        right: OpenWnn.getRightPartOfSpeech(),
      }
    };
    var hasSameData = false;
    candidates.forEach(function(value) {
       if (value.candidate == result.candidate) {
         hasSameData = true;
       }
    });

    if (aStroke.length && result.candidate == aStroke) {
      addedHiraganaOnly = true;
    }

    if (!hasSameData) {
      candidates.push(result);
    }
    if (allCount + candidates.length >= aCount) {
      break;
    }
    if (!(candidates.length % CANDIDATE_COUNT_PER_POST)) {
      postCandidateListMessage(aStroke, candidates);
      allCount += candidates.length;
      candidates = [];
    }
  }

  if (aStroke.length && !addedHiraganaOnly) {
    candidates.push({
      stroke: aStroke,
      candidate: aStroke,
      frequency: 0,
      partOfSpeech: {
        left: 0,
        right: 0
      }
    });
  }

  postCandidateListMessage(aStroke, candidates);
}

// Worker's message handlers

function predict(aStroke, aCount)
{
  OpenWnn.setDictionaryForPrediction(aStroke.length);
  if (aStroke.length) {
    OpenWnn.searchWord(OpenWnn.SEARCH_PREFIX, OpenWnn.ORDER_BY_FREQUENCY,
                       aStroke);
  } else {
    OpenWnn.searchWord(OpenWnn.SEARCH_LINK, OpenWnn.ORDER_BY_FREQUENCY,
                       "");
  }
  showCandidateList(aStroke, aCount);
}

function select(aText, aCount)
{
  // set selected data to use next prediction
  OpenWnn.setStroke(aText.stroke);
  OpenWnn.setCandidate(aText.candidate);
  OpenWnn.setLeftPartOfSpeech(aText.partOfSpeech.left);
  OpenWnn.setRightPartOfSpeech(aText.partOfSpeech.right);
  OpenWnn.selectWord();

  OpenWnn.searchWord(OpenWnn.SEARCH_LINK, OpenWnn.ORDER_BY_FREQUENCY,
                     "");
  showCandidateList("", aCount);
}

function convert(aStroke, aCount)
{
  OpenWnn.setDictionaryForIndependentWords();
  OpenWnn.searchWord(OpenWnn.SEARCH_EXACT, OpenWnn.ORDER_BY_FREQUENCY,
                     aStroke);
  showCandidateList(aStroke, aCount);
}

onmessage = function(e) {
  switch(e.data.cmd) {
  case 'composing':
    predict(e.data.stroke, e.data.count);
    break;
  case 'select':
    select(e.data.selected, e.data.count);
    break;
  case 'convert':
    convert(e.data.stroke, e.data.count);
    break;
  }
};

OpenWnn.initOpenWnn();
