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
  getConnectAray: Module.cwrap('GetConnectArray', 'number', ['number']),
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

  eachWord: function(aCallback) {
    while (true) {
      if (OpenWnn.getNextWord(0) <= 0) {
        return;
      }
      var word = {
        stroke: OpenWnn.getStroke(),
        candidate: OpenWnn.getCandidate(),
        frequency: OpenWnn.getFrequency(),
        partOfSpeech: {
          left: OpenWnn.getLeftPartOfSpeech(),
          right: OpenWnn.getRightPartOfSpeech()
        }
      };
      if (!aCallback(word)) {
        return;
      }
    }
  },

  getStroke: function() {
    return Module.UTF16ToString(this._getStroke());
  },

  getCandidate: function() {
    return Module.UTF16ToString(this._getCandidate());
  },

  searchWord: function(aMode, aOrder, aSearch) {
    var ptr = Module._malloc((aSearch.length + 1) * 2);
    Module.stringToUTF16(aSearch, ptr);
    var ret = this._searchWord(aMode, aOrder, ptr);
    Module._free(ptr);
    return ret;
  },

  setCandidate: function(aCandidate) {
    var ptr = Module._malloc((aCandidate.length + 1) * 2);
    Module.stringToUTF16(aCandidate, ptr);
    this._setCandidate(ptr);
    Module._free(ptr);
  },

  setStroke: function(aStroke) {
    var ptr = Module._malloc((aStroke.length + 1) * 2);
    Module.stringToUTF16(aStroke, ptr);
    this._setStroke(ptr);
    Module._free(ptr);
  }
};
