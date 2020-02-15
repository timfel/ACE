/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <fstream>
#include "common/logging.h"
#include "common/glyph_set.h"
#include "common/fs.h"
#include "common/json.h"
#include "common/utf8.h"

static const std::string s_szDefaultCharset = (
	" ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
);

enum class tOutType: uint8_t {
	INVALID,
	TTF,
	DIR,
	PNG,
	FNT,
};

std::map<std::string, tOutType> s_mOutType = {
	{"ttf", tOutType::TTF},
	{"dir", tOutType::DIR},
	{"png", tOutType::PNG},
	{"fnt", tOutType::FNT}
};

void printUsage(const std::string &szAppName) {
	using fmt::print;
	print("Usage:\n\t{} inPath outType [extraOpts]\n\n", szAppName);
	print("inPath\t- path to TTF file or directory with PNG glyphs\n");
	print("outType\t- one of following:\n");
	print("\tdir\tDirectory with each glyph as separate PNG\n");
	print("\tpng\tSingle PNG file with whole font\n");
	print("\tfnt\tACE font file\n");
	print("extraOpts:\n");
	// -chars
	print("\t-chars \"AB D\"\t\tInclude only 'A', 'B', ' ' and 'D' chars in font. Only for TTF input.\n");
	print("\t\t\t\tUse backslash (\\) to escape quote (\") char inside charset specifier\n");
	print("\t\t\t\tDefault charset is: \"{}\"\n\n", s_szDefaultCharset);
	// -charFile
	print("\t-charfile \"file.txt\"\tInclude chars specified in file.txt.\n");
	print("\t\t\t\tNewline chars (\\r, \\n) and repeats are omitted.\n\n");
	// -size
	print("\t-size 8\t\t\tRasterize font using size of 8pt. Default: 20.\n\n");
	// -out
	print("\t-out outPath\t\tSpecify output path, including file name.\n");
	print("\t\t\t\tDefault is same name as input with changed extension\n\n");
}

static uint32_t getCharCodeFromTok(const tJson *pJson, uint16_t uwTok) {
	uint32_t ulVal = 0;
	const auto &Token = pJson->pTokens[uwTok];
	if(Token.type == JSMN_STRING) {
		// read unicode char and return its codepoint
		uint32_t ulCodepoint, ulState = 0;

		for(auto i = Token.start; i <= Token.end; ++i) {
			auto CharCode = *reinterpret_cast<const uint8_t*>(&pJson->szData[i]);
			if (
				decode(&ulState, &ulCodepoint, CharCode) != UTF8_ACCEPT ||
				ulCodepoint == '\n' || ulCodepoint == '\r'
			) {
				continue;
			}
			ulVal = ulCodepoint;
			break;
		}
	}
	else {
		// read number as it is in file and return it
		ulVal = jsonTokToUlong(pJson, uwTok);
	}
	return ulVal;
}

int main(int lArgCount, const char *pArgs[])
{
	const uint8_t ubMandatoryArgCnt = 2;
	// Mandatory args
	if(lArgCount - 1 < ubMandatoryArgCnt) {
		nLog::error("Too few arguments, expected {}", ubMandatoryArgCnt);
		printUsage(pArgs[0]);
		return EXIT_FAILURE;
	}

	std::string szFontPath = pArgs[1];
	tOutType eOutType = s_mOutType[pArgs[2]];

	// Optional args' default values
	std::string szCharset = "";
	std::string szOutPath = "";
	std::string szRemapPath = "";
	int32_t lSize = -1;

	// Search for optional args
	for(auto i = ubMandatoryArgCnt+1; i < lArgCount; ++i) {
		if(pArgs[i] == std::string("-chars") && i < lArgCount - 1) {
			++i;
			szCharset += pArgs[i];
		}
		else if(pArgs[i] == std::string("-out") && i < lArgCount - 1) {
			++i;
			szOutPath = pArgs[i];
		}
		else if(pArgs[i] == std::string("-charFile") && i < lArgCount - 1) {
			++i;
			auto File = std::ifstream(pArgs[i], std::ios::binary);
			while(!File.eof()) {
				char c;
				File.read(&c, 1);
				szCharset += c;
			}
		}
		else if(pArgs[i] == std::string("-size") && i < lArgCount - 1) {
			++i;
			lSize = std::stol(pArgs[i]);
		}
		else if(pArgs[i] == std::string("-remap") && i < lArgCount - 1) {
			++i;
			szRemapPath = pArgs[i];
		}
		else {
			nLog::error("Unknown arg or missing value: '{}'", pArgs[i]);
			printUsage(pArgs[0]);
			return EXIT_FAILURE;
		}
	}

	if(szCharset.length() == 0) {
		szCharset = s_szDefaultCharset;
	}
	if(lSize < 0) {
		lSize = 20;
	}

	// Load glyphs from input file
	tGlyphSet Glyphs;
	tOutType eInType = tOutType::INVALID;
	if(szFontPath.find(".ttf") != std::string::npos) {
		Glyphs = tGlyphSet::fromTtf(szFontPath, lSize, szCharset, 128);
		eInType = tOutType::TTF;
	}
	else if(nFs::isDir(szFontPath)) {
		Glyphs = tGlyphSet::fromDir(szFontPath);
		if(!Glyphs.isOk()) {
			nLog::error("Loading glyphs from dir '{}' failed", szFontPath);
			return EXIT_FAILURE;
		}
		eInType = tOutType::DIR;
	}
	else {
		nLog::error("Unsupported font source: '{}'", pArgs[1]);
		return EXIT_FAILURE;
	}
	if(eInType == tOutType::INVALID || !Glyphs.isOk()) {
		nLog::error("Couldn't read any font glyphs");
		return EXIT_FAILURE;
	}

	// Remap chars accordingly
	if(!szRemapPath.empty()) {
		auto *pJson = jsonCreate(szRemapPath.c_str());
		if(pJson == nullptr) {
			nLog::error("Couldn't open remap file: '{}'", szRemapPath);
			return EXIT_FAILURE;
		}
		auto TokRemapArray = jsonGetDom(pJson, "remap");
		auto ElementCount = pJson->pTokens[TokRemapArray].size;
		std::vector<std::pair<uint32_t, uint32_t>> vFromTo;
		for(auto i = ElementCount; i--;) {
			auto TokRemapEntry = jsonGetElementInArray(pJson, TokRemapArray, i);
			auto TokFrom = jsonGetElementInArray(pJson, TokRemapEntry, 0);
			auto TokTo = jsonGetElementInArray(pJson, TokRemapEntry, 1);
			auto From = getCharCodeFromTok(pJson, TokFrom);
			auto To = getCharCodeFromTok(pJson, TokTo);
			fmt::print("Remapping {} => {}\n", From, To);
			vFromTo.push_back({std::move(From), std::move(To)});
		}
		Glyphs.remapGlyphs(vFromTo);
	}

	// Determine default output path
	if(szOutPath == "") {
		szOutPath = szFontPath;
		auto PosDot = szOutPath.find_last_of(".");
		if(PosDot != std::string::npos) {
			szOutPath = szOutPath.substr(0, PosDot);
		}
	}
	if(eInType == eOutType) {
		nLog::error("Output file type can't be same as input");
		return EXIT_FAILURE;
	}

	if(eOutType == tOutType::DIR) {
		if(szOutPath == szFontPath) {
			szOutPath += ".dir";
		}
		Glyphs.toDir(szOutPath);
	}
	else {
		tChunkyBitmap FontChunky = Glyphs.toPackedBitmap();
		if(eOutType == tOutType::PNG) {
			if(szOutPath.substr(szOutPath.length() - 4) != ".png") {
				szOutPath += ".png";
			}
			FontChunky.toPng(szOutPath);
		}
		else if(eOutType == tOutType::FNT) {
			tPlanarBitmap FontPlanar(FontChunky, tPalette({
				tRgb(0xFF), tRgb(0x00)
			}), tPalette());
			if(szOutPath.substr(szOutPath.length() - 4) != ".fnt") {
				szOutPath += ".fnt";
			}
			Glyphs.toAceFont(szOutPath);
		}
		else {
			nLog::error("Unsupported output type");
			return EXIT_FAILURE;
		}
	}
	fmt::print("All done!\n");
	return EXIT_SUCCESS;
}
