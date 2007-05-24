// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#include "entityparser.h"


/* **********************************************************************
				   CONSTANTS
 * ********************************************************************** */

typedef std::pair<std::string, std::string > StrPair;

StrPair _html_entity_list[] =  {
	StrPair("nbsp",std::string("\xc2\xa0",2)), //  
	StrPair("iexcl",std::string("\xc2\xa1",2)), // ¡
	StrPair("cent",std::string("\xc2\xa2",2)), // ¢
	StrPair("pound",std::string("\xc2\xa3",2)), // £
	StrPair("curren",std::string("\xc2\xa4",2)), // ¤
	StrPair("yen",std::string("\xc2\xa5",2)), // ¥
	StrPair("brvbar",std::string("\xc2\xa6",2)), // ¦
	StrPair("sect",std::string("\xc2\xa7",2)), // §
	StrPair("uml",std::string("\xc2\xa8",2)), // ¨
	StrPair("copy",std::string("\xc2\xa9",2)), // ©
	StrPair("ordf",std::string("\xc2\xaa",2)), // ª
	StrPair("laquo",std::string("\xc2\xab",2)), // «
	StrPair("not",std::string("\xc2\xac",2)), // ¬
	StrPair("shy",std::string("\xc2\xad",2)), // ­
	StrPair("reg",std::string("\xc2\xae",2)), // ®
	StrPair("macr",std::string("\xc2\xaf",2)), // ¯
	StrPair("deg",std::string("\xc2\xb0",2)), // °
	StrPair("plusmn",std::string("\xc2\xb1",2)), // ±
	StrPair("sup2",std::string("\xc2\xb2",2)), // ²
	StrPair("sup3",std::string("\xc2\xb3",2)), // ³
	StrPair("acute",std::string("\xc2\xb4",2)), // ´
	StrPair("micro",std::string("\xc2\xb5",2)), // µ
	StrPair("para",std::string("\xc2\xb6",2)), // ¶
	StrPair("middot",std::string("\xc2\xb7",2)), // ·
	StrPair("cedil",std::string("\xc2\xb8",2)), // ¸
	StrPair("sup1",std::string("\xc2\xb9",2)), // ¹
	StrPair("ordm",std::string("\xc2\xba",2)), // º
	StrPair("raquo",std::string("\xc2\xbb",2)), // »
	StrPair("frac14",std::string("\xc2\xbc",2)), // ¼
	StrPair("frac12",std::string("\xc2\xbd",2)), // ½
	StrPair("frac34",std::string("\xc2\xbe",2)), // ¾
	StrPair("iquest",std::string("\xc2\xbf",2)), // ¿
	StrPair("Agrave",std::string("\xc3\x80",2)), // À
	StrPair("Aacute",std::string("\xc3\x81",2)), // Á
	StrPair("Acirc",std::string("\xc3\x82",2)), // Â
	StrPair("Atilde",std::string("\xc3\x83",2)), // Ã
	StrPair("Auml",std::string("\xc3\x84",2)), // Ä
	StrPair("Aring",std::string("\xc3\x85",2)), // Å
	StrPair("AElig",std::string("\xc3\x86",2)), // Æ
	StrPair("Ccedil",std::string("\xc3\x87",2)), // Ç
	StrPair("Egrave",std::string("\xc3\x88",2)), // È
	StrPair("Eacute",std::string("\xc3\x89",2)), // É
	StrPair("Ecirc",std::string("\xc3\x8a",2)), // Ê
	StrPair("Euml",std::string("\xc3\x8b",2)), // Ë
	StrPair("Igrave",std::string("\xc3\x8c",2)), // Ì
	StrPair("Iacute",std::string("\xc3\x8d",2)), // Í
	StrPair("Icirc",std::string("\xc3\x8e",2)), // Î
	StrPair("Iuml",std::string("\xc3\x8f",2)), // Ï
	StrPair("ETH",std::string("\xc3\x90",2)), // Ð
	StrPair("Ntilde",std::string("\xc3\x91",2)), // Ñ
	StrPair("Ograve",std::string("\xc3\x92",2)), // Ò
	StrPair("Oacute",std::string("\xc3\x93",2)), // Ó
	StrPair("Ocirc",std::string("\xc3\x94",2)), // Ô
	StrPair("Otilde",std::string("\xc3\x95",2)), // Õ
	StrPair("Ouml",std::string("\xc3\x96",2)), // Ö
	StrPair("times",std::string("\xc3\x97",2)), // ×
	StrPair("Oslash",std::string("\xc3\x98",2)), // Ø
	StrPair("Ugrave",std::string("\xc3\x99",2)), // Ù
	StrPair("Uacute",std::string("\xc3\x9a",2)), // Ú
	StrPair("Ucirc",std::string("\xc3\x9b",2)), // Û
	StrPair("Uuml",std::string("\xc3\x9c",2)), // Ü
	StrPair("Yacute",std::string("\xc3\x9d",2)), // Ý
	StrPair("THORN",std::string("\xc3\x9e",2)), // Þ
	StrPair("szlig",std::string("\xc3\x9f",2)), // ß
	StrPair("agrave",std::string("\xc3\xa0",2)), // à
	StrPair("aacute",std::string("\xc3\xa1",2)), // á
	StrPair("acirc",std::string("\xc3\xa2",2)), // â
	StrPair("atilde",std::string("\xc3\xa3",2)), // ã
	StrPair("auml",std::string("\xc3\xa4",2)), // ä
	StrPair("aring",std::string("\xc3\xa5",2)), // å
	StrPair("aelig",std::string("\xc3\xa6",2)), // æ
	StrPair("ccedil",std::string("\xc3\xa7",2)), // ç
	StrPair("egrave",std::string("\xc3\xa8",2)), // è
	StrPair("eacute",std::string("\xc3\xa9",2)), // é
	StrPair("ecirc",std::string("\xc3\xaa",2)), // ê
	StrPair("euml",std::string("\xc3\xab",2)), // ë
	StrPair("igrave",std::string("\xc3\xac",2)), // ì
	StrPair("iacute",std::string("\xc3\xad",2)), // í
	StrPair("icirc",std::string("\xc3\xae",2)), // î
	StrPair("iuml",std::string("\xc3\xaf",2)), // ï
	StrPair("eth",std::string("\xc3\xb0",2)), // ð
	StrPair("ntilde",std::string("\xc3\xb1",2)), // ñ
	StrPair("ograve",std::string("\xc3\xb2",2)), // ò
	StrPair("oacute",std::string("\xc3\xb3",2)), // ó
	StrPair("ocirc",std::string("\xc3\xb4",2)), // ô
	StrPair("otilde",std::string("\xc3\xb5",2)), // õ
	StrPair("ouml",std::string("\xc3\xb6",2)), // ö
	StrPair("divide",std::string("\xc3\xb7",2)), // ÷
	StrPair("oslash",std::string("\xc3\xb8",2)), // ø
	StrPair("ugrave",std::string("\xc3\xb9",2)), // ù
	StrPair("uacute",std::string("\xc3\xba",2)), // ú
	StrPair("ucirc",std::string("\xc3\xbb",2)), // û
	StrPair("uuml",std::string("\xc3\xbc",2)), // ü
	StrPair("yacute",std::string("\xc3\xbd",2)), // ý
	StrPair("thorn",std::string("\xc3\xbe",2)), // þ
	StrPair("yuml",std::string("\xc3\xbf",2)), // ÿ
	StrPair("fnof",std::string("\xc6\x92",2)), // ƒ
	StrPair("Alpha",std::string("\xce\x91",2)), // Α
	StrPair("Beta",std::string("\xce\x92",2)), // Β
	StrPair("Gamma",std::string("\xce\x93",2)), // Γ
	StrPair("Delta",std::string("\xce\x94",2)), // Δ
	StrPair("Epsilon",std::string("\xce\x95",2)), // Ε
	StrPair("Zeta",std::string("\xce\x96",2)), // Ζ
	StrPair("Eta",std::string("\xce\x97",2)), // Η
	StrPair("Theta",std::string("\xce\x98",2)), // Θ
	StrPair("Iota",std::string("\xce\x99",2)), // Ι
	StrPair("Kappa",std::string("\xce\x9a",2)), // Κ
	StrPair("Lambda",std::string("\xce\x9b",2)), // Λ
	StrPair("Mu",std::string("\xce\x9c",2)), // Μ
	StrPair("Nu",std::string("\xce\x9d",2)), // Ν
	StrPair("Xi",std::string("\xce\x9e",2)), // Ξ
	StrPair("Omicron",std::string("\xce\x9f",2)), // Ο
	StrPair("Pi",std::string("\xce\xa0",2)), // Π
	StrPair("Rho",std::string("\xce\xa1",2)), // Ρ
	StrPair("Sigma",std::string("\xce\xa3",2)), // Σ
	StrPair("Tau",std::string("\xce\xa4",2)), // Τ
	StrPair("Upsilon",std::string("\xce\xa5",2)), // Υ
	StrPair("Phi",std::string("\xce\xa6",2)), // Φ
	StrPair("Chi",std::string("\xce\xa7",2)), // Χ
	StrPair("Psi",std::string("\xce\xa8",2)), // Ψ
	StrPair("Omega",std::string("\xce\xa9",2)), // Ω
	StrPair("alpha",std::string("\xce\xb1",2)), // α
	StrPair("beta",std::string("\xce\xb2",2)), // β
	StrPair("gamma",std::string("\xce\xb3",2)), // γ
	StrPair("delta",std::string("\xce\xb4",2)), // δ
	StrPair("epsilon",std::string("\xce\xb5",2)), // ε
	StrPair("zeta",std::string("\xce\xb6",2)), // ζ
	StrPair("eta",std::string("\xce\xb7",2)), // η
	StrPair("theta",std::string("\xce\xb8",2)), // θ
	StrPair("iota",std::string("\xce\xb9",2)), // ι
	StrPair("kappa",std::string("\xce\xba",2)), // κ
	StrPair("lambda",std::string("\xce\xbb",2)), // λ
	StrPair("mu",std::string("\xce\xbc",2)), // μ
	StrPair("nu",std::string("\xce\xbd",2)), // ν
	StrPair("xi",std::string("\xce\xbe",2)), // ξ
	StrPair("omicron",std::string("\xce\xbf",2)), // ο
	StrPair("pi",std::string("\xcf\x80",2)), // π
	StrPair("rho",std::string("\xcf\x81",2)), // ρ
	StrPair("sigmaf",std::string("\xcf\x82",2)), // ς
	StrPair("sigma",std::string("\xcf\x83",2)), // σ
	StrPair("tau",std::string("\xcf\x84",2)), // τ
	StrPair("upsilon",std::string("\xcf\x85",2)), // υ
	StrPair("phi",std::string("\xcf\x86",2)), // φ
	StrPair("chi",std::string("\xcf\x87",2)), // χ
	StrPair("psi",std::string("\xcf\x88",2)), // ψ
	StrPair("omega",std::string("\xcf\x89",2)), // ω
	StrPair("thetasym",std::string("\xcf\x91",2)), // ϑ
	StrPair("upsih",std::string("\xcf\x92",2)), // ϒ
	StrPair("piv",std::string("\xcf\x96",2)), // ϖ
	StrPair("bull",std::string("\xe2\x80\xa2",3)), // •
	StrPair("hellip",std::string("\xe2\x80\xa6",3)), // …
	StrPair("prime",std::string("\xe2\x80\xb2",3)), // ′
	StrPair("Prime",std::string("\xe2\x80\xb3",3)), // ″
	StrPair("oline",std::string("\xe2\x80\xbe",3)), // ‾
	StrPair("frasl",std::string("\xe2\x81\x84",3)), // ⁄
	StrPair("weierp",std::string("\xe2\x84\x98",3)), // ℘
	StrPair("image",std::string("\xe2\x84\x91",3)), // ℑ
	StrPair("real",std::string("\xe2\x84\x9c",3)), // ℜ
	StrPair("trade",std::string("\xe2\x84\xa2",3)), // ™
	StrPair("alefsym",std::string("\xe2\x84\xb5",3)), // ℵ
	StrPair("larr",std::string("\xe2\x86\x90",3)), // ←
	StrPair("uarr",std::string("\xe2\x86\x91",3)), // ↑
	StrPair("rarr",std::string("\xe2\x86\x92",3)), // →
	StrPair("darr",std::string("\xe2\x86\x93",3)), // ↓
	StrPair("harr",std::string("\xe2\x86\x94",3)), // ↔
	StrPair("crarr",std::string("\xe2\x86\xb5",3)), // ↵
	StrPair("lArr",std::string("\xe2\x87\x90",3)), // ⇐
	StrPair("uArr",std::string("\xe2\x87\x91",3)), // ⇑
	StrPair("rArr",std::string("\xe2\x87\x92",3)), // ⇒
	StrPair("dArr",std::string("\xe2\x87\x93",3)), // ⇓
	StrPair("hArr",std::string("\xe2\x87\x94",3)), // ⇔
	StrPair("forall",std::string("\xe2\x88\x80",3)), // ∀
	StrPair("part",std::string("\xe2\x88\x82",3)), // ∂
	StrPair("exist",std::string("\xe2\x88\x83",3)), // ∃
	StrPair("empty",std::string("\xe2\x88\x85",3)), // ∅
	StrPair("nabla",std::string("\xe2\x88\x87",3)), // ∇
	StrPair("isin",std::string("\xe2\x88\x88",3)), // ∈
	StrPair("notin",std::string("\xe2\x88\x89",3)), // ∉
	StrPair("ni",std::string("\xe2\x88\x8b",3)), // ∋
	StrPair("prod",std::string("\xe2\x88\x8f",3)), // ∏
	StrPair("sum",std::string("\xe2\x88\x91",3)), // ∑
	StrPair("minus",std::string("\xe2\x88\x92",3)), // −
	StrPair("lowast",std::string("\xe2\x88\x97",3)), // ∗
	StrPair("radic",std::string("\xe2\x88\x9a",3)), // √
	StrPair("prop",std::string("\xe2\x88\x9d",3)), // ∝
	StrPair("infin",std::string("\xe2\x88\x9e",3)), // ∞
	StrPair("ang",std::string("\xe2\x88\xa0",3)), // ∠
	StrPair("and",std::string("\xe2\x88\xa7",3)), // ∧
	StrPair("or",std::string("\xe2\x88\xa8",3)), // ∨
	StrPair("cap",std::string("\xe2\x88\xa9",3)), // ∩
	StrPair("cup",std::string("\xe2\x88\xaa",3)), // ∪
	StrPair("int",std::string("\xe2\x88\xab",3)), // ∫
	StrPair("there4",std::string("\xe2\x88\xb4",3)), // ∴
	StrPair("sim",std::string("\xe2\x88\xbc",3)), // ∼
	StrPair("cong",std::string("\xe2\x89\x85",3)), // ≅
	StrPair("asymp",std::string("\xe2\x89\x88",3)), // ≈
	StrPair("ne",std::string("\xe2\x89\xa0",3)), // ≠
	StrPair("equiv",std::string("\xe2\x89\xa1",3)), // ≡
	StrPair("le",std::string("\xe2\x89\xa4",3)), // ≤
	StrPair("ge",std::string("\xe2\x89\xa5",3)), // ≥
	StrPair("sub",std::string("\xe2\x8a\x82",3)), // ⊂
	StrPair("sup",std::string("\xe2\x8a\x83",3)), // ⊃
	StrPair("nsub",std::string("\xe2\x8a\x84",3)), // ⊄
	StrPair("sube",std::string("\xe2\x8a\x86",3)), // ⊆
	StrPair("supe",std::string("\xe2\x8a\x87",3)), // ⊇
	StrPair("oplus",std::string("\xe2\x8a\x95",3)), // ⊕
	StrPair("otimes",std::string("\xe2\x8a\x97",3)), // ⊗
	StrPair("perp",std::string("\xe2\x8a\xa5",3)), // ⊥
	StrPair("sdot",std::string("\xe2\x8b\x85",3)), // ⋅
	StrPair("lceil",std::string("\xe2\x8c\x88",3)), // ⌈
	StrPair("rceil",std::string("\xe2\x8c\x89",3)), // ⌉
	StrPair("lfloor",std::string("\xe2\x8c\x8a",3)), // ⌊
	StrPair("rfloor",std::string("\xe2\x8c\x8b",3)), // ⌋
	StrPair("lang",std::string("\xe2\x8c\xa9",3)), // 〈
	StrPair("rang",std::string("\xe2\x8c\xaa",3)), // 〉
	StrPair("loz",std::string("\xe2\x97\x8a",3)), // ◊
	StrPair("spades",std::string("\xe2\x99\xa0",3)), // ♠
	StrPair("clubs",std::string("\xe2\x99\xa3",3)), // ♣
	StrPair("hearts",std::string("\xe2\x99\xa5",3)), // ♥
	StrPair("diams",std::string("\xe2\x99\xa6",3)), // ♦
	StrPair("quot",std::string("\x22",1)), // "
	StrPair("amp",std::string("\x26",1)), // &
	StrPair("lt",std::string("\x3c",1)), // <
	StrPair("gt",std::string("\x3e",1)), // >
	StrPair("OElig",std::string("\xc5\x92",2)), // Œ
	StrPair("oelig",std::string("\xc5\x93",2)), // œ
	StrPair("Scaron",std::string("\xc5\xa0",2)), // Š
	StrPair("scaron",std::string("\xc5\xa1",2)), // š
	StrPair("Yuml",std::string("\xc5\xb8",2)), // Ÿ
	StrPair("circ",std::string("\xcb\x86",2)), // ˆ
	StrPair("tilde",std::string("\xcb\x9c",2)), // ˜
	StrPair("ensp",std::string("\xe2\x80\x82",3)), //  
	StrPair("emsp",std::string("\xe2\x80\x83",3)), //  
	StrPair("thinsp",std::string("\xe2\x80\x89",3)), //  
	StrPair("zwnj",std::string("\xe2\x80\x8c",3)), // ‌
	StrPair("zwj",std::string("\xe2\x80\x8d",3)), // ‍
	StrPair("lrm",std::string("\xe2\x80\x8e",3)), // ‎
	StrPair("rlm",std::string("\xe2\x80\x8f",3)), // ‏
	StrPair("ndash",std::string("\xe2\x80\x93",3)), // –
	StrPair("mdash",std::string("\xe2\x80\x94",3)), // —
	StrPair("lsquo",std::string("\xe2\x80\x98",3)), // ‘
	StrPair("rsquo",std::string("\xe2\x80\x99",3)), // ’
	StrPair("sbquo",std::string("\xe2\x80\x9a",3)), // ‚
	StrPair("ldquo",std::string("\xe2\x80\x9c",3)), // “
	StrPair("rdquo",std::string("\xe2\x80\x9d",3)), // ”
	StrPair("bdquo",std::string("\xe2\x80\x9e",3)), // „
	StrPair("dagger",std::string("\xe2\x80\xa0",3)), // †
	StrPair("Dagger",std::string("\xe2\x80\xa1",3)), // ‡
	StrPair("permil",std::string("\xe2\x80\xb0",3)), // ‰
	StrPair("lsaquo",std::string("\xe2\x80\xb9",3)), // ‹
	StrPair("rsaquo",std::string("\xe2\x80\xba",3)), // ›
	StrPair("euro",std::string("\xe2\x82\xac",3)), // €
};

StrPair _html_entity_alphanum_list[] =  {
	StrPair("nbsp",std::string(" ")), //  
	StrPair("iexcl",std::string(" ")), // ¡
	StrPair("cent",std::string(" ")), // ¢
	StrPair("pound",std::string(" ")), // £
	StrPair("curren",std::string(" ")), // ¤
	StrPair("yen",std::string(" ")), // ¥
	StrPair("brvbar",std::string(" ")), // ¦
	StrPair("sect",std::string(" ")), // §
	StrPair("uml",std::string(" ")), // ¨
	StrPair("copy",std::string(" ")), // ©
	StrPair("ordf",std::string(" ")), // ª
	StrPair("laquo",std::string(" ")), // «
	StrPair("not",std::string(" ")), // ¬
	StrPair("shy",std::string(" ")), // ­
	StrPair("reg",std::string(" ")), // ®
	StrPair("macr",std::string(" ")), // ¯
	StrPair("deg",std::string(" ")), // °
	StrPair("plusmn",std::string(" ")), // ±
	StrPair("sup2",std::string(" ")), // ²
	StrPair("sup3",std::string(" ")), // ³
	StrPair("acute",std::string(" ")), // ´
	StrPair("micro",std::string(" ")), // µ
	StrPair("para",std::string(" ")), // ¶
	StrPair("middot",std::string(" ")), // ·
	StrPair("cedil",std::string(" ")), // ¸
	StrPair("sup1",std::string(" ")), // ¹
	StrPair("ordm",std::string(" ")), // º
	StrPair("raquo",std::string(" ")), // »
	StrPair("frac14",std::string(" ")), // ¼
	StrPair("frac12",std::string(" ")), // ½
	StrPair("frac34",std::string(" ")), // ¾
	StrPair("iquest",std::string(" ")), // ¿
	StrPair("divide",std::string(" ")), // ÷
	StrPair("Agrave",std::string("\xc3\x80",2)), // À
	StrPair("Aacute",std::string("\xc3\x81",2)), // Á
	StrPair("Acirc",std::string("\xc3\x82",2)), // Â
	StrPair("Atilde",std::string("\xc3\x83",2)), // Ã
	StrPair("Auml",std::string("\xc3\x84",2)), // Ä
	StrPair("Aring",std::string("\xc3\x85",2)), // Å
	StrPair("AElig",std::string("\xc3\x86",2)), // Æ
	StrPair("Ccedil",std::string("\xc3\x87",2)), // Ç
	StrPair("Egrave",std::string("\xc3\x88",2)), // È
	StrPair("Eacute",std::string("\xc3\x89",2)), // É
	StrPair("Ecirc",std::string("\xc3\x8a",2)), // Ê
	StrPair("Euml",std::string("\xc3\x8b",2)), // Ë
	StrPair("Igrave",std::string("\xc3\x8c",2)), // Ì
	StrPair("Iacute",std::string("\xc3\x8d",2)), // Í
	StrPair("Icirc",std::string("\xc3\x8e",2)), // Î
	StrPair("Iuml",std::string("\xc3\x8f",2)), // Ï
	StrPair("ETH",std::string("\xc3\x90",2)), // Ð
	StrPair("Ntilde",std::string("\xc3\x91",2)), // Ñ
	StrPair("Ograve",std::string("\xc3\x92",2)), // Ò
	StrPair("Oacute",std::string("\xc3\x93",2)), // Ó
	StrPair("Ocirc",std::string("\xc3\x94",2)), // Ô
	StrPair("Otilde",std::string("\xc3\x95",2)), // Õ
	StrPair("Ouml",std::string("\xc3\x96",2)), // Ö
	StrPair("times",std::string("\xc3\x97",2)), // ×
	StrPair("Oslash",std::string("\xc3\x98",2)), // Ø
	StrPair("Ugrave",std::string("\xc3\x99",2)), // Ù
	StrPair("Uacute",std::string("\xc3\x9a",2)), // Ú
	StrPair("Ucirc",std::string("\xc3\x9b",2)), // Û
	StrPair("Uuml",std::string("\xc3\x9c",2)), // Ü
	StrPair("Yacute",std::string("\xc3\x9d",2)), // Ý
	StrPair("THORN",std::string("\xc3\x9e",2)), // Þ
	StrPair("szlig",std::string("\xc3\x9f",2)), // ß
	StrPair("agrave",std::string("\xc3\xa0",2)), // à
	StrPair("aacute",std::string("\xc3\xa1",2)), // á
	StrPair("acirc",std::string("\xc3\xa2",2)), // â
	StrPair("atilde",std::string("\xc3\xa3",2)), // ã
	StrPair("auml",std::string("\xc3\xa4",2)), // ä
	StrPair("aring",std::string("\xc3\xa5",2)), // å
	StrPair("aelig",std::string("\xc3\xa6",2)), // æ
	StrPair("ccedil",std::string("\xc3\xa7",2)), // ç
	StrPair("egrave",std::string("\xc3\xa8",2)), // è
	StrPair("eacute",std::string("\xc3\xa9",2)), // é
	StrPair("ecirc",std::string("\xc3\xaa",2)), // ê
	StrPair("euml",std::string("\xc3\xab",2)), // ë
	StrPair("igrave",std::string("\xc3\xac",2)), // ì
	StrPair("iacute",std::string("\xc3\xad",2)), // í
	StrPair("icirc",std::string("\xc3\xae",2)), // î
	StrPair("iuml",std::string("\xc3\xaf",2)), // ï
	StrPair("eth",std::string("\xc3\xb0",2)), // ð
	StrPair("ntilde",std::string("\xc3\xb1",2)), // ñ
	StrPair("ograve",std::string("\xc3\xb2",2)), // ò
	StrPair("oacute",std::string("\xc3\xb3",2)), // ó
	StrPair("ocirc",std::string("\xc3\xb4",2)), // ô
	StrPair("otilde",std::string("\xc3\xb5",2)), // õ
	StrPair("ouml",std::string("\xc3\xb6",2)), // ö
	StrPair("oslash",std::string("\xc3\xb8",2)), // ø
	StrPair("ugrave",std::string("\xc3\xb9",2)), // ù
	StrPair("uacute",std::string("\xc3\xba",2)), // ú
	StrPair("ucirc",std::string("\xc3\xbb",2)), // û
	StrPair("uuml",std::string("\xc3\xbc",2)), // ü
	StrPair("yacute",std::string("\xc3\xbd",2)), // ý
	StrPair("thorn",std::string("\xc3\xbe",2)), // þ
	StrPair("yuml",std::string("\xc3\xbf",2)), // ÿ
	StrPair("fnof",std::string(" ")), // ƒ
	StrPair("Alpha",std::string(" ")), // Α
	StrPair("Beta",std::string(" ")), // Β
	StrPair("Gamma",std::string(" ")), // Γ
	StrPair("Delta",std::string(" ")), // Δ
	StrPair("Epsilon",std::string(" ")), // Ε
	StrPair("Zeta",std::string(" ")), // Ζ
	StrPair("Eta",std::string(" ")), // Η
	StrPair("Theta",std::string(" ")), // Θ
	StrPair("Iota",std::string(" ")), // Ι
	StrPair("Kappa",std::string(" ")), // Κ
	StrPair("Lambda",std::string(" ")), // Λ
	StrPair("Mu",std::string(" ")), // Μ
	StrPair("Nu",std::string(" ")), // Ν
	StrPair("Xi",std::string(" ")), // Ξ
	StrPair("Omicron",std::string(" ")), // Ο
	StrPair("Pi",std::string(" ")), // Π
	StrPair("Rho",std::string(" ")), // Ρ
	StrPair("Sigma",std::string(" ")), // Σ
	StrPair("Tau",std::string(" ")), // Τ
	StrPair("Upsilon",std::string(" ")), // Υ
	StrPair("Phi",std::string(" ")), // Φ
	StrPair("Chi",std::string(" ")), // Χ
	StrPair("Psi",std::string(" ")), // Ψ
	StrPair("Omega",std::string(" ")), // Ω
	StrPair("alpha",std::string(" ")), // α
	StrPair("beta",std::string(" ")), // β
	StrPair("gamma",std::string(" ")), // γ
	StrPair("delta",std::string(" ")), // δ
	StrPair("epsilon",std::string(" ")), // ε
	StrPair("zeta",std::string(" ")), // ζ
	StrPair("eta",std::string(" ")), // η
	StrPair("theta",std::string(" ")), // θ
	StrPair("iota",std::string(" ")), // ι
	StrPair("kappa",std::string(" ")), // κ
	StrPair("lambda",std::string(" ")), // λ
	StrPair("mu",std::string(" ")), // μ
	StrPair("nu",std::string(" ")), // ν
	StrPair("xi",std::string(" ")), // ξ
	StrPair("omicron",std::string(" ")), // ο
	StrPair("pi",std::string(" ")), // π
	StrPair("rho",std::string(" ")), // ρ
	StrPair("sigmaf",std::string(" ")), // ς
	StrPair("sigma",std::string(" ")), // σ
	StrPair("tau",std::string(" ")), // τ
	StrPair("upsilon",std::string(" ")), // υ
	StrPair("phi",std::string(" ")), // φ
	StrPair("chi",std::string(" ")), // χ
	StrPair("psi",std::string(" ")), // ψ
	StrPair("omega",std::string(" ")), // ω
	StrPair("thetasym",std::string(" ")), // ϑ
	StrPair("upsih",std::string(" ")), // ϒ
	StrPair("piv",std::string(" ")), // ϖ
	StrPair("bull",std::string(" ")), // •
	StrPair("hellip",std::string(" ")), // …
	StrPair("prime",std::string(" ")), // ′
	StrPair("Prime",std::string(" ")), // ″
	StrPair("oline",std::string(" ")), // ‾
	StrPair("frasl",std::string(" ")), // ⁄
	StrPair("weierp",std::string(" ")), // ℘
	StrPair("image",std::string(" ")), // ℑ
	StrPair("real",std::string(" ")), // ℜ
	StrPair("trade",std::string(" ")), // ™
	StrPair("alefsym",std::string(" ")), // ℵ
	StrPair("larr",std::string(" ")), // ←
	StrPair("uarr",std::string(" ")), // ↑
	StrPair("rarr",std::string(" ")), // →
	StrPair("darr",std::string(" ")), // ↓
	StrPair("harr",std::string(" ")), // ↔
	StrPair("crarr",std::string(" ")), // ↵
	StrPair("lArr",std::string(" ")), // ⇐
	StrPair("uArr",std::string(" ")), // ⇑
	StrPair("rArr",std::string(" ")), // ⇒
	StrPair("dArr",std::string(" ")), // ⇓
	StrPair("hArr",std::string(" ")), // ⇔
	StrPair("forall",std::string(" ")), // ∀
	StrPair("part",std::string(" ")), // ∂
	StrPair("exist",std::string(" ")), // ∃
	StrPair("empty",std::string(" ")), // ∅
	StrPair("nabla",std::string(" ")), // ∇
	StrPair("isin",std::string(" ")), // ∈
	StrPair("notin",std::string(" ")), // ∉
	StrPair("ni",std::string(" ")), // ∋
	StrPair("prod",std::string(" ")), // ∏
	StrPair("sum",std::string(" ")), // ∑
	StrPair("minus",std::string(" ")), // −
	StrPair("lowast",std::string(" ")), // ∗
	StrPair("radic",std::string(" ")), // √
	StrPair("prop",std::string(" ")), // ∝
	StrPair("infin",std::string(" ")), // ∞
	StrPair("ang",std::string(" ")), // ∠
	StrPair("and",std::string(" ")), // ∧
	StrPair("or",std::string(" ")), // ∨
	StrPair("cap",std::string(" ")), // ∩
	StrPair("cup",std::string(" ")), // ∪
	StrPair("int",std::string(" ")), // ∫
	StrPair("there4",std::string(" ")), // ∴
	StrPair("sim",std::string(" ")), // ∼
	StrPair("cong",std::string(" ")), // ≅
	StrPair("asymp",std::string(" ")), // ≈
	StrPair("ne",std::string(" ")), // ≠
	StrPair("equiv",std::string(" ")), // ≡
	StrPair("le",std::string(" ")), // ≤
	StrPair("ge",std::string(" ")), // ≥
	StrPair("sub",std::string(" ")), // ⊂
	StrPair("sup",std::string(" ")), // ⊃
	StrPair("nsub",std::string(" ")), // ⊄
	StrPair("sube",std::string(" ")), // ⊆
	StrPair("supe",std::string(" ")), // ⊇
	StrPair("oplus",std::string(" ")), // ⊕
	StrPair("otimes",std::string(" ")), // ⊗
	StrPair("perp",std::string(" ")), // ⊥
	StrPair("sdot",std::string(" ")), // ⋅
	StrPair("lceil",std::string(" ")), // ⌈
	StrPair("rceil",std::string(" ")), // ⌉
	StrPair("lfloor",std::string(" ")), // ⌊
	StrPair("rfloor",std::string(" ")), // ⌋
	StrPair("lang",std::string(" ")), // 〈
	StrPair("rang",std::string(" ")), // 〉
	StrPair("loz",std::string(" ")), // ◊
	StrPair("spades",std::string(" ")), // ♠
	StrPair("clubs",std::string(" ")), // ♣
	StrPair("hearts",std::string(" ")), // ♥
	StrPair("diams",std::string(" ")), // ♦
	StrPair("quot",std::string("\x22",1)), // "
	StrPair("amp",std::string("\x26",1)), // &
	StrPair("lt",std::string("\x3c",1)), // <
	StrPair("gt",std::string("\x3e",1)), // >
	StrPair("OElig",std::string("\xc5\x92",2)), // Œ
	StrPair("oelig",std::string("\xc5\x93",2)), // œ
	StrPair("Scaron",std::string("\xc5\xa0",2)), // Š
	StrPair("scaron",std::string("\xc5\xa1",2)), // š
	StrPair("Yuml",std::string("\xc5\xb8",2)), // Ÿ
	StrPair("circ",std::string("\xcb\x86",2)), // ˆ
	StrPair("tilde",std::string("\xcb\x9c",2)), // ˜
	StrPair("ensp",std::string(" ")), //  
	StrPair("emsp",std::string(" ")), //  
	StrPair("thinsp",std::string(" ")), //  
	StrPair("zwnj",std::string(" ")), // ‌
	StrPair("zwj",std::string(" ")), // ‍
	StrPair("lrm",std::string(" ")), // ‎
	StrPair("rlm",std::string(" ")), // ‏
	StrPair("ndash",std::string(" ")), // –
	StrPair("mdash",std::string(" ")), // —
	StrPair("lsquo",std::string(" ")), // ‘
	StrPair("rsquo",std::string(" ")), // ’
	StrPair("sbquo",std::string(" ")), // ‚
	StrPair("ldquo",std::string(" ")), // “
	StrPair("rdquo",std::string(" ")), // ”
	StrPair("bdquo",std::string(" ")), // „
	StrPair("dagger",std::string(" ")), // †
	StrPair("Dagger",std::string(" ")), // ‡
	StrPair("permil",std::string(" ")), // ‰
	StrPair("lsaquo",std::string(" ")), // ‹
	StrPair("rsaquo",std::string(" ")), // ›
	StrPair("euro",std::string(" ")), // €
};


int _html_entity_list_len = sizeof(_html_entity_list)/sizeof(StrPair);

int _html_entity_alphanum_list_len =
	sizeof(_html_entity_alphanum_list)/ sizeof(StrPair);

typedef __gnu_cxx::hash_map< std::string, std::string, std::tr1::hash<std::string> > StrStrHashMap;

const StrStrHashMap html_entity_map(	_html_entity_list,
				 _html_entity_list + _html_entity_list_len);


const StrStrHashMap html_entity_alphanum_map(	_html_entity_alphanum_list,
		_html_entity_alphanum_list + _html_entity_alphanum_list_len);




/* ********************************************************************** *
			       Base Entity Parser
 * ********************************************************************** */

void BaseEntityParser::parsePureText()
{
	filebuf text = readUntilDelimiter("&");
	handlePureText(text);
}

void BaseEntityParser::parseReference()
{
	// We may need to go back if things go wrong...
	filebuf previous_start(text);
	try {
		consumeToken("&");
		checkForEOF(); // There should be more characters to read

		if ( *text == '#' ) {
			parseCharacterReference();
		} else if ( is_in(*text, LETTERS) ) {
			parseEntityReference();
		} else {
			// WTF? What is this thing?
			throw InvalidCharError("Unexpected character while "
					       "parsing reference.");
		}

		// Consume trailing ';' if it is there...
		if ( not text.eof() and *text == ';') {
			consumeToken(';');
		}

	// Why Oh! Why doesn't C++ has multiple catch blocks...
	} catch (InvalidCharError&) {
		// Invalid or unexpected character found?
		// Just turn everything we had from previous_start to
		// the current read position into text content.
		int length = this->text.current - previous_start.current;
		handlePureText(filebuf(previous_start.current, length ));
	} catch (UnknownEntityReferenceError&) {
		// Unknown entity?
		// Just turn everything we had from previous_start to
		// the current read position into text content.
		int length = this->text.current - previous_start.current;
		handlePureText(filebuf(previous_start.current, length ));
	} catch (ParserEOFError&) {
		// Unexpected EOF?
		// Just turn everything we had from previous_start to
		// to this->text's end into text content.
		handlePureText(previous_start);
	}
}

void BaseEntityParser::parseEntityReference()
{
	StrStrHashMap::const_iterator entity_value;
	const char* start = text.current;
        int length = 0;

	// '&' was already read. Get the identifier...
	// Find the end of this name
        while ( (not text.eof()) && is_in(*text, ALPHANUM) ){
                ++text;
                ++length;
        }

	// Empty Empty Refrence?
	if ( length == 0 ){
		// In theory, we should NEVER reach this point,
		// since we got here because he had read an LETTER
		// in first place!
		throw InvalidCharError("Invalid char while reading Entity "
				"Identifier.");
	}

	std::string identifier(start, length);

	// Is this a known identifier?
	entity_value = entity_map.find(identifier);
        if ( entity_value == entity_map.end() ) {
            throw UnknownEntityReferenceError(
				std::string("Unknown entity identifier '") +
				identifier + "'");
	}
	
	// Horay!
	handleEntityText(entity_value->second);

}

void BaseEntityParser::parseCharacterReference()
{
	unsigned int uni_val = 0;

	// '&' was already read,
        //'#' is in the current position 
        consumeToken('#');
        // There MUST be something after this token
        checkForEOF();

	if (*text == 'x') {
		// hexa number
		consumeToken('x');
		uni_val = readNumber(true);
	} else {
		// commom (decimal) number
		uni_val = readNumber();
	}

        // Convert number into unicode
        handleEntityText(unichr(uni_val));
}


unsigned int BaseEntityParser::readNumber(bool hex)
{
	const char* start = text.current;
        int length = 0;
	std::istringstream conversor;
	unsigned int number;
	
	// Default is to read decimal numbers
	std::string ALLOWED_CHARS = DIGITS;

	// base setup
        if ( hex ) {
		conversor >> std::hex;
		ALLOWED_CHARS = HEXDIGITS;
	}

	// Find the end of this number
        while ( (not text.eof()) && is_in(*text, ALLOWED_CHARS) ){
                ++text;
                ++length;
        }
        
        // Empty number?
	if ( length == 0 ){
		throw InvalidCharError("Invalid char while reading a number in "
				"a character reference.");
	}

	// Setup conversor with the value read;
	conversor.str( std::string(start, length) );
	conversor >> number; // No error checking.. let's assume it works...

        return number;
}


void BaseEntityParser::parse()
{
	while(! text.eof()){
		if (*text == '&') {
			parseReference();
		} else {
			parsePureText();
		}
	}
}

// EOF
