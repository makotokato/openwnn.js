/* -*- Mode: js; tab-width: 2; indent-tab-mode: nul; c-base-offset: 2 -*- */
'use strict';

const CANDIDATE_COUNT_PER_POST = 10;

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

  initOpenWnn: Module.cwrap('InitOpenWnn', 'number', []),
  getFrequency: Module.cwrap('GetFrequency', 'number', []),
  getNextWord: Module.cwrap('GetNextWord', 'number', ['number']),
  setDictionaryForIndependentWords:
    Module.cwrap('SetDictionaryForIndependentWords', 'number', []),
  setDictionaryForPrediction: Module.cwrap('SetDictionaryForPrediction',
                                           'number', ['number']),

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

    candidates.push({
      stroke: OpenWnn.getStroke(),
      candidate: OpenWnn.getCandidate(),
      frequency: OpenWnn.getFrequency()
    });
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
  OpenWnn.searchWord(OpenWnn.SEARCH_PREFIX, OpenWnn.ORDER_BY_FREQUENCY,
                     aStroke);
  showCandidateList(aStroke, aCount);
}

function select(aStroke, aCandidate)
{
  // set selected data to use next prediction
  OpenWnn.setStroke(aStroke);
  OpenWnn.setCandidate(aCandidate);
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
    select(e.data.stroke, e.data.candidate);
    break;
  case 'convert':
    convert(e.data.stroke, e.data.count);
    break;
  }
};

OpenWnn.initOpenWnn();
