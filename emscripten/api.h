/**
 * Structure of internal work area
 */
#define NJ_MAX_CHARSET_FROM_LEN                     1
#define NJ_MAX_CHARSET_TO_LEN                       3
#define NJ_APPROXSTORE_SIZE                         (NJ_MAX_CHARSET_FROM_LEN + NJ_TERM_LEN + NJ_MAX_CHARSET_TO_LEN + NJ_TERM_LEN)


#define jp_co_omronsoft_openwnn_OpenWnnDictionaryImplJni_POS_TYPE_V1 0L
#define jp_co_omronsoft_openwnn_OpenWnnDictionaryImplJni_POS_TYPE_V2 1L
#undef jp_co_omronsoft_openwnn_OpenWnnDictionaryImplJni_POS_TYPE_V3
#define jp_co_omronsoft_openwnn_OpenWnnDictionaryImplJni_POS_TYPE_V3 2L
#undef jp_co_omronsoft_openwnn_OpenWnnDictionaryImplJni_POS_TYPE_BUNTOU
#define jp_co_omronsoft_openwnn_OpenWnnDictionaryImplJni_POS_TYPE_BUNTOU 3L
#undef jp_co_omronsoft_openwnn_OpenWnnDictionaryImplJni_POS_TYPE_TANKANJI
#define jp_co_omronsoft_openwnn_OpenWnnDictionaryImplJni_POS_TYPE_TANKANJI 4L
#undef jp_co_omronsoft_openwnn_OpenWnnDictionaryImplJni_POS_TYPE_SUUJI
#define jp_co_omronsoft_openwnn_OpenWnnDictionaryImplJni_POS_TYPE_SUUJI 5L
#undef jp_co_omronsoft_openwnn_OpenWnnDictionaryImplJni_POS_TYPE_MEISI
#define jp_co_omronsoft_openwnn_OpenWnnDictionaryImplJni_POS_TYPE_MEISI 6L
#undef jp_co_omronsoft_openwnn_OpenWnnDictionaryImplJni_POS_TYPE_JINMEI
#define jp_co_omronsoft_openwnn_OpenWnnDictionaryImplJni_POS_TYPE_JINMEI 7L
#undef jp_co_omronsoft_openwnn_OpenWnnDictionaryImplJni_POS_TYPE_CHIMEI
#define jp_co_omronsoft_openwnn_OpenWnnDictionaryImplJni_POS_TYPE_CHIMEI 8L
#undef jp_co_omronsoft_openwnn_OpenWnnDictionaryImplJni_POS_TYPE_KIGOU
#define jp_co_omronsoft_openwnn_OpenWnnDictionaryImplJni_POS_TYPE_KIGOU 9L

typedef struct {
    int         size;
    NJ_UINT8*   from;
    NJ_UINT8*   to;
} PREDEF_APPROX_PATTERN;

