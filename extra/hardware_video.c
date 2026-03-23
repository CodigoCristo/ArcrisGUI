/*
 * hardware_video.c - Verificador de drivers Nvidia para Arch Linux
 *
 * Escribe el modelo GPU y obtiene la versión del driver Linux 64-bit
 * consultando directamente la API de Nvidia (processFind.aspx).
 *
 * Compilar:
 *   gcc hardware_video.c -o nvidia_check
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ─── Paquetes Arch (según wiki Arch Linux) ─────────────────────────── *
 *  PKG_OPEN   Blackwell (GB)               → nvidia-open        extra
 *  PKG_NVIDIA Turing (TU) → Ada (AD)       → nvidia             extra
 *  PKG_580XX  Maxwell (GM) → Volta (GV100) → nvidia-580xx-dkms  AUR  legacy
 *  PKG_470XX  Kepler (GK)                  → nvidia-470xx-dkms  AUR  legacy
 *  PKG_390XX  Fermi (GF)                   → nvidia-390xx-dkms  AUR  legacy
 *  PKG_340XX  Tesla (G80–GT200)            → nvidia-340xx-dkms  AUR  legacy
 * ─────────────────────────────────────────────────────────────────────*/
typedef enum {
    PKG_OPEN,
    PKG_NVIDIA,
    PKG_580XX,
    PKG_470XX,
    PKG_390XX,
    PKG_340XX,
} PkgType;

static const char *pkg_main (PkgType t) {
    switch (t) {
        case PKG_OPEN:  return "nvidia-open";
        case PKG_580XX: return "nvidia-580xx-dkms";
        case PKG_470XX: return "nvidia-470xx-dkms";
        case PKG_390XX: return "nvidia-390xx-dkms";
        case PKG_340XX: return "nvidia-340xx-dkms";
        default:        return "nvidia";
    }
}
static const char *pkg_lts (PkgType t) {
    switch (t) {
        case PKG_OPEN:  return "nvidia-open-lts";
        case PKG_NVIDIA:return "nvidia-lts";
        default:        return "(solo dkms disponible)";
    }
}
static const char *pkg_dkms (PkgType t) {
    switch (t) {
        case PKG_OPEN:  return "nvidia-open-dkms";
        case PKG_580XX: return "nvidia-580xx-dkms";
        case PKG_470XX: return "nvidia-470xx-dkms";
        case PKG_390XX: return "nvidia-390xx-dkms";
        case PKG_340XX: return "nvidia-340xx-dkms";
        default:        return "nvidia-dkms";
    }
}
static const char *pkg_repo (PkgType t) {
    return (t == PKG_NVIDIA || t == PKG_OPEN) ? "extra" : "AUR";
}
static const char *pkg_status (PkgType t) {
    switch (t) {
        case PKG_OPEN:  return "Recomendado por ArchWiki";
        case PKG_NVIDIA:return "Soportado";
        case PKG_580XX: return "Legacy, soportado";
        case PKG_470XX: return "Legacy, sin soporte oficial";
        case PKG_390XX: return "Legacy";
        case PKG_340XX: return "Legacy";
        default:        return "";
    }
}

/* Familia GPU según serie Nvidia y paquete */
static const char *gpu_family (int psid, PkgType t) {
    switch (t) {
        case PKG_OPEN:
            return (psid == 133) ? "Blackwell (GB) — Laptop"
                                 : "Blackwell (GB) — Desktop";
        case PKG_NVIDIA:
            if (psid == 127 || psid == 129) return "Ada Lovelace (AD)";
            if (psid == 120 || psid == 123) return "Ampere (GA)";
            return "Turing (TU)";
        case PKG_580XX:
            if (psid == 101 || psid == 102) return "Pascal (GP)";
            return "Maxwell (GM)";
        case PKG_470XX: return "Kepler (GK)";
        case PKG_390XX: return "Fermi (GF)";
        case PKG_340XX: return "Tesla (G80/GT200)";
        default:        return "Desconocida";
    }
}

/* ─── Tabla de GPUs ─────────────────────────────────────────────────── */
/* IDs obtenidos de la API:
 *   TypeID=2&ParentID=1  → series (psid)
 *   TypeID=3&ParentID=N  → productos (pfid)
 *   processFind.aspx?psid=N&pfid=N&osid=12 → driver Linux 64-bit    */

typedef struct {
    const char *name;  /* en minúsculas, para matching */
    int         psid;
    int         pfid;
    PkgType     pkg;
} Gpu;

static const Gpu gpus[] = {
    /* ══ nvidia-open — RTX 50 Series Desktop (psid=131) ══════════════ */
    { "rtx 5090 d",          131, 1067, PKG_OPEN },
    { "rtx 5090",            131, 1066, PKG_OPEN },
    { "rtx 5080",            131, 1065, PKG_OPEN },
    { "rtx 5070 ti",         131, 1068, PKG_OPEN },
    { "rtx 5070",            131, 1070, PKG_OPEN },
    { "rtx 5060 ti",         131, 1076, PKG_OPEN },
    { "rtx 5060",            131, 1078, PKG_OPEN },
    { "rtx 5050",            131, 1090, PKG_OPEN },
    /* ══ nvidia-open — RTX 50 Laptop (psid=133) ══════════════════════ */
    { "rtx 5090 laptop",     133, 1073, PKG_OPEN },
    { "rtx 5080 laptop",     133, 1074, PKG_OPEN },
    { "rtx 5070 ti laptop",  133, 1075, PKG_OPEN },
    { "rtx 5070 laptop",     133, 1077, PKG_OPEN },
    { "rtx 5060 laptop",     133, 1079, PKG_OPEN },
    { "rtx 5050 laptop",     133, 1091, PKG_OPEN },
    /* ══ nvidia — RTX 40 Desktop (psid=127) ══════════════════════════ */
    { "rtx 4090 d",          127, 1036, PKG_NVIDIA },
    { "rtx 4090",            127,  995, PKG_NVIDIA },
    { "rtx 4080 super",      127, 1041, PKG_NVIDIA },
    { "rtx 4080",            127,  999, PKG_NVIDIA },
    { "rtx 4070 ti super",   127, 1040, PKG_NVIDIA },
    { "rtx 4070 ti",         127, 1001, PKG_NVIDIA },
    { "rtx 4070 super",      127, 1039, PKG_NVIDIA },
    { "rtx 4070",            127, 1015, PKG_NVIDIA },
    { "rtx 4060 ti",         127, 1022, PKG_NVIDIA },
    { "rtx 4060",            127, 1023, PKG_NVIDIA },
    /* ══ nvidia — RTX 40 Laptop (psid=129) ═══════════════════════════ */
    { "rtx 4090 laptop",     129, 1004, PKG_NVIDIA },
    { "rtx 4080 laptop",     129, 1005, PKG_NVIDIA },
    { "rtx 4070 laptop",     129, 1006, PKG_NVIDIA },
    { "rtx 4060 laptop",     129, 1007, PKG_NVIDIA },
    { "rtx 4050 laptop",     129, 1008, PKG_NVIDIA },
    /* ══ nvidia — RTX 30 Desktop (psid=120) ══════════════════════════ */
    { "rtx 3090 ti",         120,  985, PKG_NVIDIA },
    { "rtx 3090",            120,  930, PKG_NVIDIA },
    { "rtx 3080 ti",         120,  964, PKG_NVIDIA },
    { "rtx 3080",            120,  929, PKG_NVIDIA },
    { "rtx 3070 ti",         120,  965, PKG_NVIDIA },
    { "rtx 3070",            120,  933, PKG_NVIDIA },
    { "rtx 3060 ti",         120,  934, PKG_NVIDIA },
    { "rtx 3060",            120,  942, PKG_NVIDIA },
    { "rtx 3050",            120,  975, PKG_NVIDIA },
    /* ══ nvidia — RTX 30 Laptop (psid=123) ═══════════════════════════ */
    { "rtx 3080 ti laptop",  123,  976, PKG_NVIDIA },
    { "rtx 3080 laptop",     123,  938, PKG_NVIDIA },
    { "rtx 3070 ti laptop",  123,  979, PKG_NVIDIA },
    { "rtx 3070 laptop",     123,  939, PKG_NVIDIA },
    { "rtx 3060 laptop",     123,  940, PKG_NVIDIA },
    { "rtx 3050 ti laptop",  123,  962, PKG_NVIDIA },
    { "rtx 3050 laptop",     123,  963, PKG_NVIDIA },
    /* ══ nvidia — RTX 20 Desktop (psid=107) ══════════════════════════ */
    { "rtx 2080 ti",         107,  877, PKG_NVIDIA },
    { "rtx 2080 super",      107,  904, PKG_NVIDIA },
    { "rtx 2080",            107,  879, PKG_NVIDIA },
    { "rtx 2070 super",      107,  903, PKG_NVIDIA },
    { "rtx 2070",            107,  880, PKG_NVIDIA },
    { "rtx 2060 super",      107,  902, PKG_NVIDIA },
    { "rtx 2060",            107,  887, PKG_NVIDIA },
    /* ══ nvidia — RTX 20 Laptop (psid=111) ═══════════════════════════ */
    { "rtx 2080 super laptop",111,  919, PKG_NVIDIA },
    { "rtx 2080 laptop",     111,  890, PKG_NVIDIA },
    { "rtx 2070 super laptop",111,  920, PKG_NVIDIA },
    { "rtx 2070 laptop",     111,  889, PKG_NVIDIA },
    { "rtx 2060 laptop",     111,  888, PKG_NVIDIA },
    { "rtx 2050 laptop",     111,  978, PKG_NVIDIA },
    /* ══ nvidia — GTX 16 Desktop (psid=112) ══════════════════════════ */
    { "gtx 1660 super",      112,  910, PKG_NVIDIA },
    { "gtx 1650 super",      112,  911, PKG_NVIDIA },
    { "gtx 1660 ti",         112,  892, PKG_NVIDIA },
    { "gtx 1660",            112,  895, PKG_NVIDIA },
    { "gtx 1650",            112,  897, PKG_NVIDIA },
    { "gtx 1630",            112,  993, PKG_NVIDIA },
    /* ══ nvidia — GTX 16 Laptop (psid=115) ═══════════════════════════ */
    { "gtx 1660 ti laptop",  115,  899, PKG_NVIDIA },
    { "gtx 1650 ti laptop",  115,  921, PKG_NVIDIA },
    { "gtx 1650 laptop",     115,  898, PKG_NVIDIA },
    /* ══ nvidia-580xx — GTX 10 Desktop (psid=101) — Pascal ═══════════ */
    { "gtx 1080 ti",         101,  845, PKG_580XX },
    { "gtx 1080",            101,  815, PKG_580XX },
    { "gtx 1070 ti",         101,  859, PKG_580XX },
    { "gtx 1070",            101,  816, PKG_580XX },
    { "gtx 1060",            101,  817, PKG_580XX },
    { "gtx 1050 ti",         101,  825, PKG_580XX },
    { "gtx 1050",            101,  826, PKG_580XX },
    { "gt 1030",             101,  852, PKG_580XX },
    { "gt 1010",             101,  936, PKG_580XX },
    /* ══ nvidia-580xx — GTX 10 Laptop (psid=102) — Pascal ════════════ */
    { "gtx 1080 laptop",     102,  819, PKG_580XX },
    { "gtx 1070 laptop",     102,  820, PKG_580XX },
    { "gtx 1060 laptop",     102,  821, PKG_580XX },
    { "gtx 1050 ti laptop",  102,  836, PKG_580XX },
    { "gtx 1050 laptop",     102,  837, PKG_580XX },
    /* ══ nvidia-580xx — GTX 900 Desktop (psid=98) — Maxwell ══════════ */
    { "gtx 980 ti",           98,  778, PKG_580XX },
    { "gtx 980",              98,  755, PKG_580XX },
    { "gtx 970",              98,  756, PKG_580XX },
    { "gtx 960",              98,  764, PKG_580XX },
    { "gtx 950",              98,  782, PKG_580XX },
    /* ══ nvidia-580xx — GTX 900M Laptop (psid=99) — Maxwell ══════════ */
    { "gtx 980m",             99,  757, PKG_580XX },
    { "gtx 970m",             99,  758, PKG_580XX },
    { "gtx 965m",             99,  765, PKG_580XX },
    { "gtx 960m",             99,  769, PKG_580XX },
    { "gtx 950m",             99,  770, PKG_580XX },
    { "945m",                 99,  795, PKG_580XX },
    { "940mx",                99,  796, PKG_580XX },
    { "930mx",                99,  807, PKG_580XX },
    { "920mx",                99,  808, PKG_580XX },
    { "940m",                 99,  771, PKG_580XX },
    { "930m",                 99,  772, PKG_580XX },
    { "920m",                 99,  773, PKG_580XX },
    { "910m",                 99,  779, PKG_580XX },
    /* ══ nvidia-580xx — GTX 750/750 Ti (Maxwell, dentro de serie 700) */
    { "gtx 750 ti",           95,  727, PKG_580XX },
    { "gtx 750",              95,  728, PKG_580XX },
    /* ══ nvidia-470xx — GTX 700 Desktop (psid=95) ════════════════════ */
    { "gtx 780 ti",           95,  712, PKG_470XX },
    { "gtx 780",              95,  691, PKG_470XX },
    { "gtx 770",              95,  694, PKG_470XX },
    { "gtx 760 ti",           95,  711, PKG_470XX },
    { "gtx 760",              95,  703, PKG_470XX },
    { "gtx 745",              95,  730, PKG_470XX },
    { "gt 740",               95,  745, PKG_470XX },
    { "gt 730",               95,  746, PKG_470XX },
    { "gt 720",               95,  754, PKG_470XX },
    { "gt 710",               95,  742, PKG_470XX },
    { "gt 705",               95,  761, PKG_470XX },
    /* ══ nvidia-470xx — GTX 700M Laptop (psid=92) ════════════════════ */
    { "gtx 780m",             92,  700, PKG_470XX },
    { "gtx 775m",             92,  740, PKG_470XX },
    { "gtx 770m",             92,  702, PKG_470XX },
    { "gtx 765m",             92,  701, PKG_470XX },
    { "gtx 760m",             92,  699, PKG_470XX },
    { "gt 755m",              92,  707, PKG_470XX },
    { "gt 750m",              92,  690, PKG_470XX },
    { "gt 745m",              92,  689, PKG_470XX },
    { "gt 740m",              92,  686, PKG_470XX },
    { "gt 735m",              92,  688, PKG_470XX },
    { "gt 730m",              92,  672, PKG_470XX },
    { "gt 720m",              92,  687, PKG_470XX },
    { "gt 710m",              92,  798, PKG_470XX },
    { "720m",                 92,  803, PKG_470XX },
    { "710m",                 92,  680, PKG_470XX },
    { "705m",                 92,  775, PKG_470XX },
    /* ══ nvidia-470xx — GTX 600 Desktop (psid=85) ════════════════════ */
    { "gtx 690",              85,  627, PKG_470XX },
    { "gtx 680",              85,  610, PKG_470XX },
    { "gtx 670",              85,  629, PKG_470XX },
    { "gtx 660 ti",           85,  653, PKG_470XX },
    { "gtx 660",              85,  660, PKG_470XX },
    { "gtx 650 ti boost",     85,  683, PKG_470XX },
    { "gtx 650 ti",           85,  666, PKG_470XX },
    { "gtx 650",              85,  661, PKG_470XX },
    { "gtx 645",              85,  731, PKG_470XX },
    { "gt 645",               85,  633, PKG_470XX },
    { "gt 640",               85,  632, PKG_470XX },
    { "gt 635",               85,  685, PKG_470XX },
    { "gt 630",               85,  631, PKG_470XX },
    { "gt 625",               85,  741, PKG_470XX },
    { "gt 620",               85,  611, PKG_470XX },
    { "gt 610",               85,  630, PKG_470XX },
    /* ══ nvidia-470xx — GTX 600M Laptop (psid=84) ════════════════════ */
    { "gtx 680mx",            84,  677, PKG_470XX },
    { "gtx 680m",             84,  646, PKG_470XX },
    { "gtx 675mx",            84,  664, PKG_470XX },
    { "gtx 675m",             84,  638, PKG_470XX },
    { "gtx 670mx",            84,  663, PKG_470XX },
    { "gtx 670m",             84,  639, PKG_470XX },
    { "gtx 660m",             84,  637, PKG_470XX },
    { "gt 650m",              84,  636, PKG_470XX },
    { "gt 645m",              84,  662, PKG_470XX },
    { "gt 640m le",           84,  644, PKG_470XX },
    { "gt 640m",              84,  635, PKG_470XX },
    { "gt 635m",              84,  619, PKG_470XX },
    { "gt 630m",              84,  612, PKG_470XX },
    { "gt 625m",              84,  671, PKG_470XX },
    { "gt 620m",              84,  634, PKG_470XX },
    { "610m",                 84,  620, PKG_470XX },
    /* ══ nvidia-390xx — GTX 500 Desktop (psid=76) ════════════════════ */
    { "gtx 590",              76,  545, PKG_390XX },
    { "gtx 580",              76,  528, PKG_390XX },
    { "gtx 570",              76,  532, PKG_390XX },
    { "gtx 560 ti",           76,  540, PKG_390XX },
    { "gtx 560 se",           76,  609, PKG_390XX },
    { "gtx 560",              76,  560, PKG_390XX },
    { "gtx 555",              76,  626, PKG_390XX },
    { "gtx 550 ti",           76,  544, PKG_390XX },
    { "gt 545",               76,  559, PKG_390XX },
    { "gt 530",               76,  557, PKG_390XX },
    { "gt 520",               76,  554, PKG_390XX },
    /* ══ nvidia-390xx — GTX 500M Laptop (psid=78) ════════════════════ */
    { "gtx 580m",             78,  601, PKG_390XX },
    { "gtx 570m",             78,  603, PKG_390XX },
    { "gtx 560m",             78,  561, PKG_390XX },
    { "gt 555m",              78,  552, PKG_390XX },
    { "gt 550m",              78,  551, PKG_390XX },
    { "gt 540m",              78,  550, PKG_390XX },
    { "gt 525m",              78,  549, PKG_390XX },
    { "gt 520m",              78,  548, PKG_390XX },
    { "gt 520mx",             78,  602, PKG_390XX },
    /* ══ nvidia-390xx — GTX 400 Desktop (psid=71) ════════════════════ */
    { "gtx 480",              71,  490, PKG_390XX },
    { "gtx 470",              71,  491, PKG_390XX },
    { "gtx 465",              71,  498, PKG_390XX },
    { "gtx 460 se",           71,  530, PKG_390XX },
    { "gtx 460",              71,  499, PKG_390XX },
    { "gts 450",              71,  514, PKG_390XX },
    { "gt 440",               71,  541, PKG_390XX },
    { "gt 430",               71,  516, PKG_390XX },
    { "gt 420",               71,  538, PKG_390XX },
    /* ══ nvidia-390xx — GTX 400M Laptop (psid=72) ════════════════════ */
    { "gtx 485m",             72,  556, PKG_390XX },
    { "gtx 480m",             72,  500, PKG_390XX },
    { "gtx 470m",             72,  533, PKG_390XX },
    { "gtx 460m",             72,  518, PKG_390XX },
    { "gt 445m",              72,  523, PKG_390XX },
    { "gt 435m",              72,  519, PKG_390XX },
    { "gt 425m",              72,  520, PKG_390XX },
    { "gt 420m",              72,  521, PKG_390XX },
    { "gt 415m",              72,  522, PKG_390XX },
    /* ══ nvidia-340xx — GeForce 300 Desktop (psid=70) ════════════════ */
    { "gt 340",               70,  473, PKG_340XX },
    { "gt 330",               70,  474, PKG_340XX },
    { "gt 320",               70,  475, PKG_340XX },
    { "geforce 315",          70,  494, PKG_340XX },
    { "geforce 310",          70,  493, PKG_340XX },
    /* ══ nvidia-340xx — GeForce 300M Laptop (psid=69) ════════════════ */
    { "gts 360m",             69,  455, PKG_340XX },
    { "gts 350m",             69,  478, PKG_340XX },
    { "gt 335m",              69,  458, PKG_340XX },
    { "gt 330m",              69,  459, PKG_340XX },
    { "gt 325m",              69,  476, PKG_340XX },
    { "gt 320m",              69,  495, PKG_340XX },
    /* ══ nvidia-340xx — GeForce 200 Desktop (psid=52) ════════════════ */
    { "gtx 295",              52,  376, PKG_340XX },
    { "gtx 285",              52,  377, PKG_340XX },
    { "gtx 280",              52,  299, PKG_340XX },
    { "gtx 275",              52,  416, PKG_340XX },
    { "gtx 260",              52,  300, PKG_340XX },
    { "gts 250",              52,  399, PKG_340XX },
    { "gts 240",              52,  450, PKG_340XX },
    { "gt 240",               52,  470, PKG_340XX },
    { "gt 230",               52,  449, PKG_340XX },
    { "gt 220",               52,  448, PKG_340XX },
    { "g210",                 52,  447, PKG_340XX },
    /* ══ nvidia-340xx — GeForce 200M Laptop (psid=62) ════════════════ */
    { "gtx 285m",             62,  454, PKG_340XX },
    { "gtx 280m",             62,  421, PKG_340XX },
    { "gtx 260m",             62,  422, PKG_340XX },
    { "gts 260m",             62,  456, PKG_340XX },
    { "gts 250m",             62,  457, PKG_340XX },
    { "gt 240m",              62,  460, PKG_340XX },
    { "gt 230m",              62,  461, PKG_340XX },
    { "gt 220m",              62,  462, PKG_340XX },
    { "g210m",                62,  465, PKG_340XX },
    /* ══ nvidia-340xx — GeForce 100 Desktop (psid=59) ════════════════ */
    { "gt 140",               59,  400, PKG_340XX },
    { "gt 130",               59,  401, PKG_340XX },
    { "gt 120",               59,  402, PKG_340XX },
    { "g100",                 59,  415, PKG_340XX },
    /* ══ nvidia-340xx — GeForce 100M Laptop (psid=61) ════════════════ */
    { "gts 160m",             61,  424, PKG_340XX },
    { "gt 130m",              61,  425, PKG_340XX },
    { "gt 120m",              61,  426, PKG_340XX },
    /* ══ nvidia-340xx — GeForce 9 Desktop (psid=51) ══════════════════ */
    { "9800 gx2",             51,  286, PKG_340XX },
    { "9800 gtx",             51,  287, PKG_340XX },
    { "9800 gt",              51,  309, PKG_340XX },
    { "9600 gt",              51,  283, PKG_340XX },
    { "9600 gso",             51,  295, PKG_340XX },
    { "9500 gt",              51,  307, PKG_340XX },
    { "9400 gt",              51,  311, PKG_340XX },
    /* ══ nvidia-340xx — GeForce 9M Laptop (psid=53) ══════════════════ */
    { "9800m gtx",            53,  347, PKG_340XX },
    { "9800m gts",            53,  348, PKG_340XX },
    { "9800m gt",             53,  349, PKG_340XX },
    { "9600m gt",             53,  355, PKG_340XX },
    { "9500m gs",             53,  357, PKG_340XX },
    { "9400m",                53,  383, PKG_340XX },
    /* ══ nvidia-340xx — GeForce 8 Desktop (psid=1) ═══════════════════ */
    { "8800 ultra",            1,    8, PKG_340XX },
    { "8800 gtx",              1,    7, PKG_340XX },
    { "8800 gt",               1,  258, PKG_340XX },
    { "8800 gts",              1,    6, PKG_340XX },
    { "8600 gts",              1,    5, PKG_340XX },
    { "8600 gt",               1,    4, PKG_340XX },
    { "8500 gt",               1,    3, PKG_340XX },
    { "8400 gs",               1,    2, PKG_340XX },
    /* ══ nvidia-340xx — GeForce 8M Laptop (psid=54) ══════════════════ */
    { "8800m gtx",            54,  363, PKG_340XX },
    { "8800m gts",            54,  364, PKG_340XX },
    { "8600m gt",             54,  367, PKG_340XX },
    { "8600m gs",             54,  368, PKG_340XX },
    { "8400m gt",             54,  369, PKG_340XX },
    { "8400m gs",             54,  370, PKG_340XX },
};
static const int GPU_COUNT = (int)(sizeof(gpus) / sizeof(gpus[0]));

/* ─── Búsqueda ──────────────────────────────────────────────────────── */

static void str_lower(const char *src, char *dst, int len) {
    int i = 0;
    for (; src[i] && i < len - 1; i++)
        dst[i] = (char)tolower((unsigned char)src[i]);
    dst[i] = '\0';
}

/* Elimina prefijos comunes antes de buscar */
static const char *strip_prefix(const char *s) {
    const char *prefixes[] = { "nvidia geforce ", "geforce ", "nvidia ", NULL };
    for (int i = 0; prefixes[i]; i++) {
        size_t len = strlen(prefixes[i]);
        if (strncmp(s, prefixes[i], len) == 0)
            return s + len;
    }
    return s;
}

static const Gpu *find_gpu(const char *input_lower) {
    const char *key = strip_prefix(input_lower);

    /* 1) Coincidencia exacta */
    for (int i = 0; i < GPU_COUNT; i++)
        if (strcmp(gpus[i].name, input_lower) == 0 ||
            strcmp(gpus[i].name, key) == 0)
            return &gpus[i];

    /* 2) El input contiene el nombre completo del registro
     *    → preferir el nombre más largo (más específico) */
    {
        const Gpu *best = NULL;
        int best_len = -1;
        for (int i = 0; i < GPU_COUNT; i++) {
            if (strstr(input_lower, gpus[i].name) || strstr(key, gpus[i].name)) {
                int len = (int)strlen(gpus[i].name);
                if (len > best_len) { best_len = len; best = &gpus[i]; }
            }
        }
        if (best) return best;
    }

    /* 3) El nombre del registro contiene el input/key
     *    → preferir el nombre más corto (evita "4070 ti" → "4070 ti super") */
    {
        const Gpu *best = NULL;
        int best_len = 9999;
        for (int i = 0; i < GPU_COUNT; i++) {
            if (strstr(gpus[i].name, input_lower) || strstr(gpus[i].name, key)) {
                int len = (int)strlen(gpus[i].name);
                if (len < best_len) { best_len = len; best = &gpus[i]; }
            }
        }
        return best;
    }
}

/* ─── Consulta versión driver ───────────────────────────────────────── */

static int query_nvidia(int psid, int pfid, char *out, int out_len) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "curl -s --max-time 15 "
        "\"https://www.nvidia.com/Download/processFind.aspx"
        "?psid=%d&pfid=%d&osid=12&lid=1&whql=1&lang=en-us&ctk=0\"",
        psid, pfid);

    FILE *fp = popen(cmd, "r");
    if (!fp) return 0;

    char buf[131072] = {0};
    fread(buf, 1, sizeof(buf) - 1, fp);
    pclose(fp);

    const char *p = buf;
    while (*p) {
        if (isdigit((unsigned char)p[0]) &&
            isdigit((unsigned char)p[1]) &&
            isdigit((unsigned char)p[2]) &&
            p[3] == '.') {
            if (p > buf && isdigit((unsigned char)p[-1])) { p++; continue; }
            int i = 4;
            while (isdigit((unsigned char)p[i])) i++;
            if (i >= 6 && i <= 7) {
                int len = i;
                if (p[i] == '.') {
                    int j = i + 1;
                    while (isdigit((unsigned char)p[j])) j++;
                    if (j - i - 1 >= 2) len = j;
                }
                if (len >= out_len) len = out_len - 1;
                strncpy(out, p, len);
                out[len] = '\0';
                return 1;
            }
        }
        p++;
    }
    return 0;
}

/* ─── Main ──────────────────────────────────────────────────────────── */

int main(void) {
    printf("\n=== Driver Nvidia para Arch Linux ===\n\n");
    printf("Modelo GPU: ");
    fflush(stdout);

    char input[128] = {0};
    if (!fgets(input, sizeof(input), stdin)) return 1;
    input[strcspn(input, "\n")] = '\0';
    if (!input[0]) { printf("Modelo vacío.\n"); return 1; }

    char lower[128];
    str_lower(input, lower, sizeof(lower));

    const Gpu *gpu = find_gpu(lower);
    if (!gpu) {
        printf("\nNo encontrado: \"%s\"\n", input);
        printf("Prueba con el número del modelo: 5060, 4090, 1060, 780, 580...\n");
        return 1;
    }

    printf("Consultando Nvidia.com");
    fflush(stdout);
    char version[32] = "no disponible";
    int ok = query_nvidia(gpu->psid, gpu->pfid, version, sizeof(version));
    printf(" %s\n\n", ok ? "✓" : "(sin respuesta)");

    const char *family = gpu_family(gpu->psid, gpu->pkg);
    const char *status = pkg_status(gpu->pkg);
    const char *repo   = pkg_repo(gpu->pkg);

    printf("┌──────────────────────────────────────────────────┐\n");
    printf("│ GPU:     NVIDIA GeForce %-26s│\n", gpu->name);
    printf("│ Familia: %-41s│\n", family);
    printf("│ OS:      Linux x86_64 (64-bit)                   │\n");
    printf("│ Driver:  %-41s│\n", version);
    printf("│ Repo:    %-6s  Estado: %-25s│\n", repo, status);
    printf("├──────────────────────────────────────────────────┤\n");
    printf("│ Paquete recomendado:                             │\n");

    if (gpu->pkg == PKG_NVIDIA || gpu->pkg == PKG_OPEN) {
        printf("│  linux     → %-37s│\n", pkg_main(gpu->pkg));
        printf("│  linux-lts → %-37s│\n", pkg_lts(gpu->pkg));
        printf("│  otro ker. → %-37s│\n", pkg_dkms(gpu->pkg));
    } else {
        /* legacy: solo dkms disponible en AUR */
        printf("│  cualquier → %-37s│\n", pkg_dkms(gpu->pkg));
    }

    if (gpu->pkg == PKG_OPEN)
        printf("│  ★ Blackwell: driver open-source oficial         │\n");
    if (gpu->pkg == PKG_580XX)
        printf("│  ⚠ Maxwell→Volta: legacy, instalar desde AUR     │\n");
    if (gpu->pkg == PKG_470XX)
        printf("│  ⚠ Kepler: legacy, sin soporte oficial           │\n");
    if (gpu->pkg == PKG_390XX || gpu->pkg == PKG_340XX)
        printf("│  ⚠ Legacy — alternativa libre: nouveau           │\n");

    printf("└──────────────────────────────────────────────────┘\n\n");
    return 0;
}
