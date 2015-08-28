EMCC=emcc
OPENWNNPATH=../OpenWnn/libs

SOURCES =	\
	$(OPENWNNPATH)/libwnnDictionary/engine/ndapi.c \
	$(OPENWNNPATH)/libwnnDictionary/engine/ndbdic.c \
	$(OPENWNNPATH)/libwnnDictionary/engine/ndcommon.c \
	$(OPENWNNPATH)/libwnnDictionary/engine/ndfdic.c \
	$(OPENWNNPATH)/libwnnDictionary/engine/ndldic.c \
	$(OPENWNNPATH)/libwnnDictionary/engine/ndrdic.c \
	$(OPENWNNPATH)/libwnnDictionary/engine/neapi.c \
	$(OPENWNNPATH)/libwnnDictionary/engine/necode.c \
	$(OPENWNNPATH)/libwnnDictionary/engine/nj_str.c \
	$(NULL)

SOURCES +=	\
	$(OPENWNNPATH)/libwnnJpnDic/WnnJpnDic.c \
	$(NULL)

SOURCES +=	\
	api.c \
	$(NULL)

all: OpenWnn.js

clean:
	rm OpenWnn.js
	rm OpenWnn.js.mem

OpenWnn.js: $(SOURCES)
	$(EMCC) \
		-O1 \
		-I$(OPENWNNPATH)/libwnnDictionary/include \
		-I$(OPENWNNPATH)/libwnnDictionary \
		-o $@ $(SOURCES) \
		-s EXPORTED_FUNCTIONS="['_ClearApproxPatterns', '_ClearDictionaryParameters', '_GetCandidate', '_GetNextWord', '_GetStroke', '_InitOpenWnn', '_SearchWord', '_SetDictionaryParameter', '_SetApproxPatternByPattern','_SetDictionaryForPrediction']"
