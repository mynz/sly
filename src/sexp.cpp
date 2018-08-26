#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

#define ASSERT assert

const char* ReadSexp(const char* filepath, int* outSize)
{
	FILE* fp = fopen(filepath, "rb");

	fseek(fp, 0, SEEK_END);
	int fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char* text = (char*)malloc(fsize+1);
	size_t nread = 0;
	do {
		nread += fread(text + nread, 1, fsize - nread, fp);
	} while ( nread != fsize );

	// 実際のファイルサイズよりも大きく確保してNull-Terminatedにする.
	text[fsize] = '\0';

	*outSize = fsize;
	return text;
}

static const int FIRST_RESERVED = 257;

enum TokenType {
	TK_INVALID /* = FIRST_RESERVED */ ,
	TK_PARBEG,
	TK_PAREND,
	TK_SYMBOL,
	TK_STRING,
	TK_NUMBER,
	TK_KEYWORD,
	TK_EOF,
};

const char* s_tokenLabels[] = {
	"TK_INVALID",
	"TK_PARBEG",
	"TK_PAREND",
	"TK_SYMBOL",
	"TK_STRING",
	"TK_NUMBER",
	"TK_KEYWORD",
	"TK_EOF",
};

static inline bool IsNewline(int c) {
	return c == '\n' || c == '\r';
}

struct InputStream
{
	const char* m_src;
	int m_size;

	int pos;

	InputStream() : m_src(0), m_size(0), pos(0)
	{
	}

	int Next() {
		ASSERT(pos <= m_size);
		return m_src[pos++];
	}
};

struct LexState {
	int cur;

	int nline;

	InputStream is;

	std::string buf;

public:

	LexState() : nline(1) { }

	void Init(const char* src, int nsrc) {
		is.m_src = src;
		is.m_size = nsrc;

		Next();
	}

	void Next() {
		cur = is.Next();
	}

	void SaveAndNext() {
		buf += cur;
		Next();
	}

	void ResetBuf() {
		buf.clear();
	}

	void IncLineNumber() {
		++nline;
	}

	void DebugPrint() {
		printf(" DBG: cur: [%c]=%x, pos: %d\n", cur, cur, is.pos);
	}
};

struct SemInfo {
	float m_num;
	std::string m_str;
};

static void ReadNumeric(LexState& ls, SemInfo& sem)
{
	int c = ls.cur;
	while ( isdigit(ls.cur) || ls.cur == '.' ) {
		ls.SaveAndNext();
	}

	sem.m_num = (float)atof(ls.buf.c_str()); 
}

static void ReadString(LexState& ls, int delimiter, SemInfo& sem)
{
	ls.SaveAndNext();

	while ( ls.cur != delimiter ) {

		ls.SaveAndNext();
	}

	ls.SaveAndNext();

	// 両端のクォートは除外.
	sem.m_str = ls.buf.substr(1, ls.buf.size() - 2);
}

TokenType GetToken(LexState& ls, SemInfo& sem)
{
	ls.ResetBuf();

	for (;;) {

		int c = ls.cur;
		char ch = (char)c; // XXX: for debugging
		if ( c == '\0' ) { return TK_EOF; }

		ls.DebugPrint();

		switch ( c ) {
			case '\n': case '\r':
				// line breaks 
				ls.IncLineNumber();
				ls.Next();
				break;

			case ' ':
			case '\t':
			case '\f':
			case '\v':
				// spaces
				ls.Next();
				break;
			case ';':
				// comment.
				while (!IsNewline(ls.cur) && ls.cur != '\0'  ) {
					ls.Next(); // skip until end of line(or end of file) 
				}
				break;
			case '(':
				ls.Next();
				return TK_PARBEG;
			case ')':
				ls.Next();
				return TK_PAREND;
				break;

			case '"': case '\'':
				ReadString(ls, c, sem);
				return TK_STRING;
				break;
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				ReadNumeric(ls, sem);
				return TK_NUMBER;
				break;

			case ':':
				ls.Next(); // truncate the prefix.
				while ( isalnum(ls.cur) || ls.cur == '_' || ls.cur == '-' ) {
					ls.SaveAndNext();
				}
				sem.m_str = ls.buf;
				return TK_KEYWORD;
				break;
			default:{
				// symbol?
				if ( isalnum(c) ) {
					do {
						ls.SaveAndNext();
					} while ( isalnum(ls.cur) );

					sem.m_str = ls.buf;

					// printf("sym buf: [%s]\n", ls.buf.c_str());

					return TK_SYMBOL;
				}
				ASSERT( 0 );
				return TK_INVALID;
			}
		}
	}

	return TK_INVALID;
}

struct Token
{
	TokenType	m_type;
	SemInfo		m_sem;
};

int main(int argc, char* argv[])
{
	puts("hello");

	const char* filepath = "./sample/sample.scm";

	int srcSize;
	const char* srcText = ReadSexp(filepath, &srcSize);


	cout << "srcSize: " << srcSize << endl;
	// cout << "srcText: " << srcText << endl;
	
	LexState ls;
	ls.Init(srcText, srcSize);

	TokenType	ttype;
	int			i = 0;

	std::vector<Token> tokens;
	do {
		Token token;
		token.m_sem.m_num = 0.f;

		ttype = GetToken(ls, token.m_sem);
		token.m_type = ttype;

		tokens.push_back(token);

		i++;
	} while( ttype != TK_EOF );


	puts("----");

	for ( auto t : tokens ) {
		printf("Toke: %d, %s, sem: [%s][%f]\n",
				t.m_type,
				s_tokenLabels[t.m_type],
				t.m_sem.m_str.c_str(),
				t.m_sem.m_num);
	}

	puts("the end of program");

	return 0;
}

