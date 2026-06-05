#ifndef TOKEN_H
#define TOKEN_H

// parsing / tokenizing, whatever the fuck you wanna call it

typedef struct TokenList{
    const char *start;
    const char *end;

    struct TokenList *next;
} TokenList;

TokenList  *tokenListInit(void);
TokenList  *tokenListAppend(TokenList *list, const char *start, const char *end);
void        tokenListPrint(TokenList *list);
TokenList  *tokenizeSrc(const char *line);

int         isWhiteSpace(const char *p);
const char *skipWhiteSpace(const char *p);
int         isDigit(const char p);

#endif
