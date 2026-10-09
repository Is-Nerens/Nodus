#pragma once

#include "nu_stylesheet_tokens.h"

static int AssertSelectionOpeningBraceGrammar(TokenArray* tokens, int i)
{
    // ENFORCE RULE: NEXT TOKEN MUST BE A PROPERTY INDENTIFIER OR CLOSING BRACE
    if (i < tokens->size - 1)
    {
        StylesheetToken next_token = TokenArray_Get(tokens, i+1);
        if (Stylesheet_IsPropertyIdentifierToken(next_token) || next_token == STYLE_SELECTOR_CLOSE_BRACE) return 1;
    }
    ErrorSystem_AddError(&GUI.errorSystem, "<CSS Error> Expected property identifier");
    return 0;
}

static int AssertSelectionClosingBraceGrammar(TokenArray* tokens, int i)
{
    // ENFORCE RULE: MUST BE LAST TOKEN OR NEXT TOKEN MUST BE A SELECTOR
    if (i == tokens->size - 1) return 1;
    StylesheetToken next_token = TokenArray_Get(tokens, i+1);
    if (next_token == STYLE_CLASS_SELECTOR ||
        next_token == STYLE_ID_SELECTOR ||
        Stylesheet_IsTagSelectorToken(next_token) ||
        next_token == STYLE_FONT_CREATION_SELECTOR ||
        next_token == STYLE_DEFAULT_SELECTOR ||
        next_token == STYLE_VAR_SELECTOR ||
        next_token == STYLE_SCREEN_SELECTOR ||
        next_token == STYLE_SCROLLBAR_SELECTOR ||
        next_token == STYLE_SCROLL_THUMB_SELECTOR ||
        next_token == STYLE_SCROLL_TRACK_SELECTOR)
    {
        return 1;
    }
    ErrorSystem_AddError(&GUI.errorSystem, "<CSS Error> Expected a selector or end of file");
    return 0;
}

static int AssertSelectorGrammar(TokenArray* tokens, int i)
{
    // ENFORCE RULE: NEXT TOKEN MUST BE A COMMA OR SELECTOR OPENING BRACE
    if (i < tokens->size - 1)
    {
        StylesheetToken next_token = TokenArray_Get(tokens, i+1);
        return next_token == STYLE_SELECTOR_COMMA || next_token == STYLE_SELECTOR_OPEN_BRACE;
    }
    ErrorSystem_AddError(&GUI.errorSystem, "<CSS Error> Expected comma ',' or scope '{'");
    return 0;
}

static int AssertVariableSelectorGrammar(TokenArray* tokens, int i)
{
    // ENFORCE RULE: NEXT TOKEN MUST BE AN OPEN BRACE
    if (i < tokens->size - 1 && TokenArray_Get(tokens, i+1) == STYLE_SELECTOR_OPEN_BRACE) return 1;
    ErrorSystem_AddError(&GUI.errorSystem, "<CSS Error> Expected scope ','");
    return 0;
}

static int AssertFontCreationSelectorGrammar(TokenArray* tokens, int i)
{
    // ENFORCE RULE: NEXT TOKEN MUST BE A FONT CREATION NAME
    // ENFORCE RULE: FOLLOWING TOKEN MUST BE AN OPEN BRACE
    if (i < tokens->size - 2)
    {
        StylesheetToken next_token = TokenArray_Get(tokens, i+1);
        StylesheetToken following_token = TokenArray_Get(tokens, i+2);
        if (next_token == STYLE_FONT_NAME && following_token == STYLE_SELECTOR_OPEN_BRACE) return 1;
    }
    ErrorSystem_AddError(&GUI.errorSystem, "<CSS Error> @font selector must be followed by a name and scope '{'");
    return 0;
}

static int AssertDefaultSelectorGrammar(TokenArray* tokens, int i)
{
    // ENFORCE RULE: NEXT TOKEN MUST BE A SELECTOR OPEN BRACE
    if (i < tokens->size - 1 && TokenArray_Get(tokens, i+1) == STYLE_SELECTOR_OPEN_BRACE) return 1;
    ErrorSystem_AddError(&GUI.errorSystem, "<CSS Error> Expected scope ','");
    return 0;
}

static int AssertScrollSelectorGrammar(TokenArray* tokens, int i)
{
    // ENFORCE RULE: NEXT TOKEN MUST BE A SELECTOR OPEN BRACE
    if (i < tokens->size - 1 && TokenArray_Get(tokens, i+1) == STYLE_SELECTOR_OPEN_BRACE) return 1;
    ErrorSystem_AddError(&GUI.errorSystem, "<CSS Error> Expected scope ','");
    return 0;
}

static int AssertScreenSelectorGrammar(TokenArray* tokens, int i)
{
    // ENFORCE RULE: NEXT TOKEN MUST BE A COMPARISON OPERATOR
    // ENFORCE RULE: FOLLOWING TOKEN MUST BE A STYLE_SCREEN_QUERY_WIDTH
    // ENFORCE RULE: FOLLOWING TOKEN MUST BE A SELECTOR OPEN BRACE
    if (i < tokens->size - 2)
    {
        StylesheetToken next_token = TokenArray_Get(tokens, i+1);
        StylesheetToken following_token = TokenArray_Get(tokens, i+2);
        StylesheetToken third_token = TokenArray_Get(tokens, i+3);
        if ((next_token == STYLE_GREATER ||
            next_token == STYLE_LESS ||
            next_token == STYLE_GREATER_EQUAL ||
            next_token == STYLE_LESS_EQUAL) &&
            following_token == STYLE_SCREEN_QUERY_WIDTH && third_token == STYLE_SELECTOR_OPEN_BRACE) return 1;
    }
    ErrorSystem_AddError(&GUI.errorSystem, "<CSS Error> Expected [camparator (>, <, >=, <=), screen width (int), open brace '{'}]  ','");
    return 0;
}

static int AssertSelectorCommaGrammar(TokenArray* tokens, int i)
{
    // ENFORCE RULE: NEXT TOKEN MUST BE A SELECTOR
    if (i < tokens->size - 1)
    {
        StylesheetToken next_token = TokenArray_Get(tokens, i+1);
        if (next_token == STYLE_CLASS_SELECTOR || next_token == STYLE_ID_SELECTOR || Stylesheet_IsTagSelectorToken(next_token)) return 1;
    }
    ErrorSystem_AddError(&GUI.errorSystem, "<CSS Error> Expected selector after comma ','");
    return 0;
}

static int AssertPropertyIdentifierGrammar(TokenArray* tokens, int i)
{
    // ENFORCE RULE: NEXT TOKEN MUST BE A PROPERTY ASSIGNMENT ':'
    // ENFORCE RULE: FOLLOWING TOKEN MUST BE A PROPERTY VALUE OR VARIABLE PROPERTY VALUE
    // ENFORCE RULE: THIRD TOKENS MUST BE CLOSE BRACE OR ANOTHER PROPERTY IDENTIFIER
    if (i < tokens->size - 3)
    {
        StylesheetToken next_token = TokenArray_Get(tokens, i+1);
        StylesheetToken following_token = TokenArray_Get(tokens, i+2);
        StylesheetToken third_token = TokenArray_Get(tokens, i+3);
        if (next_token == STYLE_PROPERTY_ASSIGNMENT &&
            (following_token == STYLE_PROPERTY_VALUE || following_token == STYLE_VARIABLE_PROPERTY_VALUE) &&
            (third_token == STYLE_SELECTOR_CLOSE_BRACE || Stylesheet_IsPropertyIdentifierToken(third_token))) return 1;
    }
    ErrorSystem_AddError(&GUI.errorSystem, "<CSS Error> Expected property assignment");
    return 0;
}

static int AssertVariableAssignmentGrammar(TokenArray* tokens, int i)
{
    // ENFORCE RULE: NEXT TOKEN MUST BE A PROPERTY ASSIGNMENT ':'
    // ENFORCE RULE: FOLLOWING TOKEN MUST BE A PROPERTY VALUE
    // // ENFORCE RULE: THIRD TOKENS MUST BE CLOSE BRACE OR ANOTHER VARIABLE NAME
    if (i < tokens->size - 3)
    {
        StylesheetToken next_token = TokenArray_Get(tokens, i+1);
        StylesheetToken following_token = TokenArray_Get(tokens, i+2);
        StylesheetToken third_token = TokenArray_Get(tokens, i+3);
        return next_token == STYLE_PROPERTY_ASSIGNMENT && following_token == STYLE_PROPERTY_VALUE && (third_token == STYLE_SELECTOR_CLOSE_BRACE || third_token == STYLE_VARIABLE_NAME);
    }
    ErrorSystem_AddError(&GUI.errorSystem, "<CSS Error> Expected variable assignment");
    return 0;
}
