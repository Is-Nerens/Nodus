#pragma once

static int AssertSelectionOpeningBraceGrammar(struct Vector* tokens, int i)
{
    // ENFORCE RULE: NEXT TOKEN MUST BE A PROPERTY INDENTIFIER OR CLOSING BRACE
    if (i < tokens->size - 1)
    {
        enum NU_Style_Token next_token = *((enum NU_Style_Token*) Vector_Get(tokens, i+1));
        if (NU_Is_Property_Identifier_Token(next_token) || next_token == STYLE_SELECTOR_CLOSE_BRACE) return 1;
    }
    printf("%s", "[Generate Stylesheet] Error! Expected property identifier!");
    return 0;
}

static int AssertFontCreationSelectorGrammar(struct Vector* tokens, int i)
{
    // ENFORCE RULE: NEXT TOKEN MUST BE A FONT CREATION NAME 
    // ENFORCE RULE: FOLLOWING TOKEN MUST BE AN OPEN BRACE
    if (i < tokens->size - 2)
    {
        enum NU_Style_Token next_token = *((enum NU_Style_Token*) Vector_Get(tokens, i+1));
        enum NU_Style_Token following_token = *((enum NU_Style_Token*) Vector_Get(tokens, i+2));
        if (next_token == STYLE_FONT_NAME && following_token == STYLE_SELECTOR_OPEN_BRACE) return 1;
    }
    printf("%s", "[Generate Stylesheet] Error! @font selector must be followed by a name and {!");
    return 0;
}

static int AssertSelectionClosingBraceGrammar(struct Vector* tokens, int i)
{
    // ENFORCE RULE: MUST BE LAST TOKEN OR NEXT TOKEN MUST BE A SELECTOR
    if (i == tokens->size - 1) return 1;
    enum NU_Style_Token next_token = *((enum NU_Style_Token*) Vector_Get(tokens, i+1));
    if (next_token == STYLE_CLASS_SELECTOR || next_token == STYLE_ID_SELECTOR || NU_Is_Tag_Selector_Token(next_token) || next_token == STYLE_FONT_CREATION_SELECTOR) return 1;
    printf("%s", "[Generate Stylesheet] Error! Expected selector or end of file!");
    return 0;
}

static int AssertSelectorGrammar(struct Vector* tokens, int i)
{
    // ENFORCE RULE: NEXT TOKEN MUST BE A COMMA OR SELECTION OPENING BRACE
    if (i < tokens->size - 1)
    {
        enum NU_Style_Token next_token = *((enum NU_Style_Token*) Vector_Get(tokens, i+1));
        return next_token == STYLE_SELECTOR_COMMA || next_token == STYLE_SELECTOR_OPEN_BRACE;
    }
    return 0;
}

static int AssertSelectorCommaGrammar(struct Vector* tokens, int i)
{
    // ENFORCE RULE: NEXT TOKEN MUST BE A SELECTOR
    if (i < tokens->size - 1)
    {
        enum NU_Style_Token next_token = *((enum NU_Style_Token*) Vector_Get(tokens, i+1));
        if (next_token == STYLE_CLASS_SELECTOR || next_token == STYLE_ID_SELECTOR || NU_Is_Tag_Selector_Token(next_token)) return 1;
    }
    printf("%s", "[Generate Stylesheet] Error! Expected selector after selector comma!");
    return 0;
}

static int AssertPropertyIdentifierGrammar(struct Vector* tokens, int i)
{
    // ENFORCE RULE: NEXT TOKEN MUST BE A PROPERTY ASSIGNMENT ':' 
    // ENFORCE RULE: FOLLOWING TOKEN MUST BE A PROPERTY VALUE
    if (i < tokens->size - 3)
    {
        enum NU_Style_Token next_token = *((enum NU_Style_Token*) Vector_Get(tokens, i+1));
        enum NU_Style_Token following_token = *((enum NU_Style_Token*) Vector_Get(tokens, i+2));
        enum NU_Style_Token third_token = *((enum NU_Style_Token*) Vector_Get(tokens, i+3));
        return next_token == STYLE_PROPERTY_ASSIGNMENT && following_token == STYLE_PROPERTY_VALUE && (third_token == STYLE_SELECTOR_CLOSE_BRACE || NU_Is_Property_Identifier_Token(third_token));
    }
    return 0;
}
