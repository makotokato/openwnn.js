/* -*- Mode: js; tab-width: 2; indent-tab-mode: nul; c-base-offset: 2 -*- */
'use strict';

const CANDIDATE_COUNT_PER_POST = 40;

importScripts('emscripten_openwnn.js')

// Emscripten's OpenWnn binding

var OpenWnn = {
  SEARCH_EXACT: 0,
  SEARCH_PREFIX: 1,
  SEARCH_LINK: 2,

  ORDER_BY_FREQUENCY: 0,
  ORDER_BY_KEY: 1,

  _getStroke: Module.cwrap('GetStroke', 'number', []),
  _getCandidate: Module.cwrap('GetCandidate', 'number', []),
  _searchWord: Module.cwrap('SearchWord', 'number',
                            ['number', 'number', 'number']),
  _setCandidate: Module.cwrap('SetCandidate', 'number', ['number']),
  _setStroke: Module.cwrap('SetStroke', 'number', ['number']),

  initOpenWnn: Module.cwrap('InitOpenWnn', 'number', []),
  getFrequency: Module.cwrap('GetFrequency', 'number', []),
  getLeftPartOfSpeech: Module.cwrap('GetLeftPartOfSpeech', 'number', []),
  getNextWord: Module.cwrap('GetNextWord', 'number', ['number']),
  getRightPartOfSpeech: Module.cwrap('GetRightPartOfSpeech', 'number', []),
  selectWord: Module.cwrap('SelectWord', 'number', []),
  setDictionaryForIndependentWords:
    Module.cwrap('SetDictionaryForIndependentWords', 'number', []),
  setDictionaryForPrediction: Module.cwrap('SetDictionaryForPrediction',
                                           'number', ['number']),
  setLeftPartOfSpeech: Module.cwrap('SetLeftPartOfSpeech', 'number',
                                     ['number']),
  setRightPartOfSpeech: Module.cwrap('SetRightPartOfSpeech', 'number',
                                     ['number']),

  getStroke: function() {
    return Module.UTF16ToString(this._getStroke());
  },

  getCandidate: function() {
    return Module.UTF16ToString(this._getCandidate());
  },

  searchWord: function(aMode, aOrder, aSearch) {
    var ptr = Module._malloc((aSearch.length + 1) * 2);
    Module.stringToUTF16(aSearch, ptr);
    return this._searchWord(aMode, aOrder, ptr);
  },

  setCandidate: function(aCandidate) {
    var ptr = Module._malloc((aCandidate.length + 1) * 2);
    Module.stringToUTF16(aCandidate, ptr);
    this._setCandidate(ptr);
  },

  setStroke: function(aStroke) {
    var ptr = Module._malloc((aStroke.length + 1) * 2);
    Module.stringToUTF16(aStroke, ptr);
    this._setStroke(ptr);
  }
};

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
