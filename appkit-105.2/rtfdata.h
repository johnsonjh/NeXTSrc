/*
	rtfdata.h
  	Copyright 1988, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

/*
 * Global data for RTFText.m
 */

#import "Font.h"
#ifndef RTFSTRUCTS_H
#import "rtfstructs.h"
#endif RTFSTRUCTS_H

static const RichTextFamily richTextFamilies[] = {
    {"Courier", "modern"},
    {"Ohlfs", "modern"},
    {"Helvetica", "swiss"},
    {"Symbol", "tech"},
    {"Times", "roman"},
    {0, 0}
};

static const char *TimesMap[] = {
    "Times", "Times Roman", "Times-Roman", "Tms-Rmn", "Tms Rmn", 0
};

static const NXSymbolTable unhashedSymbols[] = {
    {"chpgn", SPECIAL, 0},
    {"chftn", SPECIAL, 0},
    {"chdate", SPECIAL, 0},
    {"|", SPECIAL, 0},
    {"~", SPECIAL, NX_FIGSPACE},
    {"-", SPECIAL, '-'},
    {"_", SPECIAL, '-'},
    {"\'", SPECIAL, HEX_SPECIAL},
    {"\r", SPECIAL, '\n'},
    {"page", SPECIAL, '\n'},
    {"line", SPECIAL, '\n'},
    {"par", SPECIAL, '\n'},
    {"sect", SPECIAL, '\n'},
    {"tab", SPECIAL, '\t'},
    {"paperw", SPECIAL, PAPERW_SPECIAL},
    {"paperh", SPECIAL, PAPERH_SPECIAL},
    {"snext", SPECIAL, NEXTSTYLE_SPECIAL},  
    {"sbasedon", SPECIAL, BASEDONSTYLE_SPECIAL},
    {"tx", SPECIAL, SETTAB_SPECIAL},
    {"margl", SPECIAL, MARGL_SPECIAL},
    {"margr", SPECIAL, MARGR_SPECIAL},
    {"smartcopy", SPECIAL, SMARTCOPY_SPECIAL},

    {"s",  DEST, STYLE_GROUP},
    {"rtf", DEST, DOCUMENT_GROUP},
    {"colortbl", DEST, COLORTABLE_GROUP},
    {"fonttbl", DEST, FONTTABLE_GROUP},
    {"stylesheet", DEST, STYLESHEET_GROUP},
    {"pict", DEST, PICTURE_GROUP},
    {"footnote", DEST, FOOTNOTE_GROUP},
    {"header", DEST, HEADER_GROUP},
    {"headerl", DEST, HEADER_GROUP},
    {"headerr", DEST, HEADER_GROUP},
    {"headerf", DEST, HEADER_GROUP},
    {"footer", DEST, FOOTER_GROUP},
    {"footerl", DEST, FOOTER_GROUP},
    {"footerr", DEST, FOOTER_GROUP},
    {"footerf", DEST, FOOTER_GROUP},
    {"ftnsep", DEST, 0},
    {"ftnsepc", DEST, 0},
    {"fntcn", DEST, 0},
    {"info", DEST, INFO_GROUP},
    {"comment", DEST, 0},
    {"attachment", DEST, ATTACHMENT_GROUP},

    {"f", PROP, FONTPROP},
    {"b", PROP, BOLDPROP},
    {"i", PROP, ITALICPROP},
    {"fs", PROP, SIZEPROP},
    {"fi", PROP, FIRSTINDENTPROP},
    {"li", PROP, LEFTINDENTPROP},
    {"ql", PROP, LEFTJUSTPROP},
    {"qr", PROP, RIGHTJUSTPROP},
    {"qj", PROP, JUSTIFYPROP},
    {"qc", PROP, CENTERPROP},
    {"up", PROP, SUPERSCRIPTPROP},
    {"dn", PROP, SUBSCRIPTPROP},
    {"pard", PROP, INITPROP},
    {"plain", PROP, PLAINPROP},
    {"gray", PROP, GRAYPROP},
    {"fc", PROP, COLORPROP},
    {"ul", PROP, UNDERLINEPROP},
    {"ulnone", PROP, STOPUNDERLINEPROP},
    {"red", PROP, REDPROP},
    {"green", PROP, GREENPROP},
    {"blue", PROP, BLUEPROP},
    {0, 0, 0}
};

