#pragma once

inline void NU_Internal_Set_Cursor_Default()
{
    SDL_SetCursor(__NGUI.cursorDefault);
}

inline void NU_Internal_Set_Cursor_Pointer()
{
    SDL_SetCursor(__NGUI.cursorPointer);
}

inline void NU_Internal_Set_Cursor_Text()
{
    SDL_SetCursor(__NGUI.cursorText);
}

inline void NU_Internal_Set_Cursor_Wait()
{
    SDL_SetCursor(__NGUI.cursorWait);
}

inline void NU_Internal_Set_Cursor_Crosshair()
{
    SDL_SetCursor(__NGUI.cursorCrosshair);
}

inline void NU_Internal_Set_Cursor_Move()
{
    SDL_SetCursor(__NGUI.cursorMove);
}

inline void NU_Internal_Set_Cursor_NsResize()
{
    SDL_SetCursor(__NGUI.cursorNsResize);
}

inline void NU_Internal_Set_Cursor_EwResize()
{
    SDL_SetCursor(__NGUI.cursorEwResize);
}

inline void NU_Internal_Set_Cursor_NwseResize()
{
    SDL_SetCursor(__NGUI.cursorNwseResize);
}

inline void NU_Internal_Set_Cursor_NeswResize()
{
    SDL_SetCursor(__NGUI.cursorNeswResize);
}